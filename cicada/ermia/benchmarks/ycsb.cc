/*
 * A YCSB implementation based off of Silo's and equivalent to FOEDUS's.
 */
#include <iostream>
#include <sstream>
#include <vector>
#include <utility>
#include <string>
#include <set>

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <numa.h>

#include "../macros.h"
#include "../util.h"
#include "../spinbarrier.h"

#include "bench.h"
#include "ycsb.h"

using namespace std;
using namespace util;

YcsbRecord::YcsbRecord(char value) {
  memset(data_, value, kFields * kFieldLength * sizeof(char));
}

void YcsbRecord::initialize_field(char *field) {
  memset(field, 'a', kFieldLength);
}

uint64_t local_key_counter[sysconf::MAX_THREADS];

uint g_reps_per_tx = 1;
uint g_rmw_additional_reads = 0;
double g_rmw_read_ratio = 0.;
double g_zipf_theta = 0.;
char g_workload = 'F';
uint g_initial_table_size = 10000;
int g_sort_load_keys = 0;

// { insert, read, update, scan, rmw }
YcsbWorkload YcsbWorkloadA('A', 0,  50U,  100U, 0,    0);     // Workload A - 50% read, 50% update
YcsbWorkload YcsbWorkloadB('B', 0,  95U,  100U, 0,    0);     // Workload B - 95% read, 5% update
YcsbWorkload YcsbWorkloadC('C', 0,  100U, 0,    0,    0);     // Workload C - 100% read
YcsbWorkload YcsbWorkloadD('D', 5U, 100U, 0,    0,    0);     // Workload D - 95% read, 5% insert
YcsbWorkload YcsbWorkloadE('E', 5U, 0,    0,    100U, 0);     // Workload E - 5% insert, 95% scan

// Combine reps_per_tx and rmw_additional_reads to have "10R+10RMW" style transactions.
YcsbWorkload YcsbWorkloadF('F', 0,  0,    0,    0,    100U);  // Workload F - 100% RMW

// Extra workloads (not in spec)
YcsbWorkload YcsbWorkloadG('G', 0,  0,    5U,   100U, 0);     // Workload G - 5% update, 95% scan
YcsbWorkload YcsbWorkloadH('H', 0,  0,    0,    100U, 0);     // Workload H - 100% scan

YcsbWorkload workload = YcsbWorkloadF;

// This looks like sharing the same random state, which is bad
//fast_random rnd_record_select(477377);

/*
YcsbKey key_arena;
YcsbKey&
build_rmw_key(int worker_id) {
  uint64_t key_seq = rnd_record_select.next_uniform() * g_initial_table_size;
  auto cnt = local_key_counter[worker_id];
  if (cnt == 0) {
    cnt = local_key_counter[0];
  }
  auto hi = key_seq / cnt;
  auto lo = key_seq % cnt;
  key_arena.build(hi, lo);
  return key_arena;
}
*/

class ycsb_worker : public bench_worker {
public:
  ycsb_worker(unsigned int worker_id,
              unsigned long seed, abstract_db *db,
              const map<string, abstract_ordered_index *> &open_tables,
              spin_barrier *barrier_a, spin_barrier *barrier_b)
    : bench_worker(worker_id, seed, db,
                   open_tables, barrier_a, barrier_b),
      tbl(open_tables.at("USERTABLE")),
      rnd_record_select(477377 + seed),
      rnd_op_select(477377 + 10101 + seed)
  {
    all_keys = new YcsbKey[ops_per_worker * (g_reps_per_tx + g_rmw_additional_reads)];
  }

  virtual ~ycsb_worker() { delete [] all_keys; }

