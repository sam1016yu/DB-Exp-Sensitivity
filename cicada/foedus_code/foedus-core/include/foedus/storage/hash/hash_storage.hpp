/*
 * Copyright (c) 2014-2015, Hewlett-Packard Development Company, LP.
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details. You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * HP designates this particular file as subject to the "Classpath" exception
 * as provided by HP in the LICENSE.txt file that accompanied this code.
 */
#ifndef FOEDUS_STORAGE_HASH_HASH_STORAGE_HPP_
#define FOEDUS_STORAGE_HASH_HASH_STORAGE_HPP_
#include <iosfwd>
#include <string>

#include "foedus/attachable.hpp"
#include "foedus/cxx11.hpp"
#include "foedus/fwd.hpp"
#include "foedus/storage/fwd.hpp"
#include "foedus/storage/storage.hpp"
#include "foedus/storage/storage_id.hpp"
#include "foedus/storage/hash/fwd.hpp"
#include "foedus/storage/hash/hash_combo.hpp"
#include "foedus/storage/hash/hash_id.hpp"
#include "foedus/thread/fwd.hpp"
#include "foedus/xct/fwd.hpp"
#include "foedus/xct/xct_id.hpp"

namespace foedus {
namespace storage {
namespace hash {
/**
 * @brief Represents a key-value store based on a dense and regular hash.
 * @ingroup HASH
 */
class HashStorage CXX11_FINAL : public Storage<HashStorageControlBlock> {
 public:
  typedef HashStoragePimpl   ThisPimpl;
  typedef HashCreateLogType  ThisCreateLogType;
  typedef HashMetadata       ThisMetadata;

  HashStorage();
  HashStorage(Engine* engine, HashStorageControlBlock* control_block);
  HashStorage(Engine* engine, StorageControlBlock* control_block);
  HashStorage(Engine* engine, StorageId id);
  HashStorage(Engine* engine, const StorageName& name);
  HashStorage(const HashStorage& other);
  HashStorage& operator=(const HashStorage& other);

  // Storage interface
  const HashMetadata* get_hash_metadata()  const;
  /** @return levels of intermediate pages in this storage */
  uint8_t             get_levels() const;
  /** @return the total number of hash bins in this storage */
  HashBin             get_bin_count() const;
  /** @return the number of bits to represent hash bins in this storage */
  uint8_t             get_bin_bits() const;
  /** @return the number of bit shifts to extract bins from hashes for this storage */
  uint8_t             get_bin_shifts() const;
  /** @return the number of child pointers in the root page for this storage */
  uint16_t            get_root_children() const;
  ErrorStack          create(const Metadata &metadata);
  ErrorStack          load(const StorageControlBlock& snapshot_block);
  ErrorStack          drop();
  friend std::ostream& operator<<(std::ostream& o, const HashStorage& v);

  /**
   * @copydoc foedus::storage::StorageManager::track_moved_record()
   * @note Implementation note
   * Hash storage also uses the moved bit for record expansion.
   * The basic idea and protocol are same as the masstree package, but it's easier here!
   *
   * When we migrate records for expansion, we only move it to a later position, either
   * a larger slot index in the same page or somewhere in next-page linked-list.
   * Further, we keep the full key in the original place.
   * So, tracking the moved record is fairly simple and efficient.
   * Also, no chance of cannot-track case.
   */
  xct::TrackMovedRecordResult track_moved_record(
    xct::RwLockableXctId* old_address,
    xct::WriteXctAccess* write_set);

  ErrorStack  verify_single_thread(Engine* engine);
  ErrorStack  verify_single_thread(thread::Thread* context);

  /**
   * Resets all volatile pages' temperature stat to be zero in this storage.
   * Used only in HCC-branch.
   */
  ErrorStack  hcc_reset_all_temperature_stat();

  /**
   * @brief A super-expensive and single-thread only debugging feature to write out
   * gigantic human-readable texts to describe the hash storage in details.
   * @details
   * Do not invoke this method for more than 100 pages, or in production use.
   * This is for debugging.
   */
  ErrorStack  debugout_single_thread(
    Engine* engine,
    bool volatile_only = false,
    bool intermediate_only = false,
    uint32_t max_pages = 1024U);

