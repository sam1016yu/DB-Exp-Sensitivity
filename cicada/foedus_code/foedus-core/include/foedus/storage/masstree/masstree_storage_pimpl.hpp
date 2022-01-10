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
#ifndef FOEDUS_STORAGE_MASSTREE_MASSTREE_STORAGE_PIMPL_HPP_
#define FOEDUS_STORAGE_MASSTREE_MASSTREE_STORAGE_PIMPL_HPP_
#include <stdint.h>

#include <mutex>
#include <string>
#include <vector>

#include "foedus/attachable.hpp"
#include "foedus/compiler.hpp"
#include "foedus/cxx11.hpp"
#include "foedus/fwd.hpp"
#include "foedus/cache/fwd.hpp"
#include "foedus/log/fwd.hpp"
#include "foedus/memory/fwd.hpp"
#include "foedus/soc/shared_memory_repo.hpp"
#include "foedus/storage/fwd.hpp"
#include "foedus/storage/page.hpp"
#include "foedus/storage/storage.hpp"
#include "foedus/storage/storage_id.hpp"
#include "foedus/storage/masstree/fwd.hpp"
#include "foedus/storage/masstree/masstree_id.hpp"
#include "foedus/storage/masstree/masstree_metadata.hpp"
#include "foedus/storage/masstree/masstree_page_impl.hpp"
#include "foedus/storage/masstree/masstree_storage.hpp"
#include "foedus/thread/thread.hpp"
#include "foedus/xct/xct_id.hpp"

namespace foedus {
namespace storage {
namespace masstree {
/** Shared data of this storage type */
struct MasstreeStorageControlBlock final {
  // this is backed by shared memory. not instantiation. just reinterpret_cast.
  MasstreeStorageControlBlock() = delete;
  ~MasstreeStorageControlBlock() = delete;

  bool exists() const { return status_ == kExists || status_ == kMarkedForDeath; }

  soc::SharedMutex    status_mutex_;
  /** Status of the storage */
  StorageStatus       status_;
  /**
   * Points to the root page (or something equivalent).
   * Masstree-specific:
   * Volatile pointer is always active.
   * This is always MasstreeIntermediatePage (a contract only for first layer's root page).
   * When the first layer B-tree grows, this points to a new page. So, this is one of the few
   * page pointers that might be \e swapped. Transactions thus have to add this to a pointer
   * set even thought they are following a volatile pointer.
   *
   * Instead, this always points to a root. We don't need "is_root" check in [YANDONG12] and
   * thus doesn't need a parent pointer.
   */
  DualPagePointer     root_page_pointer_;
  /** metadata of this storage. */
  MasstreeMetadata    meta_;

  // Do NOT reorder members up to here. The layout must be compatible with StorageControlBlock
  // Type-specific shared members below.

  /**
   * Used to synchronize updates to root_page_pointer_.
   * The only usecase is to grow the root of the first layer.
   * This lock must be one that can be used outside of volatile page, i.e., a dumb spinlock
   * or MCS Lock because we put it in the control block. Locks that require UniversalLockId
   * conversion can't be used here, e.g., the Extended RW Lock.
   * We thus use a very different lock here separate from our main lock/commit protocol.
   * @see GrowFirstLayerRoot
   */
  bool                first_root_locked_;
};

/**
 * @brief Pimpl object of MasstreeStorage.
 * @ingroup MASSTREE
 * @details
 * A private pimpl object for MasstreeStorage.
 * Do not include this header from a client program unless you know what you are doing.
 */
class MasstreeStoragePimpl final : public Attachable<MasstreeStorageControlBlock> {
 public:
  MasstreeStoragePimpl() : Attachable<MasstreeStorageControlBlock>() {}
  explicit MasstreeStoragePimpl(MasstreeStorage* storage)
    : Attachable<MasstreeStorageControlBlock>(
      storage->get_engine(),
      storage->get_control_block()) {}

  ErrorStack  create(const MasstreeMetadata& metadata);
  ErrorStack  load(const StorageControlBlock& snapshot_block);
  ErrorStack  load_empty();
  ErrorStack  drop();

