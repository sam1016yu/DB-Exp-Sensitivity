/*
 *  The code is part of our project called DrTM, which leverages HTM and RDMA for speedy distributed
 *  in-memory transactions.
 *
 *
 * Copyright (C) 2015 Institute of Parallel and Distributed Systems (IPADS), Shanghai Jiao Tong University
 *     All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  For more about this software, visit:  http://ipads.se.sjtu.edu.cn/drtm.html
 *
 */

/*
 * bench worker,runner s  - XingDa
 */


#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <utility>
#include <string>

#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <sys/time.h>
#include <sys/timex.h>

#include "bench.h"
//#include "base_txn_btree.h"
//#include "counter.h"
#include "scopedperf.hh"
//#include "allocator.h"
#define SET_AFFINITY 1

#ifdef USE_JEMALLOC
//cannot include this header b/c conflicts with malloc.h
//#include <jemalloc/jemalloc.h>
extern "C" void malloc_stats_print(void (*write_cb)(void *, const char *), void *cbopaque, const char *opts);
extern "C" int mallctl(const char *name, void *oldp, size_t *oldlenp, void *newp, size_t newlen);
#endif
#ifdef USE_TCMALLOC
#include <google/heap-profiler.h>
#endif

#include <glog/logging.h>
#include <extra.h>

using namespace std;
using namespace util;

volatile uint64_t timestamp;

size_t nthreads = 1;
size_t total_partition = 1;
size_t current_partition = 0;
std::string config_file = "default.txt";//this file may not exesists

volatile bool running = true;
int verbose = 0;
uint64_t txn_flags = 0;
double scale_factor = 1.0;
uint64_t runtime = 30;
uint64_t ops_per_worker = 0;
int run_mode = RUNMODE_TIME;
int enable_parallel_loading = false;
int pin_cpus = 0;
int slow_exit = 0;
int retry_aborted_transaction = 0;
int no_reset_counters = 0;
int backoff_aborted_transaction = 0;

uint64_t read_bytes[MAX_THREADS];
uint64_t write_bytes[MAX_THREADS];
uint64_t time_rdma_op[MAX_THREADS];

uint64_t rdma_reads[MAX_CLIENTS][MAX_THREADS];
uint64_t rdma_writes[MAX_CLIENTS][MAX_THREADS];
uint64_t rdma_atomics[MAX_CLIENTS][MAX_THREADS];
uint64_t rdma_searches[MAX_CLIENTS][MAX_THREADS];

uint64_t g_rdma_size = 0;
uint64_t g_rdmatablesize = 0;
uint64_t g_records_per_thread = 0;
uint64_t g_reads_per_txn = 0;
uint64_t g_writes_per_txn = 0;

//#define HYPER

//#define SCALE

#ifdef HYPER

const int per_socket_cores = 20;//TODO!! hard coded

int socket_0[] =  {
  1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39

};

int socket_1[] = {
  0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38
};


#else

//methods for set affinity
//const int per_socket_cores = 10;//TODO!! hard coded
const int per_socket_cores = 8;//reserve 2 cores

int socket_0[] =  {
  1,3,5,7,9,11,13,15,17,19

};

int socket_1[] = {
  0,2,4,6,8,10,12,14,16,18
};

#endif


void PinToTPCCCPU(int w_id) {

  int y;
#ifdef SCALE
  //specific  binding for scale tests
  int mac_per_node = 16 / nthreads;//there are total 16 threads avialable
  int mac_num = current_partition % mac_per_node;

  if (mac_num < mac_per_node / 2) {
    y = socket_0[w_id - 1 + mac_num * nthreads];
  }else {
    y = socket_1[w_id - 1 + (mac_num - mac_per_node / 2) * nthreads];
  }
#else
  //bind ,andway
  if ( w_id - 1 >= per_socket_cores) {
    //there is no other cores in the first socket
    y = socket_1[w_id - 1 - per_socket_cores];
  }else {
    y = socket_0[w_id - 1 - 8];
  }
#endif
  cpu_set_t  mask;
  CPU_ZERO(&mask);
  CPU_SET(y , &mask);
  sched_setaffinity(0, sizeof(mask), &mask);
}