  //// Hash table API
  // TASK(Hideaki) Add primitive-optimized versions and increment versions.
  // Low priority. Most costs are from page-traversal and hashinate.


  /**
   * Prepares a set of information that are used in many places, extracted from the given key.
   */
  inline HashCombo combo(const void* key, uint16_t key_length) const {
    return HashCombo(key, key_length, *get_hash_metadata());
  }
  /**
   * Overlord to receive key as a primitive type.
   * @attention Here we receive a pointer because HashCombo remembers the point to key.
   * In other words, the caller must make sure the primitive variable doesn't get out of scope
   * while it might reuse the returned HashCombo.
   */
  template <typename KEY>
  inline HashCombo combo(KEY* key) const {
    return HashCombo(key, sizeof(KEY), *get_hash_metadata());
  }


  // get_record() methods

  /**
   * @brief Retrieves an entire payload of the given key in this hash storage.
   * @param[in] context Thread context
   * @param[in] key Arbitrary length of key.
   * @param[in] key_length Byte size of key.
   * @param[out] payload Buffer to receive the payload of the record.
   * @param[in,out] payload_capacity [In] Byte size of the payload buffer, [Out] length of
   * the payload. This is set whether the payload capacity was too small or not.
   * @param[in] read_only Whether this read will not be followed by writes. When MOCC triggers
   * pessimistic lock, this guides us to take either read- or write-lock.
   * @details
   * When payload_capacity is smaller than the actual payload, this method returns
   * kErrorCodeStrTooSmallPayloadBuffer and payload_capacity is set to be the required length.
   *
   * When the key is not found (kErrorCodeStrKeyNotFound), we add an appropriate
   * page-version set because it is part of a transactional information.
   */
  inline ErrorCode get_record(
    thread::Thread* context,
    const void* key,
    uint16_t key_length,
    void* payload,
    uint16_t* payload_capacity,
    bool read_only) {
    HashCombo c(combo(key, key_length));
    return get_record(context, key, key_length, c, payload, payload_capacity, read_only);
  }

  /** Overlord to receive key as a primitive type. */
  template <typename KEY>
  inline ErrorCode get_record(
    thread::Thread* context,
    KEY key,
    void* payload,
    uint16_t* payload_capacity,
    bool read_only) {
    HashCombo c(combo<KEY>(&key));
    return get_record(context, &key, sizeof(key), c, payload, payload_capacity, read_only);
  }

  /** If you have already computed HashCombo, use this. */
  ErrorCode get_record(
    thread::Thread* context,
    const void* key,
    uint16_t key_length,
    const HashCombo& combo,
    void* payload,
    uint16_t* payload_capacity,
    bool read_only);

  /**
   * @brief Retrieves a part of the given key in this hash storage.
   * @param[in] context Thread context
   * @param[in] key Arbitrary length of key.
   * @param[in] key_length Byte size of key.
   * @param[out] payload Buffer to receive the payload of the record.
   * @param[in] payload_offset We copy from this byte position of the record.
   * @param[in] payload_count How many bytes we copy.
   * @param[in] read_only Whether this read will not be followed by writes. When MOCC triggers
   * pessimistic lock, this guides us to take either read- or write-lock.
   * @pre payload_offset + payload_count must be within the record's actual payload size
   * (returns kErrorCodeStrTooShortPayload if not)
   */
  inline ErrorCode get_record_part(
    thread::Thread* context,
    const void* key,
    uint16_t key_length,
    void* payload,
    uint16_t payload_offset,
    uint16_t payload_count,
    bool read_only) {
    HashCombo c(combo(key, key_length));
    return get_record_part(
      context,
      key,
      key_length,
      c,
      payload,
      payload_offset,
      payload_count,
      read_only);
  }

