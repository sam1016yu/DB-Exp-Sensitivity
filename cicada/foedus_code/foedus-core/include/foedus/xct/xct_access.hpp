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
#ifndef FOEDUS_XCT_XCT_ACCESS_HPP_
#define FOEDUS_XCT_XCT_ACCESS_HPP_
#include <stdint.h>

#include <iosfwd>

#include "foedus/compiler.hpp"
#include "foedus/log/fwd.hpp"
#include "foedus/memory/fwd.hpp"
#include "foedus/storage/fwd.hpp"
#include "foedus/storage/page.hpp"
#include "foedus/storage/storage_id.hpp"
#include "foedus/xct/fwd.hpp"
#include "foedus/xct/xct_id.hpp"

namespace foedus {
namespace xct {

/**
 * @brief Represents a record of following a page pointer during a transaction.
 * @ingroup XCT
 * @details
 * This is used in two cases. First, when we follow a snapshot pointer because a volatile
 * pointer was null, then we add the address of the volatile pointer to this set
 * so that we can get aware of the new version at precommit time.
 * Second, if a page pointer can be swapped for some reason (usually only a root page),
 * we add the pointer even if we are following a volatile pointer.
 * @par POD
 * This is a POD struct. Default destructor/copy-constructor/assignment operator work fine.
 */
struct PointerAccess {
  friend std::ostream& operator<<(std::ostream& o, const PointerAccess& v);

  /** Address of the volatile pointer. */
  const storage::VolatilePagePointer* address_;

  /** Value of the volatile pointer as of the access. */
  storage::VolatilePagePointer        observed_;
};

/**
 * @brief Represents a record of reading a page during a transaction.
 * @ingroup XCT
 * @details
 * This is similar to PointerAccess. The difference is that this remembers the PageVersion
 * value we observed when we accessed the page. This can capture many more concurrency
 * issues in the page because PageVersion contains many flags and counters.
 * However, PageVersionAccess can't be used if the page itself might be swapped.
 *
 * Both PointerAccess and PageVersionAccess can be considered as "node set" in [TU2013], but
 * for a little bit different purpose.
 * @par POD
 * This is a POD struct. Default destructor/copy-constructor/assignment operator work fine.
 */
struct PageVersionAccess {
  friend std::ostream& operator<<(std::ostream& o, const PageVersionAccess& v);

  /** Address to the page version. */
  const storage::PageVersion* address_;

  /** Value of the page version as of the access. */
  storage::PageVersionStatus observed_;
};

/** Base of ReadXctAccess and WriteXctAccess. No virtual anything. POD. */
struct RecordXctAccess {
  /** The storage we accessed. */
  storage::StorageId    storage_id_;

  /**
   * Indicates the ordinal among ReadXctAccess/WriteXctAccess of this transaction.
   * For example, first log record has 0, second 1,..
   * Next transaction of this thread resets it to 0 at begin_xct.
   * This is an auxiliary field we can potentially re-calculate from lock_id_,
   * but it's tedious (it's not address order when it wraps around in log buffer).
   * So, we maintain it here for sanity checks.
   * Also used for stable sorting (in case lock_id_ is same) of read/write-sets.
   */
  uint32_t              ordinal_;

  /**
   * Pointer to the accessed record. We can calculate this from lock_id_,
   * this is here just for convenience.
   */
  RwLockableXctId*      owner_id_address_;

  /** Universal Lock ID of the lock in the record. */
  UniversalLockId       owner_lock_id_;

  /**
   * Setter for owner_id_address_/owner_lock_id_.
   * These two should be always set/updated together, so prefer this.
   */
  void set_owner_id_and_lock_id(RwLockableXctId* owner_id_address, UniversalLockId owner_lock_id) {
    owner_id_address_ = owner_id_address;
    owner_lock_id_ = owner_lock_id;
  }
  /**
   * Calculate owner_lock_id using the resolver. This one can't be inlined, though.
   * This is slightly wasteful if you already have lock_id calculated.
   */
  void set_owner_id_resolve_lock_id(
    const memory::GlobalVolatilePageResolver& resolver,
    RwLockableXctId* owner_id_address);