  virtual workload_desc_vec
  get_workload() const
  {
    workload_desc_vec w;
    if (workload.insert_percent())
      w.push_back(workload_desc("Insert", double(workload.insert_percent())/100.0, TxnInsert));
    if (workload.read_percent())
      w.push_back(workload_desc("Read", double(workload.read_percent())/100.0, TxnRead));
    if (workload.update_percent())
      w.push_back(workload_desc("Update", double(workload.update_percent())/100.0, TxnUpdate));
    if (workload.scan_percent())
      w.push_back(workload_desc("Scan", double(workload.scan_percent())/100.0, TxnScan));
    if (workload.rmw_percent())
      w.push_back(workload_desc("RMW", double(workload.rmw_percent())/100.0, TxnRMW));
    return w;
  }

  static rc_t
  TxnInsert(bench_worker *w)
  {
    return {RC_TRUE};
  }

  static rc_t
  TxnRead(bench_worker *w)
  {
    return {RC_TRUE};
  }

  static rc_t
  TxnUpdate(bench_worker *w)
  {
    return {RC_TRUE};
  }

  static rc_t
  TxnScan(bench_worker *w)
  {
    return {RC_TRUE};
  }

  static rc_t
  TxnRMW(bench_worker *w)
  {
    return static_cast<ycsb_worker *>(w)->txn_rmw();
  }

  rc_t txn_rmw() {
    assert(g_reps_per_tx + g_rmw_read_ratio <= max_keys);

    void *txn = db->new_txn(0, arena, txn_buf(), abstract_db::HINT_DEFAULT);
    arena.reset();
    const uint32_t threshold = (uint32_t)(g_rmw_read_ratio * (double)(1 << 20));
    size_t off = get_ntxn_commits() * (g_reps_per_tx + g_rmw_additional_reads);
    for (uint i = 0; i < g_reps_per_tx; ++i) {
      bool read = (rnd_op_select.next_u32() % (1 << 20)) < threshold;
      auto& key = all_keys[off + i];
      varstr k((char *)&key.data_, sizeof(key));
      varstr v = str(sizeof(YcsbRecord));
      // TODO(tzwang): add read/write_all_fields knobs
      try_catch(tbl->get(txn, k, v));  // Read
      if (!read) {
        memset(v.data(), 'a', v.size());
        ASSERT(v.size() == sizeof(YcsbRecord));
        try_catch(tbl->put(txn, k, v));  // Modify-write
      }
    }

    for (uint i = 0; i < g_rmw_additional_reads; ++i) {
      auto& key = all_keys[off + g_reps_per_tx + i];
      varstr k((char *)&key.data_, sizeof(key));
      varstr v = str(sizeof(YcsbRecord));
      // TODO(tzwang): add read/write_all_fields knobs
      try_catch(tbl->get(txn, k, v));  // Read
    }
    try_catch(db->commit_txn(txn));
    return {RC_TRUE};
  }

  static void calculateDenom() {
    // printf("n=%u, theta=%lf\n", g_initial_table_size, g_zipf_theta);
    assert(the_n == 0);
    the_n = g_initial_table_size;
    denom = zeta(the_n, g_zipf_theta);
    zeta_2_theta = zeta(2, g_zipf_theta);
  }

protected:
  virtual void
  on_run_setup() override
  {
    for (uint t = 0; t < ops_per_worker; ++t) {
      size_t off = t * (g_reps_per_tx + g_rmw_additional_reads);

      for (uint i = 0; i < g_reps_per_tx + g_rmw_additional_reads; ++i) {
        bool duplicate = true;
        while (duplicate) {
          duplicate = false;
          uint64_t key_seq = zipf(g_initial_table_size, g_zipf_theta);
          auto& key = all_keys[off + i];
          auto cnt = local_key_counter[worker_id];
          if (cnt == 0) {
            cnt = local_key_counter[0];
          }
          auto hi = key_seq / cnt;
          auto lo = key_seq % cnt;
          key.build(hi, lo);

          for (uint j = 0; j < i; j++)
            if (all_keys[off + j] == all_keys[off + i]) {
              duplicate = true;
              break;
          }
        }
      }
    }
    cerr << "[INFO] finished generating keys [worker_id=" << worker_id << "]" << endl;
  }