  /** Overlord to receive key as a primitive type. */
  template <typename KEY>
  inline ErrorCode get_record_part(
    thread::Thread* context,
    KEY key,
    void* payload,
    uint16_t payload_offset,
    uint16_t payload_count,
    bool read_only) {
    HashCombo c(combo<KEY>(&key));
    return get_record_part(
      context,
      &key,
      sizeof(key),
      c,
      payload,
      payload_offset,
      payload_count,
      read_only);
  }

  /** If you have already computed HashCombo, use this. */
  ErrorCode get_record_part(
    thread::Thread* context,
    const void* key,
    uint16_t key_length,
    const HashCombo& combo,
    void* payload,
    uint16_t payload_offset,
    uint16_t payload_count,
    bool read_only);

  /**
   * @brief Retrieves a part of the given key in this storage as a primitive value.
   * @param[in] context Thread context
   * @param[in] key Arbitrary length of key.
   * @param[in] key_length Byte size of key.
   * @param[out] payload Receive the payload of the record.
   * @param[in] payload_offset We copy from this byte position of the record.
   * @param[in] read_only Whether this read will not be followed by writes. When MOCC triggers
   * pessimistic lock, this guides us to take either read- or write-lock.
   * @pre payload_offset + sizeof(PAYLOAD) must be within the record's actual payload size
   * (returns kErrorCodeStrTooShortPayload if not)
   * @tparam PAYLOAD primitive type of the payload. all integers and floats are allowed.
   */
  template <typename PAYLOAD>
  inline ErrorCode get_record_primitive(
    thread::Thread* context,
    const void* key,
    uint16_t key_length,
    PAYLOAD* payload,
    uint16_t payload_offset,
    bool read_only) {
    HashCombo c(combo(key, key_length));
    return get_record_primitive<PAYLOAD>(
      context,
      key,
      key_length,
      c,
      payload,
      payload_offset,
      read_only);
  }

  /** Overlord to receive key as a primitive type. */
  template <typename KEY, typename PAYLOAD>
  inline ErrorCode get_record_primitive(
    thread::Thread* context,
    KEY key,
    PAYLOAD* payload,
    uint16_t payload_offset,
    bool read_only) {
    HashCombo c(combo<KEY>(&key));
    return get_record_primitive<PAYLOAD>(
      context,
      &key,
      sizeof(key),
      c,
      payload,
      payload_offset,
      read_only);
  }

  /** If you have already computed HashCombo, use this. */
  template <typename PAYLOAD>
  ErrorCode   get_record_primitive(
    thread::Thread* context,
    const void* key,
    uint16_t key_length,
    const HashCombo& combo,
    PAYLOAD* payload,
    uint16_t payload_offset,
    bool read_only);

  // insert_record() methods

  /**
   * @brief Inserts a new record of the given key in this hash storage.
   * @param[in] context Thread context
   * @param[in] key Arbitrary length of key.
   * @param[in] key_length Byte size of key.
   * @param[in] payload Value to insert.
   * @param[in] payload_count Length of payload.
   * @details
   * If the key already exists, it returns kErrorCodeStrKeyAlreadyExists and we add an appropriate
   * page-version set because it is part of a transactional information.
   */
  inline ErrorCode insert_record(
    thread::Thread* context,
    const void* key,
    uint16_t key_length,
    const void* payload,
    uint16_t payload_count) {
    HashCombo c(combo(key, key_length));
    return insert_record(context, key, key_length, c, payload, payload_count, payload_count);
  }

  /** Overlord to receive key as a primitive type. */
  template <typename KEY>
  inline ErrorCode insert_record(
    thread::Thread* context,
    KEY key,
    const void* payload,
    uint16_t payload_count) {
    HashCombo c(combo<KEY>(&key));
    return insert_record(context, &key, sizeof(key), c, payload, payload_count, payload_count);
  }

  /**
   * If you have already computed HashCombo, use this.
   * With this method, you can also specify \e physical_payload_hint, the
   * physical size of the record's payload part. If you will later expand
   * the payload of this record, giving a larger value will avoid record migration.
   * Default value (and minimal value) is same as the initial payload_count.
   */
  ErrorCode   insert_record(
    thread::Thread* context,
    const void* key,
    uint16_t key_length,
    const HashCombo& combo,
    const void* payload,
    uint16_t payload_count,
    uint16_t physical_payload_hint);

