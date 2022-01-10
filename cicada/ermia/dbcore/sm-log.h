// -*- mode:c++ -*-
#ifndef __SM_LOG_H
#define __SM_LOG_H

/* A high-performance log manager for an append-only system.

   This is a redo-only log, as it records only committed changes
   (*). Actually, it's not even much of a redo log; really the system
   just has to bring the OID array back up to date from the most
   recent checkpoint.

   (*) Uncommitted changes can be logged, but are recorded in such a
   way that they will be ignord during reply unless their owning
   transaction commits.

 */
#include <unordered_map>
#include "sm-common.h"
#include "sm-thread.h"
#include "window-buffer.h"

class ndb_ordered_index;
class object;
class sm_log_file_mgr;
class segment_id;
class sm_log_recover_impl;

struct sm_tx_log {
    /* Record an insertion. The payload of the version will be
       embedded in the log record on disk and the version's
       [disk_addr] will be set accordingly. The target OID will be
       re-allocated during recovery.

       The parameters [f] and [o] identify the record, whose contents
       are the payload of the version stored at [p]. In order to
       simplify the implementation, [psize] specifies the size of the
       record payload, which should *not* include the (volatile)
       version header information and which should be consistent with
       the encoded size embedded in [p].

       If [pdest] is non-NULL, the pointed-to location will be set to
       the record's location on disk. That assignment may not occur
       until a call to commit() or pre_commit(), so the pointer must
       remain valid at least that long. The pointer would normally
       reference version::disk_addr of the version this log record
       corresponds to, in which case lifetime requirements are met.

       WARNING: The caller cannot assume a record is durable just
       because it has been assigned a location.
    */
    void log_insert(FID f, OID o, fat_ptr p, int abits, fat_ptr *pdest);

    /* Record an insert to the index. p stores a pointer to the key value
     */
    void log_insert_index(FID f, OID o, fat_ptr p, int abits, fat_ptr *pdest);
    
    /* Record an update. Like an insertion, except that the OID is
       assumed to already have been allocated.
     */
    void log_update(FID f, OID o, fat_ptr p, int abits, fat_ptr *pdest);

    /* Record a change in a record's on-disk location, to the address
       indicated. The OID remains the same and the data for the new
       location is already durable. Unlike an insertion or update, the
       new version's contents are not logged (being already durable).
    */
    void log_relocate(FID f, OID o, fat_ptr p, int abits);

    /* Record a deletion. During recovery, the OID slot is cleared and
       the OID deallocated.
    */
    void log_delete(FID f, OID o);

    /* Record the creation of a table with FID and name
     */
    void log_fid(FID f, const std::string &name);

    /* Return this transaction's commit LSN, or INVALID_LSN if the
       transaction has not entered pre-commit yet.

       This function should be called by a transaction which already
       has a CLSN, and which wishes to determine whether it committed
       before or after this one. The implementation deals specifically
       with the race where the owner has acquired a CLSN but not yet
       published it, *and* where that CLSN is earlier than the one
       belonging to the caller.
     */
    LSN get_clsn();
    
    /* Acquire and return a commit LSN, but do not write the
       corresponding log records to disk yet. This function can safely
       be called multiple times to retrieve an existing commit LSN.

       NOTE: the commit LSN actually points past-end of the commit
       block, in keeping with cur_lsn and durable_flushed_lsn (which
       respectively identify the first LSN past-end of any currently
       in use, and the first LSN that is not durable).

       WARNING: log records cannot be added to the transaction after
       this call returns.
    */
    LSN pre_commit();

    /* Pre-commit succeeded. Log record(s) for this transaction can
       safely be made durable. Return the commit LSN. If [pdest] is
       non-NULL, fill it with the on-disk location of the commit
       block.

       It is not necessary to have called pre_commit first.

       NOTE: the transaction will not actually be durable until
       sm_log::durable_flushed_lsn catches up to the pre_commit LSN.

       WARNING: By calling this function, the caller gives up
       ownership of this object and should not access it again.
     */
    LSN commit(LSN *pdest);

    /* Transaction failed (perhaps even before pre-commit). Discard
       all log state and do not write anything to disk. 

       WARNING: By calling this function, the caller gives up
       ownership of this object and should not access it again.
     */
    void discard();

protected:
    // Forbid direct instantiation
    sm_tx_log() { }
};

