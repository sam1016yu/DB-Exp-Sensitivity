#define CONFIG_H "silo/config/config-perf.h"
#include "silo/rcu.h"
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <sched.h>
#include "global.h"
#include "manager.h"
#include "thread.h"
#include "txn.h"
#include "wl.h"
#include "query.h"
#include "plock.h"
#include "occ.h"
#include "vll.h"
#include "ycsb_query.h"
#include "tpcc_query.h"
#include "mem_alloc.h"
#include "test.h"

#if CC_ALG == MICA || MICA_USE_FIXED_BACKOFF

// The default backoff scheme is very slow.
#define DISABLE_BUILTIN_BACKOFF

#endif


void thread_t::init(uint64_t thd_id, workload * workload) {
	_thd_id = thd_id;
	_wl = workload;
	srand48_r((_thd_id + 1) * get_sys_clock(), &buffer);
	_abort_buffer_size = ABORT_BUFFER_SIZE;
	_abort_buffer = (AbortBufferEntry *) mem_allocator.alloc(sizeof(AbortBufferEntry) * _abort_buffer_size, thd_id);
	for (int i = 0; i < _abort_buffer_size; i++)
		_abort_buffer[i].query = NULL;
	_abort_buffer_empty_slots = _abort_buffer_size;
	_abort_buffer_enable = (g_params["abort_buffer_enable"] == "true");
}

uint64_t thread_t::get_thd_id() { return _thd_id; }
uint64_t thread_t::get_host_cid() {	return _host_cid; }
void thread_t::set_host_cid(uint64_t cid) { _host_cid = cid; }
uint64_t thread_t::get_cur_cid() { return _cur_cid; }
void thread_t::set_cur_cid(uint64_t cid) {_cur_cid = cid; }