  // delete_record() methods

  /**
   * @brief Deletes a record of the given key from this hash storage.
   * @param[in] context Thread context
   * @param[in] key Arbitrary length of key.
   * @param[in] key_length Byte size of key.
   * @details
   * When the key does not exist, it returns kErrorCodeStrKeyNotFound and we add an appropriate
   * page-version set because it is part of a transactional information.
   */
  inline ErrorCode delete_record(thread::Thread* context, const void* key, uint16_t key_length) {
    HashCombo c(combo(key, key_length));
    return delete_record(context, key, key_length, c);
  }

  /** Overlord to receive key as a primitive type. */
  template <typename KEY>
  inline ErrorCode delete_record(thread::Thread* context, KEY key) {
    HashCombo c(combo<KEY>(&key));
    return delete_record(context, &key, sizeof(key), c);
  }

  /** If you have already computed HashCombo, use this. */
  ErrorCode   delete_record(
    thread::Thread* context,
    const void* key,
    uint16_t key_length,
    const HashCombo& combo);

  // upsert_record() methods

  /**
   * @brief Inserts a new record of the given key or replaces the existing one
   * in this hash storage, or so-called \e upsert.
   * @param[in] context Thread context
   * @param[in] key Arbitrary length of key.
   * @param[in] key_length Byte size of key.
   * @param[in] payload Value to insert.
   * @param[in] payload_count Length of payload.
   * @details
   * This method puts the record whether the key already exists or not, which is handy
   * in many usecases. Internally, the Implementation comes with a bit of complexity.
   * If there is an existing record of the key (whether logically deleted or not)
   * that is spacious enough, then we simply replace the record.
   * If the key doesn't exist or the existing record is no big enough, we create/migrate
   * the record in a system transaction then install the record.
   */
  inline ErrorCode upsert_record(
    thread::Thread* context,
    const void* key,
    uint16_t key_length,
    const void* payload,
    uint16_t payload_count) {
    HashCombo c(combo(key, key_length));
    return upsert_record(context, key, key_length, c, payload, payload_count, payload_count);
  }

  /** Overlord to receive key as a primitive type. */
  template <typename KEY>
  inline ErrorCode upsert_record(
    thread::Thread* context,
    KEY key,
    const void* payload,
    uint16_t payload_count) {
    HashCombo c(combo<KEY>(&key));
    return upsert_record(context, &key, sizeof(key), c, payload, payload_count, payload_count);
  }

  /** If you have already computed HashCombo, use this. */
  ErrorCode   upsert_record(
    thread::Thread* context,
    const void* key,
    uint16_t key_length,
    const HashCombo& combo,
    const void* payload,
    uint16_t payload_count,
    uint16_t physical_payload_hint);

  // overwrite_record() methods

  /**
   * @brief Overwrites a part of one record of the given key in this hash storage.
   * @param[in] context Thread context
   * @param[in] key Arbitrary length of key.
   * @param[in] key_length Byte size of key.
   * @param[in] payload We copy from this buffer. Must be at least payload_count.
   * @param[in] payload_offset We overwrite to this byte position of the record.
   * @param[in] payload_count How many bytes we overwrite.
   * @details
   * When payload_offset+payload_count is larger than the actual payload, this method returns
   * kErrorCodeStrTooShortPayload. Just like others, when the key does not exist,
   * it returns kErrorCodeStrKeyNotFound and we add an appropriate
   * page-version set because it is part of a transactional information.
   */
  inline ErrorCode overwrite_record(
    thread::Thread* context,
    const void* key,
    uint16_t key_length,
    const void* payload,
    uint16_t payload_offset,
    uint16_t payload_count) {
    HashCombo c(combo(key, key_length));
    return overwrite_record(
      context,
      key,
      key_length,
      c,
      payload,
      payload_offset,
      payload_count);
  }