template <typename T>
static void
delete_pointers(const vector<T *> &pts)
{
  for (size_t i = 0; i < pts.size(); i++)
    delete pts[i];
}

template <typename T>
static vector<T>
elemwise_sum(const vector<T> &a, const vector<T> &b)
{
  INVARIANT(a.size() == b.size());
  vector<T> ret(a.size());
  for (size_t i = 0; i < a.size(); i++)
    ret[i] = a[i] + b[i];
  return ret;
}

template <typename K, typename V>
static void
map_agg(map<K, V> &agg, const map<K, V> &m)
{
  for (typename map<K, V>::const_iterator it = m.begin();
       it != m.end(); ++it)
    agg[it->first] += it->second;
}

// returns <free_bytes, total_bytes>
static pair<uint64_t, uint64_t>
get_system_memory_info()
{
  struct sysinfo inf;
  sysinfo(&inf);
  return make_pair(inf.mem_unit * inf.freeram, inf.mem_unit * inf.totalram);
}

static bool
clear_file(const char *name)
{
  ofstream ofs(name);
  ofs.close();
  return true;
}

static void
write_cb(void *p, const char *s) UNUSED;
static void
write_cb(void *p, const char *s)
{
  const char *f = "jemalloc.stats";
  static bool s_clear_file UNUSED = clear_file(f);
  ofstream ofs(f, ofstream::app);
  ofs << s;
  ofs.flush();
  ofs.close();
}


//static event_avg_counter evt_avg_abort_spins("avg_abort_spins");
__inline__ int64_t XADD64(uint64_t* addr, int64_t val) {
    asm volatile(
        "lock;xaddq %0, %1"
        : "+a"(val), "+m"(*addr)
        :
        : "cc");

    return val;
}