  bool                exists()    const { return control_block_->exists(); }
  StorageId           get_id()    const { return control_block_->meta_.id_; }
  const StorageName&  get_name()  const { return control_block_->meta_.name_; }
  const MasstreeMetadata& get_meta()  const { return control_block_->meta_; }
  const DualPagePointer& get_first_root_pointer() const {
    return control_block_->root_page_pointer_;
  }
  DualPagePointer* get_first_root_pointer_address() { return &control_block_->root_page_pointer_; }

  ErrorCode get_first_root(
    thread::Thread* context,
    bool for_write,
    MasstreeIntermediatePage** root);

  /**
   * Find a border node in the layer that corresponds to the given key slice.
   * @note this method is \e physical-only. It doesn't take any long-term lock or readset.
   */
  ErrorCode find_border_physical(
    thread::Thread* context,
    MasstreePage* layer_root,
    uint8_t   current_layer,
    bool      for_writes,
    KeySlice  slice,
    MasstreeBorderPage** border) ALWAYS_INLINE;

  /** Identifies page and record for the key */
  ErrorCode locate_record(
    thread::Thread* context,
    const void* key,
    KeyLength key_length,
    bool for_writes,
    RecordLocation* result);
  /** Identifies page and record for the normalized key */
  ErrorCode locate_record_normalized(
    thread::Thread* context,
    KeySlice key,
    bool for_writes,
    RecordLocation* result);

  /**
   * Like locate_record(), this is also a logical operation.
   */
  ErrorCode reserve_record(
    thread::Thread* context,
    const void* key,
    KeyLength key_length,
    PayloadLength payload_count,
    PayloadLength physical_payload_hint,
    RecordLocation* result);
  ErrorCode reserve_record_normalized(
    thread::Thread* context,
    KeySlice key,
    PayloadLength payload_count,
    PayloadLength physical_payload_hint,
    RecordLocation* result);

  /** implementation of get_record family. use with locate_record() */
  ErrorCode retrieve_general(
    thread::Thread* context,
    const RecordLocation& location,
    void* payload,
    PayloadLength* payload_capacity);
  ErrorCode retrieve_part_general(
    thread::Thread* context,
    const RecordLocation& location,
    void* payload,
    PayloadLength payload_offset,
    PayloadLength payload_count);

  /** Used in the following methods */
  ErrorCode register_record_write_log(
    thread::Thread* context,
    const RecordLocation& location,
    log::RecordLogType* log_entry);

  /** implementation of insert_record family. use with \b reserve_record() */
  ErrorCode insert_general(
    thread::Thread* context,
    const RecordLocation& location,
    const void* be_key,
    KeyLength key_length,
    const void* payload,
    PayloadLength payload_count);

  /** implementation of delete_record family. use with locate_record()  */
  ErrorCode delete_general(
    thread::Thread* context,
    const RecordLocation& location,
    const void* be_key,
    KeyLength key_length);

  /** implementation of upsert_record family. use with \b reserve_record() */
  ErrorCode upsert_general(
    thread::Thread* context,
    const RecordLocation& location,
    const void* be_key,
    KeyLength key_length,
    const void* payload,
    PayloadLength payload_count);

  /** implementation of overwrite_record family. use with locate_record()  */
  ErrorCode overwrite_general(
    thread::Thread* context,
    const RecordLocation& location,
    const void* be_key,
    KeyLength key_length,
    const void* payload,
    PayloadLength payload_offset,
    PayloadLength payload_count);

  /** implementation of increment_record family. use with locate_record()  */
  template <typename PAYLOAD>
  ErrorCode increment_general(
    thread::Thread* context,
    const RecordLocation& location,
    const void* be_key,
    KeyLength key_length,
    PAYLOAD* value,
    PayloadLength payload_offset);

  /** These are defined in masstree_storage_verify.cpp */
  ErrorStack verify_single_thread(thread::Thread* context);
  ErrorStack verify_single_thread_layer(
    thread::Thread* context,
    uint8_t layer,
    MasstreePage* layer_root);
  ErrorStack verify_single_thread_intermediate(
    thread::Thread* context,
    KeySlice low_fence,
    HighFence high_fence,
    MasstreeIntermediatePage* page);
  ErrorStack verify_single_thread_border(
    thread::Thread* context,
    KeySlice low_fence,
    HighFence high_fence,
    MasstreeBorderPage* page);

