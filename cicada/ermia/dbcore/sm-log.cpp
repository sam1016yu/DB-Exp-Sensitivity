#include "sm-log-impl.h"
#include "sm-log-offset.h"
#include "sm-oid.h"
#include "sm-oid-impl.h"
#include "sm-thread.h"
#include <cstring>

using namespace RCU;

sm_log *logmgr = NULL;
bool sm_log::need_recovery = false;

uint64_t
sm_log::persist_log_buffer()
{
    /** dummy. FIXME(tzwang) **/
    return get_impl(this)->_lm.smallest_tls_lsn_offset();
}

void
sm_log::set_tls_lsn_offset(uint64_t offset)
{
    get_impl(this)->_lm.set_tls_lsn_offset(offset);
}

uint64_t
sm_log::get_tls_lsn_offset()
{
    return get_impl(this)->_lm.get_tls_lsn_offset();
}

LSN
sm_log::flush()
{
    return get_impl(this)->_lm.flush();
}

void
sm_log::update_chkpt_mark(LSN cstart, LSN cend)
{
    get_impl(this)->_lm._lm.update_chkpt_mark(cstart, cend);
}

void
sm_log::load_object(char *buf, size_t bufsz, fat_ptr ptr, size_t align_bits)
{
    get_impl(this)->_lm._lm.load_object(buf, bufsz, ptr, align_bits);
}

fat_ptr
sm_log::load_ext_pointer(fat_ptr ptr)
{
    return get_impl(this)->_lm._lm.load_ext_pointer(ptr);
}


sm_log *
sm_log::new_log(sm_log_recover_impl *recover_functor, void *rarg)
{
    need_recovery = false;
    if (sysconf::null_log_device) {
      dirent_iterator iter(sysconf::log_dir.c_str());
      for (char const *fname : iter) {
        if (strcmp(fname, ".") and strcmp(fname, ".."))
          os_unlinkat(iter.dup(), fname);
      }
    }
    ALWAYS_ASSERT(sysconf::log_segment_mb);
    ALWAYS_ASSERT(sysconf::log_buffer_mb);
    return new sm_log_impl(recover_functor, rarg);
}

sm_log_scan_mgr *
sm_log::get_scan_mgr()
{
    return get_impl(this)->_lm._lm.scanner;
}

sm_tx_log *
sm_log::new_tx_log()
{
    auto *self = get_impl(this);
    typedef _impl_of<sm_tx_log>::type Impl;
    return new (Impl::alloc_storage()) Impl(self);
}

fat_ptr
sm_log_impl::lsn2ptr(LSN lsn, bool is_ext) {
    return get_impl(this)->_lm._lm.lsn2ptr(lsn, is_ext);
}

LSN
sm_log_impl::ptr2lsn(fat_ptr ptr) {
    return _lm._lm.ptr2lsn(ptr);
}

LSN
sm_log::cur_lsn()
{
    auto *log = &get_impl(this)->_lm;
    auto offset = log->cur_lsn_offset();
    auto *sid = log->_lm.get_offset_segment(offset);

	if (not sid) {
		/* must have raced a new segment opening */
		/*
		while (1) {
			sid = log->_lm._newest_segment();
			if (sid->start_offset >= offset)
				break;
		}
		*/

retry:
		sid = log->_lm._newest_segment();
		ASSERT(sid);
		if (offset < sid->start_offset)
			offset = sid->start_offset;
		else if (sid->end_offset <= offset) {
			    goto retry; 
		}
	}
    return sid->make_lsn(offset);
}

LSN
sm_log::durable_flushed_lsn()
{
    auto *log = &get_impl(this)->_lm;
    auto offset = log->dur_flushed_lsn_offset();
    auto *sid = log->_lm.get_offset_segment(offset);
    ASSERT(sid);
    return sid->make_lsn(offset);
}

void
sm_log::wait_for_durable_flushed_lsn_offset(uint64_t offset)
{
    auto *self = get_impl(this);
    self->_lm.wait_for_durable(offset);
}