/* A factory class for creating log scans.

   These scans form the basis of recovery, and can be used before
   normal log functions are available.
 */
struct sm_log_scan_mgr {
    static size_t const NO_PAYLOAD = -1;
    
    enum record_type { LOG_INSERT, LOG_INSERT_INDEX, LOG_UPDATE,
                       LOG_RELOCATE, LOG_DELETE, LOG_CHKPT, LOG_FID };

    /* A cursor for iterating over log records, whether those of a single
       transaction or all which follow some arbitrary starting point.
    */
    struct record_scan {
        /* Query whether the cursor currently rests on a valid record */
        bool valid();

        /* Advance to the next record */
        void next();

        /* Return the type of record */
        record_type type();
    
        /* Return the FID and OID the current record refers to */
        FID fid();
        OID oid();

        /* Return the size of the payload, or NO_PAYLOAD if the log record
           has no payload.

           NOTE: this function returns the size of the actual object, even
           for external/reloc records where the log record's "payload" is
           technically a pointer to the actual object.
        */
        size_t payload_size();
    
        /* Return a pointer to the payload, or NULL_PTR if the log record
           has no payload.

           NOTE: this function returns the pointer to the actual object,
           even for external/reloc records where the log record's
           "payload" is technically a pointer to the actual object.
        */
        fat_ptr payload_ptr();

        LSN payload_lsn();

        /* Copy the current record's payload into [buf]. Throw
           illegal_argument if the record has no payload, or the payload
           is larger than [bufsz], or the record does not reside in the
           log.

           NOTE: this function is usually more efficient than
           sm_log::load_object, because the scanner probably loaded the
           payload into memory already as part of its normal operations.
        */
        void load_object(char *buf, size_t bufsz);
    
        virtual ~record_scan() { }
    
    protected:
        // forbid direct instantiation
        record_scan() { }
    };

    /* Similar to record_scan, but it does *not* fetch payloads.

       Fetching only headers can reduce the I/O bandwidth requirements of
       the scan by anywhere from 50% to well over 99%, depending on the
       sizes of log records involved). However, it generates a random
       access pattern that will perform poorly on spinning platters. It
       also means that all payloads must be fetched manually at a later
       time, and any log record not stored directly in the log will
       require a second I/O to fetch. These indirect pointers have type
       ASI_EXT rather than ASI_LOG or ASI_HEAP, and must be dereferenced
       by a call to sm_log::load_ptr. Indirect pointers do encode the
       proper object size, however, so buffer space can be allocated
       before requesting any I/O.
    */
    struct header_scan {
        /* Query whether the cursor currently rests on a valid record */
        bool valid();

        /* Advance to the next record */
        void next();

        /* Return the type of record */
        record_type type();
    
        /* Return the FID and OID the current record refers to */
        FID fid();
        OID oid();

        /* Return the size of the payload, or NO_PAYLOAD if the log record
           has no payload.

           NOTE: this function returns the size of the actual object, even
           for external/reloc records where the log record's "payload" is
           technically a pointer to the actual object.
        */
        size_t payload_size();
    
        /* Return a pointer to the payload, or NULL_PTR if the log record
           has no payload. If [follow_ext] is set and the pointer is
           ASI_EXT, dereference it (implying an I/O operation). Otherwise,
           return the raw pointer, regardless of its type.

           NOTE: a result of type ASI_EXT means it is necessary to
           dereference the pointer to find the true location of the
           record's payload.
        */
        fat_ptr payload_ptr(bool follow_ext=false);

        /* Attempt to copy the current record's payload into [buf]. Throw
           illegal_argument if the record has no payload, or the payload
           is larger than [bufsz].

           If the record resides in the log, load it; if the record
           payload is ASI_EXT, dereference the pointer (placing the result
           in [pdest]); if the result is ASI_LOG, fetch it as well and
           return true, otherwise (e.g. ASI_HEAP) return false.

           NOTE: this function is provided as a convenience, but is no
           more efficient than sm_log::load_object.
        */
        bool load_object(fat_ptr &pdest, char *buf, size_t bufsz);

        virtual ~header_scan() { }