  /** These are defined in masstree_storage_debug.cpp */
  ErrorStack debugout_single_thread(
    Engine* engine,
    bool volatile_only,
    uint32_t max_pages);
  ErrorStack debugout_single_thread_recurse(
    Engine* engine,
    cache::SnapshotFileSet* fileset,
    MasstreePage* parent,
    bool follow_volatile,
    uint32_t* remaining_pages);
  ErrorStack debugout_single_thread_follow(
    Engine* engine,
    cache::SnapshotFileSet* fileset,
    const DualPagePointer& pointer,
    bool follow_volatile,
    uint32_t* remaining_pages);

  /** For stupid reasons (I'm lazy!) these are defined in _debug.cpp. */
  ErrorStack  hcc_reset_all_temperature_stat();
  ErrorStack  hcc_reset_all_temperature_stat_recurse(MasstreePage* parent);
  ErrorStack  hcc_reset_all_temperature_stat_follow(VolatilePagePointer page_id);

  /**
   * Thread::follow_page_pointer() for masstree.
   * Because of how masstree works, this method never creates a completely new page.
   * In other words, always !pointer->is_both_null().
   * It either reads an existing volatile/snapshot page or creates a volatile page
   * from existing snapshot page.
   */
  ErrorCode follow_page(
    thread::Thread* context,
    bool for_writes,
    storage::DualPagePointer* pointer,
    MasstreePage** page);
  /** Follows to next layer's root page. */
  ErrorCode follow_layer(
    thread::Thread* context,
    bool for_writes,
    MasstreeBorderPage* parent,
    SlotIndex record_index,
    MasstreePage** page) ALWAYS_INLINE;

  /** defined in masstree_storage_prefetch.cpp */
  ErrorCode prefetch_pages_normalized(
    thread::Thread* context,
    bool install_volatile,
    bool cache_snapshot,
    KeySlice from,
    KeySlice to);
  ErrorCode prefetch_pages_normalized_recurse(
    thread::Thread* context,
    bool install_volatile,
    bool cache_snapshot,
    KeySlice from,
    KeySlice to,
    MasstreePage* page);
  ErrorCode prefetch_pages_follow(
    thread::Thread* context,
    DualPagePointer* pointer,
    bool vol_on,
    bool snp_on,
    KeySlice from,
    KeySlice to);

  xct::TrackMovedRecordResult track_moved_record(
    xct::RwLockableXctId* old_address,
    xct::WriteXctAccess* write_set) ALWAYS_INLINE;

  /** Defined in masstree_storage_peek.cpp */
  ErrorCode     peek_volatile_page_boundaries(
    Engine* engine,
    const MasstreeStorage::PeekBoundariesArguments& args);
  ErrorCode     peek_volatile_page_boundaries_next_layer(
    const MasstreePage* layer_root,
    const memory::GlobalVolatilePageResolver& resolver,
    const MasstreeStorage::PeekBoundariesArguments& args);
  ErrorCode     peek_volatile_page_boundaries_this_layer(
    const MasstreePage* layer_root,
    const memory::GlobalVolatilePageResolver& resolver,
    const MasstreeStorage::PeekBoundariesArguments& args);
  ErrorCode     peek_volatile_page_boundaries_this_layer_recurse(
    const MasstreeIntermediatePage* cur,
    const memory::GlobalVolatilePageResolver& resolver,
    const MasstreeStorage::PeekBoundariesArguments& args);

  /** Defined in masstree_storage_fatify.cpp */
  ErrorStack    fatify_first_root(
    thread::Thread* context,
    uint32_t desired_count,
    bool disable_no_record_split);
  ErrorStack    fatify_first_root_double(thread::Thread* context, bool disable_no_record_split);
  /**
   * Returns the count of direct children in the first-root node. This is without lock, so
   * it might not be accurate.
   */
  ErrorCode     approximate_count_root_children(
    thread::Thread* context,
    uint32_t* out);

  static ErrorCode check_next_layer_bit(xct::XctId observed) ALWAYS_INLINE;
};
static_assert(sizeof(MasstreeStoragePimpl) <= kPageSize, "MasstreeStoragePimpl is too large");
static_assert(
  sizeof(MasstreeStorageControlBlock) <= soc::GlobalMemoryAnchors::kStorageMemorySize,
  "MasstreeStorageControlBlock is too large.");
}  // namespace masstree
}  // namespace storage
}  // namespace foedus
#endif  // FOEDUS_STORAGE_MASSTREE_MASSTREE_STORAGE_PIMPL_HPP_