  /** Overlord to receive key as a primitive type. */
  template <typename KEY>
  inline ErrorCode overwrite_record(
    thread::Thread* context,
    KEY key,
    const void* payload,
    uint16_t payload_offset,
    uint16_t payload_count) {
    HashCombo c(combo<KEY>(&key));
    return overwrite_record(context, &key, sizeof(key), c, payload, payload_offset, payload_count);
  }

  /** If you have already computed HashCombo, use this. */
  ErrorCode   overwrite_record(
    thread::Thread* context,
    const void* key,
    uint16_t key_length,
    const HashCombo& combo,
    const void* payload,
    uint16_t payload_offset,
    uint16_t payload_count);

  /**
   * @brief Overwrites a part of one record of the given key in this storage as a primitive value.
   * @param[in] context Thread context
   * @param[in] key Arbitrary length of key.
   * @param[in] key_length Byte size of key.
   * @param[in] payload We copy this value.
   * @param[in] payload_offset We overwrite to this byte position of the record.
   * @pre payload_offset + sizeof(PAYLOAD) must be within the record's actual payload size
   * (returns kErrorCodeStrTooShortPayload if not)
   * @tparam PAYLOAD primitive type of the payload. all integers and floats are allowed.
   */
  template <typename PAYLOAD>
  inline ErrorCode overwrite_record_primitive(
    thread::Thread* context,
    const void* key,
    uint16_t key_length,
    PAYLOAD payload,
    uint16_t payload_offset) {
    HashCombo c(combo(key, key_length));
    return overwrite_record_primitive(context, key, key_length, c, payload, payload_offset);
  }

  /** Overlord to receive key as a primitive type. */
  template <typename KEY, typename PAYLOAD>
  inline ErrorCode overwrite_record_primitive(
    thread::Thread* context,
    KEY key,
    PAYLOAD payload,
    uint16_t payload_offset) {
    HashCombo c(combo<KEY>(&key));
    return overwrite_record_primitive(context, &key, sizeof(key), c, payload, payload_offset);
  }

  /** If you have already computed HashCombo, use this. */
  template <typename PAYLOAD>
  ErrorCode   overwrite_record_primitive(
    thread::Thread* context,
    const void* key,
    uint16_t key_length,
    const HashCombo& combo,
    PAYLOAD payload,
    uint16_t payload_offset);

  // increment_record() methods

  /**
   * @brief This one further optimizes overwrite methods for the frequent use
   * case of incrementing some data in primitive type.
   * @param[in] context Thread context
   * @param[in] key Arbitrary length of key.
   * @param[in] key_length Byte size of key.
   * @param[in,out] value (in) addendum, (out) value after addition.
   * @param[in] payload_offset We overwrite to this byte position of the record.
   * @pre payload_offset + sizeof(PAYLOAD) must be within the record's actual payload size
   * (returns kErrorCodeStrTooShortPayload if not)
   * @tparam PAYLOAD primitive type of the payload. all integers and floats are allowed.
   */
  template <typename PAYLOAD>
  inline ErrorCode increment_record(
    thread::Thread* context,
    const void* key,
    uint16_t key_length,
    PAYLOAD* value,
    uint16_t payload_offset) {
    HashCombo c(combo(key, key_length));
    return increment_record(context, key, key_length, c, value, payload_offset);
  }

  /** Overlord to receive key as a primitive type. */
  template <typename KEY, typename PAYLOAD>
  inline ErrorCode increment_record(
    thread::Thread* context,
    KEY key,
    PAYLOAD* value,
    uint16_t payload_offset) {
    HashCombo c(combo<KEY>(&key));
    return increment_record(context, &key, sizeof(key), c, value, payload_offset);
  }

  /** If you have already computed HashCombo, use this. */
  template <typename PAYLOAD>
  ErrorCode       increment_record(
    thread::Thread* context,
    const void* key,
    uint16_t key_length,
    const HashCombo& combo,
    PAYLOAD* value,
    uint16_t payload_offset);
};
}  // namespace hash
}  // namespace storage
}  // namespace foedus
#endif  // FOEDUS_STORAGE_HASH_HASH_STORAGE_HPP_