    protected:
        // forbid direct instantiation
        header_scan() { }
    };

    /* Start scanning log headers from [start], stopping at
       end-of-log. Record payloads are not available, and must be
       loaded manually if desired.
     */
    header_scan *new_header_scan(LSN start);
    
    /* Start scanning the log from [start], stopping only when
       end-of-log is encountered. Record payloads are available.
     */
    record_scan *new_log_scan(LSN start, bool fetch_payloads);

    /* Start scanning log entries for the transaction whose commit
       record resides at [start]. Stop when all records for the
       transaction have been visited. Record payloads are available.
     */
    record_scan *new_tx_scan(LSN start);

    /* Load the object referenced by [ptr] from the log. The pointer
       must reference the log (ASI_LOG) and the given buffer must be large
       enough to hold the object.
     */
    void load_object(char *buf, size_t bufsz, fat_ptr ptr, size_t align_bits=DEFAULT_ALIGNMENT_BITS);

    /* Retrieve the address of an externalized log record payload.

       The pointer must be external (ASI_EXT).
     */
    fat_ptr load_ext_pointer(fat_ptr ptr);

    virtual ~sm_log_scan_mgr() { }
    
protected:
    // forbid direct instantiation
    sm_log_scan_mgr() { }
};

/* The owner of the log has complete control over what it means to
   recover. A function of this signature (passed to the log manager's
   constructor) is called after the log end has been verified and
   before forward processing begins.
*/
typedef void sm_log_recover_function(void *arg, sm_log_scan_mgr *scanner,
                                     LSN chkpt_begin, LSN chkpt_end);

struct sm_log {
    static bool need_recovery;

    void update_chkpt_mark(LSN cstart, LSN cend);
    LSN flush();
    void set_tls_lsn_offset(uint64_t offset);
    uint64_t get_tls_lsn_offset();

    /* Allocate and return a new sm_log object. If [dname] exists, it
       will be mounted and used. Otherwise, a new (empty) log
       directory will be created.
     */
    static
    sm_log *new_log(sm_log_recover_impl *recover_functor, void *rarg);

    /* Return a pointer to the log's scan manager.

       The caller should *not* delete it when finished.
     */
    sm_log_scan_mgr* get_scan_mgr();
    
    /* Allocate a new transaction log tracker. All logging occurs
       through this interface.

       WARNING: the caller is responsible to eventually call commit()
       or discard() on the returned object, or risk stalling the log.
     */
    sm_tx_log* new_tx_log();

    /* Return the current LSN. This is the LSN that the next
       successful call to allocate() will acquire.
     */
    LSN cur_lsn();

    /* Return the current durable LSN. This is the LSN before which
       all log records are known to have reached stable storage; any
       LSN at or beyond this point may not be durable yet. If
       cur_lsn() == durable_flushed_lsn(), all log records are durable.
     */
    LSN durable_flushed_lsn();

    /* Block the calling thread until durable_flushed_lsn() is not smaller
       than [dlsn]. This will not occur until all log_allocation
       objects with LSN smaller than [dlsn] have been released or
       discarded.
     */
    void wait_for_durable_flushed_lsn_offset(uint64_t offset);

    /* Load the object referenced by [ptr] from the log. The pointer
       must reference the log (ASI_LOG) and the given buffer must be large
       enough to hold the object.
     */
    void load_object(char *buf, size_t bufsz, fat_ptr ptr, size_t align_bits=DEFAULT_ALIGNMENT_BITS);

    /* Retrieve the address of an externalized log record payload.

       The pointer must be external (ASI_EXT).
     */
    fat_ptr load_ext_pointer(fat_ptr ptr);

    window_buffer &get_logbuf();
    segment_id *assign_segment(uint64_t lsn_begin, uint64_t lsn_end);
    uint64_t persist_log_buffer();
    segment_id *flush_log_buffer(window_buffer &logbuf, uint64_t new_dlsn_offset, bool update_dmark);
    void redo_log(LSN start_lsn, LSN end_lsn);
    void enqueue_committed_xct(uint32_t worker_id, uint64_t start_time);

    virtual ~sm_log() { }

protected:
    // Forbid direct instantiation
    sm_log() { }
};

extern sm_log *logmgr;
#endif