uint64_t bench_worker::total_ops = 0;
void
bench_worker::run()
{
  // XXX(stephentu): so many nasty hacks here. should actually
  // fix some of this stuff one day
  //printf("worker id %d\n", worker_id);

#if SET_AFFINITY

  int x = worker_id;
  int y = x - 8;


  bool set_af = true;

#ifdef SCALE
  //specific  binding for scale tests
  int mac_per_node = 16 / nthreads;//there are total 16 threads avialable
  int mac_num = current_partition % mac_per_node;

  if (mac_num < mac_per_node / 2) {
    y = socket_0[x - 8 + mac_num * nthreads];
  }else {
    y = socket_1[x - 8 + (mac_num - mac_per_node / 2) * nthreads];
  }
#else
#if 0
  //bind ,andway
  if ( x - 8 >= per_socket_cores) {
    //there is no other cores in the first socket
    y = socket_1[x - 8 - per_socket_cores];
  }else {
    y = socket_0[x - 8];
  }
#endif
    // In the original code, if the thread_id is smaller than 8, it maps to odd number of core cores (1,3,5...)
    // if the thread_id is greater than 8, it maps to even number of cpu cores (2,4,6)
    // For the purpose of using more threads than the number of CPU cores
    // I slightly modified the code so that don't need to use a redundant array to achieve the same goal of mapping
    
    // x - 8 is the real 0 based thread id (DrTM's thread indexing issue)
    if (x - 8 < nthreads / 2)  {
        y = (x - 8) * 2 + 1;
        assert(y >= 0);
        y %= 28; // HARD CODEDED, as the test platform has only 28 CPU cores
    }
    else {
        y = (x - 8 - nthreads / 2) * 2;
        assert(y >= 0);
        y %= 28; // HARD CODEDED, as the test platform has only 28 CPU cores
    }

#endif
  //  if (y <= 7) {
  if(set_af) {
    //fprintf(stdout,"worker: %d binding %d\n",x - 8,y);
    cpu_set_t  mask;
    CPU_ZERO(&mask);
    CPU_SET(y , &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
  }
#endif
#if 0
  if (set_core_id)
    coreid::set_core_id(worker_id); // cringe
  {
    //    scoped_rcu_region r; // register this thread in rcu region
  }
#endif
  on_run_setup();
  scoped_db_thread_ctx ctx(db, false);
  const workload_desc_vec workload = get_workload();
  txn_counts.resize(workload.size());
  barrier_a->count_down();
  barrier_b->wait_for();

  timer thr_timer;

  while (running && (run_mode != RUNMODE_OPS || total_ops > 0)) {

        if (run_mode == RUNMODE_OPS) {
            int64_t oldv = XADD64(&total_ops, -1000);
            if(oldv <= 0) break;
        }
    for (int i =0; i < 1000; i++) {
      double d = r.next_uniform();
      for (size_t i = 0; i < workload.size(); i++) {
	if ((i + 1) == workload.size() || d < workload[i].frequency) {
	retry:
	  timer t;
	  const unsigned long old_seed = r.get_seed();
	  //uint64_t start = rdtsc();

      uint64_t start = mytimer::timestamp();
	  const auto ret = workload[i].fn(this);
      uint64_t end = mytimer::timestamp();
      latencies->push_back(end - start);
      if (latencies->size() >= MAX_LATENCIES) {
        fprintf(stderr, "WARNING: exceed the max number of recorded latencies: %ld\n", latencies->size());
      }

	  if (likely(ret.first)) {
	    ++ntxn_commits;
	    //uint64_t lat = rdtsc() - start;
#ifdef LAT
	    latency_buffer.push_back(lat);
#endif
	    //	    latency_numer_us += lat;

	    backoff_shifts >>= 1;
	  } else {
	    ++ntxn_aborts;
	    if (retry_aborted_transaction && running) {
	      if (backoff_aborted_transaction) {
		if (backoff_shifts < 63)
		  backoff_shifts++;
		uint64_t spins = 1UL << backoff_shifts;
		spins *= 100; // XXX: tuned pretty arbitrarily
		//             evt_avg_abort_spins.offer(spins);
		while (spins) {
		  nop_pause();
		  spins--;
		}
	      }
	      r.set_seed(old_seed);
	      goto retry;
	    }
	  }

	  size_delta += ret.second; // should be zero on abort
	  txn_counts[i]++; // txn_counts aren't used to compute throughput (is
	  // just an informative number to print to the console
	  // in verbose mode)
	  break;
	}
	d -= workload[i].frequency;
      }
    }
  }

  secs +=  thr_timer.lap();

  while (!processDelayed());
  //printf("rdtsc %ld\n",secs);
}


void
bench_runner::run()
{
  // init rdma resources
  init_rdma();
  // load data

  const vector<bench_loader *> loaders = make_loaders();
  {
    spin_barrier b(loaders.size());
    const pair<uint64_t, uint64_t> mem_info_before = get_system_memory_info();
    {
      scoped_timer t("dataloading", verbose);
      for (vector<bench_loader *>::const_iterator it = loaders.begin();
          it != loaders.end(); ++it) {
        (*it)->set_barrier(b);
        (*it)->start();
      }
      for (vector<bench_loader *>::const_iterator it = loaders.begin();
          it != loaders.end(); ++it)
        (*it)->join();
    }
    const pair<uint64_t, uint64_t> mem_info_after = get_system_memory_info();
    const int64_t delta = int64_t(mem_info_before.first) - int64_t(mem_info_after.first); // free mem
    const double delta_mb = double(delta)/1048576.0;
    if (verbose)
      LOG(INFO) << "DB size: " << delta_mb << " MB" << endl;
  }

  initPut();
  map<string, size_t> table_sizes_before;

  if (verbose) {
    LOG(INFO) << "starting benchmark..." << endl;
  }

  bench_worker::total_ops = ops_per_worker * nthreads;
  const pair<uint64_t, uint64_t> mem_info_before = get_system_memory_info();

  const vector<bench_worker *> workers = make_workers();

  // Start perf profiler
  int perf_pid = -1;
  if (g_use_perf && current_partition == 0) {
      std::string date = date_str();
      string file_name = "perf_node" + to_string(total_partition) + "_nthreads" + to_string(nthreads) + "_sf" + to_string(scale_factor);

      string perf_output_file =
              perf_output_dir + "/" + file_name + "-" + date + PERF_EXTENSION;
      string perf_command_w_output =
              perf_command + " -o " + perf_output_file + " -p " + std::to_string(getpid());

      perf_pid = fork_excel(perf_command_w_output);
      LOG(INFO) << "fork_excel, pid=" << perf_pid << ", command=" << perf_command_w_output;
  }

  ALWAYS_ASSERT(!workers.empty());
  for (vector<bench_worker *>::const_iterator it = workers.begin();
       it != workers.end(); ++it){

    (*it)->start();
  }

  LOG(INFO) << "Start threads (listener + workers), cnt=" << to_string(workers.size());

  barrier_a.wait_for(); // wait for all threads to start up
  timer t, t_nosync;
  barrier_b.count_down(); // bombs away!
  if (run_mode == RUNMODE_TIME) {
    sleep(runtime);
    running = false;
  }
  __sync_synchronize();
  //notice,we will join the listener thread at the end
  for (size_t i = 0; i < nthreads ; i++) {
    workers[i]->join();
  }
  const unsigned long elapsed_nosync = t_nosync.lap();

  LOG(INFO) << "Join workers, cnt=" << to_string(nthreads);

  // End profiler
  if (current_partition == 0 && perf_pid >= 0) {
      LOG(INFO) << "interrupt_kill, pid=" << perf_pid;
      interrupt_kill(perf_pid);
      perf_pid = -1;
  }


  //db->do_txn_finish(); // waits for all worker txns to persist
  LOG(INFO) << endl << dashed_line("Sync and print logs");
  sync_log();
  printf((std::string(HEADER_DASHED_WIDTH_DEFAULT, '-') + "\n").data() );

  const unsigned long elapsed = t.lap(); // lap() must come after do_txn_finish(),
                                         // because do_txn_finish() potentially
                                         // waits a b

  size_t n_commits = 0;
  size_t n_aborts = 0;
  uint64_t latency_numer_us = 0;
  for (size_t i = 0; i < nthreads; i++) {
    n_commits += workers[i]->get_ntxn_commits();
    n_aborts += workers[i]->get_ntxn_aborts();
    latency_numer_us += workers[i]->get_latency_numer_us();
  }

  const double elapsed_nosync_sec = double(elapsed_nosync) / 1000000.0;
  const double agg_nosync_throughput = double(n_commits) / elapsed_nosync_sec;
  const double avg_nosync_per_core_throughput = agg_nosync_throughput / double(workers.size()-1);

  const double elapsed_sec = double(elapsed) / 1000000.0;
  const double agg_throughput = double(n_commits) / elapsed_sec;
  const double avg_per_core_throughput = agg_throughput / double(workers.size()-1);

  const double agg_abort_rate = double(n_aborts) / elapsed_sec;
  const double avg_per_core_abort_rate = agg_abort_rate / double(workers.size()-1);

  // we can use n_commits here, because we explicitly wait for all txns
  // run to be durable
  const double agg_persist_throughput = double(n_commits) / elapsed_sec;
  const double avg_per_core_persist_throughput =
    agg_persist_throughput / double(workers.size()-1);

  // XXX(stephentu): latency currently doesn't account for read-only txns
  const double avg_latency_us =
    double(latency_numer_us) / double(n_commits);
  const double avg_latency_ms = avg_latency_us / 1000.0;
  const double avg_persist_latency_ms = 0;
//    get<2>(persisted_info) / 1000.0;

  std::stringstream ss;
//    printf ("total_commits=%ld\n", n_commits);
//    printf ("total_aborts=%ld\n", n_aborts);

    LOG(INFO) << "Total commits=" << n_commits << ", total_aborts=" << n_aborts;
  if (verbose) {

    const pair<uint64_t, uint64_t> mem_info_after = get_system_memory_info();
    const int64_t delta = int64_t(mem_info_before.first) - int64_t(mem_info_after.first); // free mem
    const double delta_mb = double(delta)/1048576.0;
    map<string, size_t> agg_txn_counts = workers[0]->get_txn_counts();
    map<string, double> agg_txn_throughput;

    ssize_t size_delta = workers[0]->get_size_delta();
    for (size_t i = 1; i < nthreads; i++) {
      map_agg(agg_txn_counts, workers[i]->get_txn_counts());
      size_delta += workers[i]->get_size_delta();
    }

    map<string, size_t>::iterator it = agg_txn_counts.begin();
    for(;it != agg_txn_counts.end();++it) {
      agg_txn_throughput[it->first] = agg_txn_counts[it->first] / elapsed_sec;
    }
    const double size_delta_mb = double(size_delta)/1048576.0;
//    map<string, counter_data> ctrs = event_counter::get_all_counters();

//    cerr << "--- table statistics ---" << endl;

#ifdef ENABLE_BENCH_TXN_COUNTERS
    cerr << "--- txn counter statistics ---" << endl;
    {
      // take from thread 0 for now
      abstract_db::txn_counter_map agg = workers[0]->get_local_txn_counters();
      for (auto &p : agg) {
        cerr << p.first << ":" << endl;
        for (auto &q : p.second)
          cerr << "  " << q.first << " : " << q.second << endl;
      }
    }
#endif
    ss.str("");
    ss << endl << dashed_line("Benchmark Stats") << endl
    << "runtime: " << elapsed_sec << " sec" << endl
    << "memory delta: " << delta_mb  << " MB" << endl
    << "memory delta rate: " << (delta_mb / elapsed_sec)  << " MB/sec" << endl
    << "logical memory delta: " << size_delta_mb << " MB" << endl
    << "logical memory delta rate: " << (size_delta_mb / elapsed_sec) << " MB/sec" << endl
    << "agg_nosync_throughput: " << agg_nosync_throughput << " ops/sec" << endl
    << "avg_nosync_per_core_throughput: " << avg_nosync_per_core_throughput << " ops/sec/core" << endl
    //      << "agg_throughput: " << agg_throughput << " ops/sec" << endl
    //      << "avg_per_core_throughput: " << avg_per_core_throughput << " ops/sec/core" << endl
    //      << "agg_persist_throughput: " << agg_persist_throughput << " ops/sec" << endl
    //      << "avg_per_core_persist_throughput: " << avg_per_core_persist_throughput << " ops/sec/core" << endl
    << "avg_latency: " << avg_latency_ms << " ms" << endl
    //      << "avg_persist_latency: " << avg_persist_latency_ms << " ms" << endl
    << "agg_abort_rate: " << agg_abort_rate << " aborts/sec" << endl
    << "agg_abort_num: " << n_aborts <<endl
    << "avg_per_core_abort_rate: " << avg_per_core_abort_rate << " aborts/sec/core" << endl
    << "txn breakdown: " << format_list(agg_txn_counts.begin(), agg_txn_counts.end()) << endl
    << "txn breakdown throughput: " << format_list(agg_txn_throughput.begin(),agg_txn_throughput.end()) << endl
    << dashed_line();

    LOG(INFO) << ss.str() << endl;
    ss.str("");

    //FIXME!! hard coded
//    ss << agg_txn_throughput["NewOrder"]<<' ';
//    cerr << "--- perf counters (if enabled, for benchmark) ---" << endl;
//    PERF_EXPR(scopedperf::perfsum_base::printall());
    //  cerr << "--- allocator stats ---" << endl;
    // ::allocator::DumpStats();
//    cerr << "---------------------------------------" << endl;

#ifdef USE_JEMALLOC
    cerr << "dumping heap profile..." << endl;
    mallctl("prof.dump", NULL, NULL, NULL, 0);
    cerr << "printing jemalloc stats..." << endl;
    malloc_stats_print(write_cb, NULL, "");
#endif
#ifdef USE_TCMALLOC
    HeapProfilerDump("before-exit");
#endif
  }

  // output for plotting script
  ss<<agg_nosync_throughput<<endl;

  LOG(INFO) << endl
      << "agg_nosync_throughput=" << agg_nosync_throughput << endl
      << "agg_persist_throughput=" << agg_persist_throughput << endl
      << "elapsed_sec=" << elapsed_sec << endl
      << "agg_abort_rate=" << agg_abort_rate << endl;

  cout.flush();

  //-------------------------------------------
  //Tell the listener thread to exit

  workers[0]->ending(ss.str());
  // Join listener
  workers[nthreads]->join();
  LOG(INFO) << "Join listener";
  fprintf(stdout,"Duang~\n");
  //  final_check(); //check for consistency

#ifdef LAT
  //flush the latecny info to log
  uint64_t start_t = rdtsc();
  sleep(1);
  uint64_t lap = rdtsc() - start_t;

  FILE *file;
  char file_name[64];
  snprintf(file_name,64,"latency_%d.txt",current_partition);
  file = fopen (file_name,"w");

  int counter = 0;
  for(int i = 0; i <  nthreads  ; ++ i ) {
    for(int j = 0;j < workers[i]->latency_buffer.size();++j) {
      fprintf(file,"%.10lf\n",(double)workers[i]->latency_buffer[j]/(double)lap);
      //      fprintf(file,"%lf\n",workers[i]->latency_buffer[j]);
      counter++;
    }
  }

  fprintf(stdout,"total tx latency counted : %d\n",counter);
  fclose(file);
#endif
  //-------------------------------------------

  if (!slow_exit)
    return;

  map<string, uint64_t> agg_stats;
#if 0
  for (map<string, abstract_ordered_index *>::iterator it = open_tables.begin();
       it != open_tables.end(); ++it) {
    map_agg(agg_stats, it->second->clear());
    delete it->second;
  }
#endif
  if (verbose) {
    for (auto &p : agg_stats)
      cerr << p.first << " : " << p.second << endl;

  }
//  open_tables.clear();

  delete_pointers(loaders);
  delete_pointers(workers);
}

template <typename K, typename V>
struct map_maxer {
  typedef map<K, V> map_type;
  void
  operator()(map_type &agg, const map_type &m) const
  {
    for (typename map_type::const_iterator it = m.begin();
        it != m.end(); ++it)
      agg[it->first] = std::max(agg[it->first], it->second);
  }
};

//template <typename KOuter, typename KInner, typename VInner>
//struct map_maxer<KOuter, map<KInner, VInner>> {
//  typedef map<KInner, VInner> inner_map_type;
//  typedef map<KOuter, inner_map_type> map_type;
//};

#ifdef ENABLE_BENCH_TXN_COUNTERS
void
bench_worker::measure_txn_counters(void *txn, const char *txn_name)
{
  auto ret = db->get_txn_counters(txn);
  map_maxer<string, uint64_t>()(local_txn_counters[txn_name], ret);
}
#endif

map<string, size_t>
bench_worker::get_txn_counts() const
{
  map<string, size_t> m;
  const workload_desc_vec workload = get_workload();
  for (size_t i = 0; i < txn_counts.size(); i++)
    m[workload[i].name] = txn_counts[i];
  return m;
}