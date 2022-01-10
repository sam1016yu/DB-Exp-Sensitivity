#include "global.h"
#include "ycsb.h"
#include "tpcc.h"
#include "tatp.h"
#include "test.h"
#include "thread.h"
#include "manager.h"
#include "mem_alloc.h"
#include "query.h"
#include "plock.h"
#include "occ.h"
#include "vll.h"
#include "table.h"
#if INDEX_STRUCT == IDX_MICA
#include "index_mica.h"
#endif
#include <thread>

void* f(void*);

thread_t** m_thds;

// defined in parser.cpp
void parser(int argc, char* argv[]);

int main(int argc, char* argv[]) {
  parser(argc, argv);

  // Init workload part 1
  workload* m_wl;
  switch (WORKLOAD) {
    case YCSB:
      m_wl = new ycsb_wl;
      break;
    case TPCC:
      m_wl = new tpcc_wl;
      break;
    case TATP:
      m_wl = new tatp_wl;
      break;
    case TEST:
      m_wl = new TestWorkload;
      ((TestWorkload*)m_wl)->tick();
      break;
    default:
      assert(false);
  }

  // Init MICA
#if CC_ALG == MICA
  ::mica::util::lcore.pin_thread(0);
#endif

  m_wl->init_mica();

  // Init memory allocator
  mem_allocator.init(g_part_cnt, MEM_SIZE / g_part_cnt);
  mem_allocator.register_thread(0);

  stats.init();
  glob_manager = (Manager*)mem_allocator.alloc(sizeof(Manager), 0);
  glob_manager->init();
  if (g_cc_alg == DL_DETECT) dl_detector.init();
  printf("mem_allocator initialized!\n");

  // Init workload part 2
  m_wl->init();
  printf("workload initialized!\n");

#if CC_ALG == MICA
  {
    std::vector<std::thread> threads;
    for (uint64_t thread_id = 0; thread_id < g_thread_cnt; thread_id++) {
      threads.emplace_back([&, thread_id] {
        ::mica::util::lcore.pin_thread(thread_id);
        mem_allocator.register_thread(thread_id);

        m_wl->mica_db->activate(thread_id);

        for (auto it : m_wl->tables) {
          auto table = it.second;
          uint64_t part_id = 0;
          for (auto mica_tbl : table->mica_tbl) {
            if ((part_id++) % g_thread_cnt != thread_id) continue;

            uint64_t i = 0;
            mica_tbl->renew_rows(m_wl->mica_db->context(thread_id), 0, i,
                                 static_cast<uint64_t>(-1), false);

            if (thread_id == 0) {
              printf("thread %2" PRIu64 ": table %s part%2" PRIu64 ":\n",
                     thread_id, it.first.c_str(), part_id - 1);
              mica_tbl->print_table_status();
            }
          }
        }

#if INDEX_STRUCT == IDX_MICA
        for (auto it : m_wl->hash_indexes) {
          auto index = it.second;
          uint64_t part_id = 0;
          for (auto idx : index->mica_idx) {
            if ((part_id++) % g_thread_cnt != thread_id) continue;

            auto mica_tbl = idx->index_table();

            uint64_t i = 0;
            mica_tbl->renew_rows(m_wl->mica_db->context(thread_id), 0, i,
                                 static_cast<uint64_t>(-1), false);

            if (thread_id == 0) {
              printf("thread %2" PRIu64 ": index %s part %2" PRIu64 ":\n",
                     thread_id, it.first.c_str(), part_id - 1);
              mica_tbl->print_table_status();
            }
          }
        }
#ifndef IDX_MICA_USE_MBTREE
        for (auto it : m_wl->ordered_indexes) {
          auto index = it.second;
          uint64_t part_id = 0;
          for (auto idx : index->mica_idx) {
            if ((part_id++) % g_thread_cnt != thread_id) continue;

            auto mica_tbl = idx->index_table();

            uint64_t i = 0;
            mica_tbl->renew_rows(m_wl->mica_db->context(thread_id), 0, i,
                                 static_cast<uint64_t>(-1), false);

            if (thread_id == 0) {
              printf("thread %2" PRIu64 ": index %s part %2" PRIu64 ":\n",
                     thread_id, it.first.c_str(), part_id - 1);
              mica_tbl->print_table_status();
            }
          }
        }
#endif
#endif

        m_wl->mica_db->deactivate(thread_id);
      });
    }
    while (threads.size() > 0) {
      threads.back().join();
      threads.pop_back();
    }

    printf("MICA tables renewed\n");
  }
#endif

  uint64_t thd_cnt = g_thread_cnt;
  pthread_t p_thds[thd_cnt - 1];
  m_thds = new thread_t*[thd_cnt];
  for (uint32_t i = 0; i < thd_cnt; i++)
    m_thds[i] = (thread_t*)mem_allocator.alloc(sizeof(thread_t), 0);
  // query_queue should be the last one to be initialized!!!
  // because it collects txn latency
  query_queue = (Query_queue*)mem_allocator.alloc(sizeof(Query_queue), 0);
  if (WORKLOAD != TEST) query_queue->init(m_wl);
  pthread_barrier_init(&warmup_bar, NULL, g_thread_cnt);
  printf("query_queue initialized!\n");
#if CC_ALG == HSTORE
  part_lock_man.init();
#elif CC_ALG == OCC
  occ_man.init();
#elif CC_ALG == VLL
  vll_man.init();
#endif

  fprintf(stderr, "mem_allocator stats after workload init:\n");
  mem_allocator.dump_stats();

  pthread_barrier_init(&start_bar, NULL, g_thread_cnt + 1);

  for (uint32_t i = 0; i < thd_cnt; i++) m_thds[i]->init(i, m_wl);

#if CC_ALG == MICA
  m_wl->mica_db->reset_stats();
  m_wl->mica_db->reset_backoff();
#endif

  if (WARMUP > 0) {
    printf("WARMUP start!\n");
    // for (uint32_t i = 0; i < thd_cnt - 1; i++) {
    // 	uint64_t vid = i;
    // 	pthread_create(&p_thds[i], NULL, f, (void *)vid);
    // }
    // f((void *)(thd_cnt - 1));
    // for (uint32_t i = 0; i < thd_cnt - 1; i++)
    // 	pthread_join(p_thds[i], NULL);
    for (uint32_t i = 0; i < thd_cnt; i++) {
      uint64_t vid = i;
      pthread_create(&p_thds[i], NULL, f, (void*)vid);
    }
    pthread_barrier_wait(&start_bar);
    for (uint32_t i = 0; i < thd_cnt; i++) pthread_join(p_thds[i], NULL);
    printf("WARMUP finished!\n");
  }
  warmup_finish = true;
  pthread_barrier_init(&warmup_bar, NULL, g_thread_cnt);
#ifndef NOGRAPHITE
  CarbonBarrierInit(&enable_barrier, g_thread_cnt);
#endif
  pthread_barrier_init(&warmup_bar, NULL, g_thread_cnt);

  pthread_barrier_init(&start_bar, NULL, g_thread_cnt + 1);

  fprintf(stderr, "mem_allocator stats after warmup:\n");
  mem_allocator.dump_stats();

#if CC_ALG == MICA
  m_wl->mica_db->reset_stats();
#else
  for (uint32_t i = 0; i < thd_cnt; i++)
    m_thds[i]->inter_commit_latency.reset();
#endif

  // spawn and run txns again.
  // int ret = system("perf record -a -o perf.data sleep 1 &");
  // int ret = system("perf record -a -o perf.data -g sleep 1 &");
  // int ret = system("perf record -a -o perf.data -g -e LLC-load-misses sleep 1 &");
  // int ret = system("perf record -C 0 -o perf.data sleep 1 &");
  // int ret = system("perf record -C 1 -o perf.data sleep 1 &");
  // int ret = system("perf stat -a -e cycles,instructions,cache-references,cache-misses,branches,branch-misses,stalled-cycles-frontend,L1-dcache-load-misses,L1-dcache-store-misses,L1-icache-load-misses,LLC-loads,LLC-load-misses,LLC-stores,LLC-store-misses,dTLB-load-misses,dTLB-store-misses,iTLB-loads,iTLB-load-misses,node-stores,node-load-misses,node-stores,node-store-misses sleep 1 &");
  // int ret = system("perf stat -C 1 sleep 1 &");
  // int ret = system("perf stat -C 2 sleep 1 &");
  // (void)ret;

  // int64_t starttime = get_server_clock();
  // for (uint32_t i = 0; i < thd_cnt - 1; i++) {
  // 	uint64_t vid = i;
  // 	pthread_create(&p_thds[i], NULL, f, (void *)vid);
  // }
  // f((void *)(thd_cnt - 1));
  // for (uint32_t i = 0; i < thd_cnt - 1; i++)
  // 	pthread_join(p_thds[i], NULL);
  // int64_t endtime = get_server_clock();

  for (uint32_t i = 0; i < thd_cnt; i++) {
    uint64_t vid = i;
    pthread_create(&p_thds[i], NULL, f, (void*)vid);
  }
  pthread_barrier_wait(&start_bar);
  int64_t starttime = get_server_clock();
  for (uint32_t i = 0; i < thd_cnt; i++) pthread_join(p_thds[i], NULL);
  int64_t endtime = get_server_clock();

  if (WORKLOAD != TEST) {
    printf("PASS! SimTime = %ld\n", endtime - starttime);
    if (STATS_ENABLE) stats.print((double)(endtime - starttime) / 1000000000.);
  } else {
    ((TestWorkload*)m_wl)->summarize();
  }

#if CC_ALG == MICA
  double t = (double)(endtime - starttime) / 1000000000.;
  m_wl->mica_db->print_stats(t, t * thd_cnt);

  for (auto it : m_wl->tables) {
    auto table = it.second;
    uint64_t part_id = 0;
    for (auto mica_tbl : table->mica_tbl) {
      printf("table %s part%2" PRIu64 ":\n", it.first.c_str(), part_id);
      mica_tbl->print_table_status();
      // printf("\n");
      part_id++;
    }
  }

  m_wl->mica_db->print_pool_status();

  m_wl->mica_page_pools[0]->print_status();
  m_wl->mica_page_pools[1]->print_status();

  ::mica::util::Latency inter_commit_latency;
  for (uint32_t i = 0; i < thd_cnt; i++)
    inter_commit_latency += m_wl->mica_db->context(static_cast<uint16_t>(i))
                                ->inter_commit_latency();
#else
  ::mica::util::Latency inter_commit_latency;
  for (uint32_t i = 0; i < thd_cnt; i++)
    inter_commit_latency += m_thds[i]->inter_commit_latency;

  // printf("sum: %" PRIu64 " (us)\n", inter_commit_latency.sum());
  printf("inter-commit latency (us): min=%" PRIu64 ", max=%" PRIu64
         ", avg=%" PRIu64 "; 50-th=%" PRIu64 ", 95-th=%" PRIu64
         ", 99-th=%" PRIu64 ", 99.9-th=%" PRIu64 "\n",
         inter_commit_latency.min(), inter_commit_latency.max(),
         inter_commit_latency.avg(), inter_commit_latency.perc(0.50),
         inter_commit_latency.perc(0.95), inter_commit_latency.perc(0.99),
         inter_commit_latency.perc(0.999));
#endif

  fprintf(stderr, "mem_allocator stats after main processing:\n");
  mem_allocator.dump_stats();

#if PRINT_LAT_DIST
  printf("LatencyStart\n");
  inter_commit_latency.print(stdout);
  printf("LatencyEnd\n");
#endif
  return 0;
}

void* f(void* id) {
  uint64_t tid = (uint64_t)id;

  m_thds[tid]->run();
  return NULL;
}