RC thread_t::run() {
#if !NOGRAPHITE
	_thd_id = CarbonGetTileId();
#endif
	pthread_barrier_wait( &warmup_bar );
	stats.init(get_thd_id());
	pthread_barrier_wait( &warmup_bar );

#if CC_ALG == MICA
  ::mica::util::lcore.pin_thread(get_thd_id());

  // This activation is blocking, so this must be done after warmup_bar above.
	_wl->mica_db->activate(static_cast<uint16_t>(get_thd_id()));
	while (_wl->mica_db->active_thread_count() < g_thread_cnt) {
		PAUSE;
		_wl->mica_db->idle(static_cast<uint16_t>(get_thd_id()));
	}

  MICATiming t(_wl->mica_db->context(get_thd_id())->timing_stack(), &::mica::transaction::Stats::worker);
#else
	set_affinity(get_thd_id());
#endif

	// if (warmup_finish) {
		mem_allocator.register_thread(get_thd_id());
	// }

	pthread_barrier_wait( &start_bar );

	myrand rdm;
	rdm.init(get_thd_id());
	RC rc = RCOK;
	txn_man * m_txn;
	rc = _wl->get_txn_man(m_txn, this);
	assert (rc == RCOK);
	glob_manager->set_txn_man(m_txn);

	base_query * m_query = NULL;
	uint64_t thd_txn_id = 0;
	UInt64 txn_cnt = 0;

#if CC_ALG != MICA
  ts_t last_commit_time = 0;
#endif

	uint64_t exp_endtime;
	if (!warmup_finish)
	  exp_endtime = get_server_clock() + static_cast<uint64_t>(MAX_WARMUP_DURATION * 1000000000.);
	else
	  exp_endtime = get_server_clock() + static_cast<uint64_t>(MAX_TXN_DURATION * 1000000000.);

	while (true) {
		// ts_t starttime = get_sys_clock();
		if (WORKLOAD != TEST) {
#ifndef DISABLE_BUILTIN_BACKOFF
			int trial = 0;
			if (_abort_buffer_enable) {
				m_query = NULL;
				while (trial < 2) {
					//ts_t curr_time = get_sys_clock();
					ts_t curr_time = get_server_clock();
					ts_t min_ready_time = UINT64_MAX;
					if (_abort_buffer_empty_slots < _abort_buffer_size) {
						for (int i = 0; i < _abort_buffer_size; i++) {
							if (_abort_buffer[i].query != NULL && curr_time > _abort_buffer[i].ready_time) {
								m_query = _abort_buffer[i].query;
								_abort_buffer[i].query = NULL;
								_abort_buffer_empty_slots ++;
								break;
							} else if (_abort_buffer_empty_slots == 0
									  && _abort_buffer[i].ready_time < min_ready_time)
								min_ready_time = _abort_buffer[i].ready_time;
						}
					}
					if (m_query == NULL && _abort_buffer_empty_slots == 0) {
						assert(trial == 0);
						M_ASSERT(min_ready_time >= curr_time, "min_ready_time=%ld, curr_time=%ld\n", min_ready_time, curr_time);
						usleep(min_ready_time - curr_time);
					}
					else if (m_query == NULL)
						m_query = query_queue->get_next_query( _thd_id );
					if (m_query != NULL)
						break;
				}
			} else {
				if (rc == RCOK)
					m_query = query_queue->get_next_query( _thd_id );
			}
#else
		if (m_query == nullptr)
			m_query = query_queue->get_next_query( _thd_id );
#endif
		}
		// INC_STATS(_thd_id, time_query, get_sys_clock() - starttime);
		m_txn->abort_cnt = 0;
//#if CC_ALG == VLL
//		_wl->get_txn_man(m_txn, this);
//#endif
		m_txn->set_txn_id(get_thd_id() + thd_txn_id * g_thread_cnt);
		thd_txn_id ++;

		if ((CC_ALG == HSTORE && !HSTORE_LOCAL_TS)
				|| CC_ALG == MVCC
				|| CC_ALG == HEKATON
				|| CC_ALG == TIMESTAMP)
			m_txn->set_ts(get_next_ts());

		rc = RCOK;
#if CC_ALG == HSTORE
		if (WORKLOAD == TEST) {
			uint64_t part_to_access[1] = {0};
			rc = part_lock_man.lock(m_txn, &part_to_access[0], 1);
		} else
			rc = part_lock_man.lock(m_txn, m_query->part_to_access, m_query->part_num);
#elif CC_ALG == VLL
		vll_man.vllMainLoop(m_txn, m_query);
#elif CC_ALG == MVCC || CC_ALG == HEKATON
		glob_manager->add_ts(get_thd_id(), m_txn->get_ts());
#elif CC_ALG == OCC
		// In the original OCC paper, start_ts only reads the current ts without advancing it.
		// But we advance the global ts here to simplify the implementation. However, the final
		// results should be the same.
		m_txn->start_ts = get_next_ts();
#endif
		if (rc == RCOK)
		{
#if RCU_ALLOC || INDEX_STRUCT != IDX_MICA
		  scoped_rcu_region guard;
#endif

#if CC_ALG != VLL
			if (WORKLOAD == TEST)
				rc = runTest(m_txn);
			else
				rc = m_txn->run_txn(m_query);
#endif
#if CC_ALG == HSTORE
			if (WORKLOAD == TEST) {
				uint64_t part_to_access[1] = {0};
				part_lock_man.unlock(m_txn, &part_to_access[0], 1);
			} else
				part_lock_man.unlock(m_txn, m_query->part_to_access, m_query->part_num);
#endif
		}
		if (rc == Abort) {
#ifndef DISABLE_BUILTIN_BACKOFF
			uint64_t penalty = 0;
			if (ABORT_PENALTY != 0)  {
				double r;
				drand48_r(&buffer, &r);
				penalty = r * ABORT_PENALTY;
			}
			if (!_abort_buffer_enable)
				usleep(penalty / 1000);
			else {
				assert(_abort_buffer_empty_slots > 0);
				for (int i = 0; i < _abort_buffer_size; i ++) {
					if (_abort_buffer[i].query == NULL) {
						_abort_buffer[i].query = m_query;
						//_abort_buffer[i].ready_time = get_sys_clock() + penalty;
						_abort_buffer[i].ready_time = get_server_clock() + penalty;
						_abort_buffer_empty_slots --;
						break;
					}
				}
			}
#endif

#if CC_ALG != MICA && MICA_USE_FIXED_BACKOFF
      uint64_t us = _wl->mica_sw.c_1_usec();
			double max_backoff_time = MICA_FIXED_BACKOFF * static_cast<double>(us);

			double r;
			drand48_r(&buffer, &r);

			uint64_t now = _wl->mica_sw.now();
			uint64_t ready = now + static_cast<uint64_t>(max_backoff_time * r);

			while (ready > now) {
				PAUSE;
				now = _wl->mica_sw.now();
			}
#endif
		}

#ifdef DISABLE_BUILTIN_BACKOFF
	if (rc == RCOK && m_query != NULL) {
#if WORKLOAD == TPCC && TPCC_SPLIT_DELIVERY
		if (++m_query->sub_query_id != m_query->max_sub_query_id)
			continue;
#endif
		m_query = nullptr;
	}
#endif

#if WORKLOAD == TPCC && TPCC_SPLIT_DELIVERY
	if (rc == RCOK && m_query != NULL)
		if (++m_query->sub_query_id != m_query->max_sub_query_id) {
			for (int i = 0; i < _abort_buffer_size; i ++) {
				if (_abort_buffer[i].query == NULL) {
					_abort_buffer[i].query = m_query;
					_abort_buffer[i].ready_time = get_server_clock();
					_abort_buffer_empty_slots --;
					break;
				}
			}
			continue;
		}
#endif

		// ts_t endtime = get_sys_clock();
		// uint64_t timespan = endtime - starttime;
		// INC_STATS(get_thd_id(), run_time, timespan);
		// INC_STATS(get_thd_id(), latency, timespan);
		//stats.add_lat(get_thd_id(), timespan);
		if (rc == RCOK) {
			INC_STATS(get_thd_id(), txn_cnt, 1);
			stats.commit(get_thd_id());
			txn_cnt ++;

#if CC_ALG != MICA
      ts_t now = get_server_clock();
      if (last_commit_time != 0)
        inter_commit_latency.update((now - last_commit_time) / 1000);
      // printf("%" PRIu64 "\n", (now - last_commit_time) / 1000);
      last_commit_time = now;
#endif
		} else if (rc == Abort) {
			// INC_STATS(get_thd_id(), time_abort, timespan);
			INC_STATS(get_thd_id(), abort_cnt, 1);
			stats.abort(get_thd_id());
			m_txn->abort_cnt ++;
		}

		if (rc == FINISH) {
#if CC_ALG == MICA
    	_wl->mica_db->deactivate(static_cast<uint16_t>(get_thd_id()));
#endif
			return rc;
    }
		// if (!warmup_finish && txn_cnt >= WARMUP / g_thread_cnt)
		if (!warmup_finish && (txn_cnt >= WARMUP || static_cast<int64_t>(exp_endtime - get_server_clock()) <= 0))
		{
			stats.clear( get_thd_id() );
#if CC_ALG == MICA
    	_wl->mica_db->deactivate(static_cast<uint16_t>(get_thd_id()));
#endif
			return FINISH;
		}

		if (warmup_finish && (txn_cnt >= MAX_TXN_PER_PART || static_cast<int64_t>(exp_endtime - get_server_clock()) <= 0)) {
			// assert(txn_cnt == MAX_TXN_PER_PART);
	        if( !ATOM_CAS(_wl->sim_done, false, true) )
				assert( _wl->sim_done);
	    }
	    if (_wl->sim_done) {
#if CC_ALG == MICA
        	_wl->mica_db->deactivate(static_cast<uint16_t>(get_thd_id()));
#endif
   		    return FINISH;
   		}
	}
	assert(false);
}


ts_t
thread_t::get_next_ts() {
#if CC_ALG == MICA
	printf("oops\n");
	assert(false);
#endif
	if (g_ts_batch_alloc) {
		if (_curr_ts % g_ts_batch_num == 0) {
			_curr_ts = glob_manager->get_ts(get_thd_id());
			_curr_ts ++;
		} else {
			_curr_ts ++;
		}
		return _curr_ts - 1;
	} else {
		_curr_ts = glob_manager->get_ts(get_thd_id());
		return _curr_ts;
	}
}

RC thread_t::runTest(txn_man * txn)
{
	RC rc = RCOK;
	if (g_test_case == READ_WRITE) {
		rc = ((TestTxnMan *)txn)->run_txn(g_test_case, 0);
#if CC_ALG == OCC
		txn->start_ts = get_next_ts();
#endif
		rc = ((TestTxnMan *)txn)->run_txn(g_test_case, 1);
		printf("READ_WRITE TEST PASSED\n");
		return FINISH;
	}
	else if (g_test_case == CONFLICT) {
		rc = ((TestTxnMan *)txn)->run_txn(g_test_case, 0);
		if (rc == RCOK)
			return FINISH;
		else
			return rc;
	}
	assert(false);
	return RCOK;
}