  inline ALWAYS_INLINE varstr&
  str(uint64_t size) {
    return *arena.next(size);
  }

private:
  abstract_ordered_index *tbl;
  fast_random rnd_record_select;
  fast_random rnd_op_select;

  static const size_t max_keys = 16;
  YcsbKey* all_keys;

  static double zeta(uint64_t n, double theta) {
    double sum = 0;
    for (uint64_t i = 1; i <= n; i++) sum += pow(1.0 / i, theta);
    return sum;
  }
  uint64_t zipf(uint64_t n, double theta) {
    assert(this->the_n == n);
    assert(theta == g_zipf_theta);
    double alpha = 1 / (1 - theta);
    double zetan = denom;
    double eta = (1 - pow(2.0 / n, 1 - theta)) / (1 - zeta_2_theta / zetan);
    double u;
    u = rnd_record_select.next_uniform();
    double uz = u * zetan;
    // if (uz < 1) return 1;
    // if (uz < 1 + pow(0.5, theta)) return 2;
    // return 1 + (uint64_t)(n * pow(eta*u -eta + 1, alpha));
    if (uz < 1) return 0;
    if (uz < 1 + pow(0.5, theta)) return 1;
    uint64_t v = 0 + (uint64_t)(n * pow(eta * u - eta + 1, alpha));
    if (v >= n) v = n - 1;
    return v;
  }
  static uint64_t the_n;
  static double denom;
  static double zeta_2_theta;
};
uint64_t ycsb_worker::the_n;
double ycsb_worker::denom;
double ycsb_worker::zeta_2_theta;

class ycsb_usertable_loader : public bench_loader {
public:
  ycsb_usertable_loader(unsigned long seed,
                        abstract_db *db,
                        const map<string, abstract_ordered_index *> &open_tables)
    : bench_loader(seed, db, open_tables)
  {}

protected:
  // XXX(tzwang): for now this is serial
  void load() {
    abstract_ordered_index *tbl = open_tables.at("USERTABLE");
    std::vector<YcsbKey> keys;
    uint64_t records_per_thread = g_initial_table_size / sysconf::worker_threads;
    bool spread = true;
    if (records_per_thread == 0) {
      // Let one thread load all the keys if we don't have at least one record per **worker** thread
      records_per_thread = g_initial_table_size;
      spread = false;
    } else {
      g_initial_table_size = records_per_thread * sysconf::worker_threads;
    }

    if (verbose) {
      cerr << "[INFO] requested for " << g_initial_table_size << " records, will load " 
        << records_per_thread * sysconf::worker_threads << endl;
    }

    // insert an equal number of records on behalf of each worker
    YcsbKey key;
    uint64_t inserted = 0;
    for (uint16_t worker_id = 0; worker_id < sysconf::worker_threads; worker_id++) {
      local_key_counter[worker_id] = 0;
      auto remaining_inserts = records_per_thread;
      uint32_t high = worker_id, low = 0;
      while (true) {
        key.build(high, low++);
        keys.push_back(key);
        inserted++;
        local_key_counter[worker_id]++;
        if (--remaining_inserts == 0)
          break;
      }
      if (not spread)  // do it on behalf of only one worker
        break;
    }

    ALWAYS_ASSERT(keys.size());
    if (g_sort_load_keys)
      std::sort(keys.begin(), keys.end());

    // start a transaction and insert all the records
    for (auto& key : keys) {
      YcsbRecord r('a');
      varstr k((char *)&key.data_, sizeof(key));
      varstr v(r.data_, sizeof(r));
      void *txn = db->new_txn(0, arena, txn_buf(), abstract_db::HINT_DEFAULT);
      arena.reset();
      try_verify_strict(tbl->insert(txn, k, v));
      try_verify_strict(db->commit_txn(txn));
    }

    if (verbose)
      cerr << "[INFO] loaded " << inserted << " kyes in USERTABLE" << endl;
  }
};