  /**
   * sort the write set in a unique order. We use UniversalLockId which contains the
   * records's NodeID and local VA offset. This is different from Silo [Tu '13] because
   * we need global ordering across all processes. VA's might give different per-process
   * results.
   */
  static bool compare(const RecordXctAccess &left, const RecordXctAccess& right) ALWAYS_INLINE;
};

/**
 * @brief Represents a record of read-access during a transaction.
 * @ingroup XCT
 * @details
 * @par POD
 * This is a POD struct. Default destructor/copy-constructor/assignment operator work fine.
 */
struct ReadXctAccess : public RecordXctAccess {
  friend std::ostream& operator<<(std::ostream& o, const ReadXctAccess& v);

  /** Transaction ID of the record observed as of the access. */
  XctId               observed_owner_id_;

  /**
   * An optional member that points to a write access related to this read.
   * For example, an insert in masstree consists of a read-access that verifies the
   * physical record and a write-access to actually install the record logically.
   * If some other thread logically installs the record between the read and write,
   * the xct must abort.
   * This member connects such "related" read-write pairs so that our commit protocol can utilize.
   */
  WriteXctAccess*     related_write_;

  /** @copydoc foedux::xct::RecordXctAccess::compare() */
  static bool compare(const ReadXctAccess &left, const ReadXctAccess& right) ALWAYS_INLINE {
    return RecordXctAccess::compare(left, right);
  }
};

/**
 * @brief Represents a record of write-access during a transaction.
 * @ingroup XCT
 * @details
 * @par POD
 * This is a POD struct. Default destructor/copy-constructor/assignment operator work fine.
 */
struct WriteXctAccess : public RecordXctAccess {
  friend std::ostream& operator<<(std::ostream& o, const WriteXctAccess& v);

  /** Pointer to the payload of the record. */
  char*                 payload_address_;

  /** Pointer to the log entry in private log buffer for this write opereation. */
  log::RecordLogType*   log_entry_;

  /** @see ReadXctAccess::related_write_ */
  ReadXctAccess*        related_read_;

  /** @copydoc foedux::xct::RecordXctAccess::compare() */
  static bool compare(const WriteXctAccess &left, const WriteXctAccess& right) ALWAYS_INLINE {
    return RecordXctAccess::compare(left, right);
  }
};

/**
 * @brief Represents a record of special read-access during a transaction
 * without any need for locking.
 * @ingroup XCT
 * @details
 * @par POD
 * Some storage type doesn't need locking for serializability (so far \ref SEQUENTIAL only)
 * For them, we record only a XctId in its metadata placed in control block.
 * It's almost same as the usual ReadXctAccess in terms of the information we store here and how
 * we verify that at precommit, but to clarify we separate it to this object.
 * Also, there is a small difference now: the owner_id_address_ in this object points
 * to a control block whereas usual ReadXctAccess assumes that it points to a valid data page
 * so that we can always convert to UniversalLockId.
 */
struct LockFreeReadXctAccess {
  friend std::ostream& operator<<(std::ostream& o, const LockFreeReadXctAccess& v);

  /** XID value we observed. */
  XctId               observed_owner_id_;

  /** The storage we accessed. */
  storage::StorageId  storage_id_;

  /** Pointer to the TID we protect against. */
  RwLockableXctId*    owner_id_address_;

  // no related-write in this object.
  // also, no sorting needed for this object.
};

/**
 * @brief Represents a record of special write-access during a transaction
 * without any need for locking.
 * @ingroup XCT
 * @details
 * Some storage type doesn't need locking for serializability (so far \ref SEQUENTIAL only)
 * For them, we maintain this write-set objects separated from WriteXctAccess.
 * We don't lock/unlock for these records, and we don't even have to remember what
 * we observed (actually, we don't even observe anything when we create this).
 * @par POD
 * This is a POD struct. Default destructor/copy-constructor/assignment operator work fine.
 */
struct LockFreeWriteXctAccess {
  friend std::ostream& operator<<(std::ostream& o, const LockFreeWriteXctAccess& v);

  /** The storage we accessed. */
  storage::StorageId    storage_id_;

  /** Pointer to the log entry in private log buffer for this write opereation. */
  log::RecordLogType*   log_entry_;

  // no need for compare method or storing version/record/etc. it's lock-free!
};

inline bool RecordXctAccess::compare(
  const RecordXctAccess& left,
  const RecordXctAccess& right) {
  if (left.owner_lock_id_ != right.owner_lock_id_) {
    return left.owner_lock_id_ < right.owner_lock_id_;
  } else {
    return left.ordinal_ < right.ordinal_;
  }
}

}  // namespace xct
}  // namespace foedus
#endif  // FOEDUS_XCT_XCT_ACCESS_HPP_