class ycsb_bench_runner : public bench_runner {
public:
  ycsb_bench_runner(abstract_db *db)
    : bench_runner(db)
  {
  }

  virtual void prepare(char *)
  {
    open_tables["USERTABLE"] = db->open_index("USERTABLE", kRecordSize);
  }

protected:
  virtual vector<bench_loader *>
  make_loaders()
  {
    vector<bench_loader *> ret;
    ret.push_back(new ycsb_usertable_loader(0, db, open_tables));
    return ret;
  }

  virtual vector<bench_worker *>
  make_workers()
  {
    ycsb_worker::calculateDenom();
    fast_random r(8544290);
    vector<bench_worker *> ret;
    for (size_t i = 0; i < sysconf::worker_threads; i++)
      ret.push_back(
        new ycsb_worker(
          i, r.next(), db, open_tables,
          &barrier_a, &barrier_b));
    return ret;
  }
};

void
ycsb_do_test(abstract_db *db, int argc, char **argv)
{
  // parse options
  optind = 1;
  while (1) {
    static struct option long_options[] =
    {
      {"reps-per-tx"            , required_argument, 0                , 'r' },
      {"rmw-additional-reads"   , required_argument, 0                , 'a' },
      {"rmw-read-ratio"         , required_argument, 0                , 't' },
      {"zipf-theta"             , required_argument, 0                , 'z' },
      {"workload"               , required_argument, 0                , 'w' },
      {"initial-table-size"     , required_argument, 0                , 's' },
      {"sort-load-keys"         , no_argument      , &g_sort_load_keys, 1   },
      {0, 0, 0, 0}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "r:a:t:z:w:s:", long_options, &option_index);
    if (c == -1)
      break;
    switch (c) {
    case 0:
      if (long_options[option_index].flag != 0)
        break;
      abort();
      break;

    case 'r':
      g_reps_per_tx = strtoul(optarg, NULL, 10);
      break;

    case 'a':
      g_rmw_additional_reads = strtoul(optarg, NULL, 10);
      break;

    case 't':
      g_rmw_read_ratio = strtod(optarg, NULL);
      break;

    case 'z':
      g_zipf_theta = strtod(optarg, NULL);
      break;

    case 's':
      g_initial_table_size = strtoul(optarg, NULL, 10);
      break;

    case 'w':
      g_workload = optarg[0];
      if (g_workload == 'A')
        workload = YcsbWorkloadA;
      else if (g_workload == 'B')
        workload = YcsbWorkloadB;
      else if (g_workload == 'C')
        workload = YcsbWorkloadC;
      else if (g_workload == 'D')
        workload = YcsbWorkloadD;
      else if (g_workload == 'E')
        workload = YcsbWorkloadE;
      else if (g_workload == 'F')
        workload = YcsbWorkloadF;
      else if (g_workload == 'G')
        workload = YcsbWorkloadG;
      else if (g_workload == 'H')
        workload = YcsbWorkloadH;
      else {
        cerr << "Wrong workload type: " << g_workload << endl;
        abort();
      }
      break;

    case '?':
      /* getopt_long already printed an error message. */
      exit(1);

    default:
      abort();
    }
  }

  ALWAYS_ASSERT(g_initial_table_size);

  // Both must be non-zero because we use a trace.
  ALWAYS_ASSERT(ops_per_worker);
  ALWAYS_ASSERT(max_runtime);

  if (verbose) {
    cerr << "ycsb settings:" << endl
         << "  workload:                   " << g_workload << endl
         << "  initial user table size:    " << g_initial_table_size << endl
         << "  operations per transaction: " << g_reps_per_tx << endl
         << "  additional reads after RMW: " << g_rmw_additional_reads << endl
         << "  read ratio in RMW:          " << g_rmw_read_ratio << endl
         << "  zipf theta:                 " << g_zipf_theta << endl
         << "  sort load keys:             " << g_sort_load_keys << endl;
  }

  ycsb_bench_runner r(db);
  r.run();
}
