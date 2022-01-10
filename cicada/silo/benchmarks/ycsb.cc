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
#include "../varkey.h"
#include "../thread.h"
#include "../util.h"
#include "../spinbarrier.h"
#include "../core.h"

#include "bench.h"

using namespace std;
using namespace util;

static size_t nkeys;
static const size_t YCSBRecordSize = 100;

// [R, W, RMW, Scan]
// we're missing remove for now
// the default is a modification of YCSB "A" we made (80/20 R/W)
static unsigned g_txn_workload_mix[] = { 80, 20, 0, 0 };

uint g_reps_per_tx = 1;
uint g_rmw_additional_reads = 0;
double g_zipf_theta = 0.;
double g_rmw_read_ratio = 0.;

class ycsb_worker : public bench_worker {
public:
  ycsb_worker(unsigned int worker_id,
              unsigned long seed, abstract_db *db,
              const map<string, abstract_ordered_index *> &open_tables,
              spin_barrier *barrier_a, spin_barrier *barrier_b)
    : bench_worker(worker_id, true, seed, db,
                   open_tables, barrier_a, barrier_b),
      tbl(open_tables.at("USERTABLE")),
      computation_n(0),
      rnd_record_select(477377 + seed),
      rnd_op_select(477377 + 10101 + seed)
  {
    obj_key0.reserve(str_arena::MinStrReserveLength);
    obj_key1.reserve(str_arena::MinStrReserveLength);
    obj_v.reserve(str_arena::MinStrReserveLength);
    for (size_t i = 0; i < max_keys; i++)
      obj_keys[i].reserve(str_arena::MinStrReserveLength);

    all_keys = new uint64_t[ops_per_worker * (g_reps_per_tx + g_rmw_additional_reads)];
  }

  virtual ~ycsb_worker() { delete [] all_keys; }

  txn_result
  txn_read()
  {
    void * const txn = db->new_txn(txn_flags, arena, txn_buf(), abstract_db::HINT_KV_GET_PUT);
    scoped_str_arena s_arena(arena);
    try {
      const uint64_t k = r.next() % nkeys;
      ALWAYS_ASSERT(tbl->get(txn, u64_varkey(k).str(obj_key0), obj_v));
      computation_n += obj_v.size();
      measure_txn_counters(txn, "txn_read");
      if (likely(db->commit_txn(txn)))
        return txn_result(true, 0);
    } catch (abstract_db::abstract_abort_exception &ex) {
      db->abort_txn(txn);
    }
    return txn_result(false, 0);
  }

  static txn_result
  TxnRead(bench_worker *w)
  {
    return static_cast<ycsb_worker *>(w)->txn_read();
  }

  txn_result
  txn_write()
  {
    void * const txn = db->new_txn(txn_flags, arena, txn_buf(), abstract_db::HINT_KV_GET_PUT);
    scoped_str_arena s_arena(arena);
    try {
      tbl->put(txn, u64_varkey(r.next() % nkeys).str(str()), str().assign(YCSBRecordSize, 'b'));
      measure_txn_counters(txn, "txn_write");
      if (likely(db->commit_txn(txn)))
        return txn_result(true, 0);
    } catch (abstract_db::abstract_abort_exception &ex) {
      db->abort_txn(txn);
    }
    return txn_result(false, 0);
  }

  static txn_result
  TxnWrite(bench_worker *w)
  {
    return static_cast<ycsb_worker *>(w)->txn_write();
  }

  txn_result
  txn_rmw()
  {
    void * const txn = db->new_txn(txn_flags, arena, txn_buf(), abstract_db::HINT_KV_RMW);
    scoped_str_arena s_arena(arena);

    try {
      const uint32_t threshold = (uint32_t)(g_rmw_read_ratio * (double)(1 << 20));
      size_t off = get_ntxn_commits() * (g_reps_per_tx + g_rmw_additional_reads);

      for (uint i = 0; i < g_reps_per_tx; ++i) {
        bool read = (rnd_op_select.next_u32() % (1 << 20)) < threshold;
        auto& key = all_keys[off + i];
        ALWAYS_ASSERT(tbl->get(txn, u64_varkey(key).str(obj_keys[i]), obj_v));
        computation_n += obj_v.size();
        if (!read)
          tbl->put(txn, obj_keys[i], str().assign(YCSBRecordSize, 'c'));
      }

      for (uint i = 0; i < g_rmw_additional_reads; ++i) {
        auto& key = all_keys[off + g_reps_per_tx + i];
        ALWAYS_ASSERT(tbl->get(txn, u64_varkey(key).str(obj_keys[g_reps_per_tx + i]), obj_v));
        computation_n += obj_v.size();
      }

      measure_txn_counters(txn, "txn_rmw");
      if (likely(db->commit_txn(txn)))
       return txn_result(true, 0);
    } catch (abstract_db::abstract_abort_exception &ex) {
      db->abort_txn(txn);
    }

    return txn_result(false, 0);
  }

  static txn_result
  TxnRmw(bench_worker *w)
  {
    return static_cast<ycsb_worker *>(w)->txn_rmw();
  }

  class worker_scan_callback : public abstract_ordered_index::scan_callback {
  public:
    worker_scan_callback() : n(0) {}
    virtual bool
    invoke(const char *, size_t, const string &value)
    {
      n += value.size();
      return true;
    }
    size_t n;
  };

  txn_result
  txn_scan()
  {
    void * const txn = db->new_txn(txn_flags, arena, txn_buf(), abstract_db::HINT_KV_SCAN);
    scoped_str_arena s_arena(arena);
    const size_t kstart = r.next() % nkeys;
    const string &kbegin = u64_varkey(kstart).str(obj_key0);
    const string &kend = u64_varkey(kstart + 100).str(obj_key1);
    worker_scan_callback c;
    try {
      tbl->scan(txn, kbegin, &kend, c);
      computation_n += c.n;
      measure_txn_counters(txn, "txn_scan");
      if (likely(db->commit_txn(txn)))
        return txn_result(true, 0);
    } catch (abstract_db::abstract_abort_exception &ex) {
      db->abort_txn(txn);
    }
    return txn_result(false, 0);
  }

  static txn_result
  TxnScan(bench_worker *w)
  {
    return static_cast<ycsb_worker *>(w)->txn_scan();
  }

  virtual workload_desc_vec
  get_workload() const
  {
    //w.push_back(workload_desc("Read", 0.95, TxnRead));
    //w.push_back(workload_desc("ReadModifyWrite", 0.04, TxnRmw));
    //w.push_back(workload_desc("Write", 0.01, TxnWrite));

    //w.push_back(workload_desc("Read", 1.0, TxnRead));
    //w.push_back(workload_desc("Write", 1.0, TxnWrite));

    // YCSB workload "A" - 50/50 read/write
    //w.push_back(workload_desc("Read", 0.5, TxnRead));
    //w.push_back(workload_desc("Write", 0.5, TxnWrite));

    // YCSB workload custom - 80/20 read/write
    //w.push_back(workload_desc("Read",  0.8, TxnRead));
    //w.push_back(workload_desc("Write", 0.2, TxnWrite));

    workload_desc_vec w;
    unsigned m = 0;
    for (size_t i = 0; i < ARRAY_NELEMS(g_txn_workload_mix); i++)
      m += g_txn_workload_mix[i];
    ALWAYS_ASSERT(m == 100);
    if (g_txn_workload_mix[0])
      w.push_back(workload_desc("Read",  double(g_txn_workload_mix[0])/100.0, TxnRead));
    if (g_txn_workload_mix[1])
      w.push_back(workload_desc("Write",  double(g_txn_workload_mix[1])/100.0, TxnWrite));
    if (g_txn_workload_mix[2])
      w.push_back(workload_desc("ReadModifyWrite",  double(g_txn_workload_mix[2])/100.0, TxnRmw));
    if (g_txn_workload_mix[3])
      w.push_back(workload_desc("Scan",  double(g_txn_workload_mix[3])/100.0, TxnScan));
    return w;
  }

  static void calculateDenom() {
    // printf("n=%u, theta=%lf\n", nkeys, g_zipf_theta);
    assert(the_n == 0);
    the_n = nkeys;
    denom = zeta(the_n, g_zipf_theta);
    zeta_2_theta = zeta(2, g_zipf_theta);
  }

protected:

  virtual void
  on_run_setup() OVERRIDE
  {
    if (!pin_cpus)
      return;
    const size_t a = worker_id % coreid::num_cpus_online();
    const size_t b = a % nthreads;
    rcu::s_instance.pin_current_thread(b);

    for (uint t = 0; t < ops_per_worker; ++t) {
      size_t off = t * (g_reps_per_tx + g_rmw_additional_reads);

      for (uint i = 0; i < g_reps_per_tx + g_rmw_additional_reads; ++i) {
        bool duplicate = true;
        while (duplicate) {
          duplicate = false;
          all_keys[off + i] = zipf(nkeys, g_zipf_theta);
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

  inline ALWAYS_INLINE string &
  str() {
    return *arena.next();
  }

private:
  abstract_ordered_index *tbl;

  string obj_key0;
  string obj_key1;
  string obj_v;

  uint64_t computation_n;

  fast_random rnd_record_select;
  fast_random rnd_op_select;

  static const size_t max_keys = 16;
  string obj_keys[max_keys];

  uint64_t* all_keys;

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

static void
ycsb_load_keyrange(
    uint64_t keystart,
    uint64_t keyend,
    unsigned int pinid,
    abstract_db *db,
    abstract_ordered_index *tbl,
    str_arena &arena,
    uint64_t txn_flags,
    void *txn_buf)
{
  if (pin_cpus) {
    ALWAYS_ASSERT(pinid < nthreads);
    rcu::s_instance.pin_current_thread(pinid);
    rcu::s_instance.fault_region();
  }

  const size_t batchsize = (db->txn_max_batch_size() == -1) ?
    10000 : db->txn_max_batch_size();
  ALWAYS_ASSERT(batchsize > 0);
  const size_t nkeys = keyend - keystart;
  ALWAYS_ASSERT(nkeys > 0);
  const size_t nbatches = nkeys < batchsize ? 1 : (nkeys / batchsize);
  for (size_t batchid = 0; batchid < nbatches;) {
    scoped_str_arena s_arena(arena);
    void * const txn = db->new_txn(txn_flags, arena, txn_buf);
    try {
      const size_t rend = (batchid + 1 == nbatches) ?
        keyend : keystart + ((batchid + 1) * batchsize);
      for (size_t i = batchid * batchsize + keystart; i < rend; i++) {
        ALWAYS_ASSERT(i >= keystart && i < keyend);
        const string k = u64_varkey(i).str();
        const string v(YCSBRecordSize, 'a');
        tbl->insert(txn, k, v);
      }
      if (db->commit_txn(txn))
        batchid++;
      else
        db->abort_txn(txn);
    } catch (abstract_db::abstract_abort_exception &ex) {
      db->abort_txn(txn);
    }
  }
  if (verbose)
    cerr << "[INFO] finished loading USERTABLE range [kstart="
      << keystart << ", kend=" << keyend << ") - nkeys: " << nkeys << endl;
}

class ycsb_usertable_loader : public bench_loader {
public:
  ycsb_usertable_loader(unsigned long seed,
                        abstract_db *db,
                        const map<string, abstract_ordered_index *> &open_tables)
    : bench_loader(seed, db, open_tables)
  {}

protected:
  virtual void
  load()
  {
    abstract_ordered_index *tbl = open_tables.at("USERTABLE");
    const size_t nkeysperthd = nkeys / nthreads;
    for (size_t i = 0; i < nthreads; i++) {
      const size_t keystart = i * nkeysperthd;
      const size_t keyend = min((i + 1) * nkeysperthd, nkeys);
      ycsb_load_keyrange(
          keystart,
          keyend,
          i,
          db,
          tbl,
          arena,
          txn_flags,
          txn_buf());
    }
  }
};

class ycsb_parallel_usertable_loader : public bench_loader {
public:
  ycsb_parallel_usertable_loader(unsigned long seed,
                                 abstract_db *db,
                                 const map<string, abstract_ordered_index *> &open_tables,
                                 unsigned int pinid,
                                 uint64_t keystart,
                                 uint64_t keyend)
    : bench_loader(seed, db, open_tables),
      pinid(pinid), keystart(keystart), keyend(keyend)
  {
    INVARIANT(keyend > keystart);
    if (verbose)
      cerr << "[INFO] YCSB par loader cpu " << pinid
           << " [" << keystart << ", " << keyend << ")" << endl;
  }

protected:
  virtual void
  load()
  {
    abstract_ordered_index *tbl = open_tables.at("USERTABLE");
    ycsb_load_keyrange(
        keystart,
        keyend,
        pinid,
        db,
        tbl,
        arena,
        txn_flags,
        txn_buf());
  }

private:
  unsigned int pinid;
  uint64_t keystart;
  uint64_t keyend;
};


class ycsb_bench_runner : public bench_runner {
public:
  ycsb_bench_runner(abstract_db *db)
    : bench_runner(db)
  {
    open_tables["USERTABLE"] = db->open_index("USERTABLE", YCSBRecordSize);
  }

protected:
  virtual vector<bench_loader *>
  make_loaders()
  {
    vector<bench_loader *> ret;
    const unsigned long ncpus = coreid::num_cpus_online();
    if (enable_parallel_loading && nkeys >= nthreads) {
      // divide the key space amongst all the loaders
      const size_t nkeysperloader = nkeys / ncpus;
      if (nthreads > ncpus) {
        for (size_t i = 0; i < ncpus; i++) {
          const uint64_t kend = (i + 1 == ncpus) ?
            nkeys : (i + 1) * nkeysperloader;
          ret.push_back(
              new ycsb_parallel_usertable_loader(
                0, db, open_tables, i,
                i * nkeysperloader, kend));
        }
      } else {
        // load balance the loaders amongst numa nodes in RR fashion
        //
        // XXX: here we hardcode an assumption about the NUMA topology of
        // the system
        const vector<unsigned> numa_nodes_used = get_numa_nodes_used(nthreads);

        // assign loaders to cores based on numa node assignment in RR fashion
        const unsigned loaders_per_node = ncpus / numa_nodes_used.size();

        vector<unsigned> node_allocations(numa_nodes_used.size(), loaders_per_node);
        // RR the remaining
        for (unsigned i = 0;
             i < (ncpus - loaders_per_node * numa_nodes_used.size());
             i++)
          node_allocations[i]++;

        size_t loader_i = 0;
        for (size_t i = 0; i < numa_nodes_used.size(); i++) {
          // allocate loaders_per_node loaders to this numa node
          const vector<unsigned> cpus = numa_node_to_cpus(numa_nodes_used[i]);
          const vector<unsigned> cpus_avail = exclude(cpus, nthreads);
          const unsigned nloaders = node_allocations[i];
          for (size_t j = 0; j < nloaders; j++, loader_i++) {
            const uint64_t kend = (loader_i + 1 == ncpus) ?
              nkeys : (loader_i + 1) * nkeysperloader;
            ret.push_back(
                new ycsb_parallel_usertable_loader(
                  0, db, open_tables, cpus_avail[j % cpus_avail.size()],
                  loader_i * nkeysperloader, kend));
          }
        }
      }
    } else {
      ret.push_back(new ycsb_usertable_loader(0, db, open_tables));
    }
    return ret;
  }

  virtual vector<bench_worker *>
  make_workers()
  {
    const unsigned alignment = coreid::num_cpus_online();
    const int blockstart =
      coreid::allocate_contiguous_aligned_block(nthreads, alignment);
    ALWAYS_ASSERT(blockstart >= 0);
    ALWAYS_ASSERT((blockstart % alignment) == 0);

    ycsb_worker::calculateDenom();

    fast_random r(8544290);
    vector<bench_worker *> ret;
    for (size_t i = 0; i < nthreads; i++)
      ret.push_back(
        new ycsb_worker(
          blockstart + i, r.next(), db, open_tables,
          &barrier_a, &barrier_b));
    return ret;
  }

private:

  static vector<unsigned>
  get_numa_nodes_used(unsigned nthds)
  {
    // assuming CPUs [0, nthds) are used, what are all the
    // NUMA nodes touched by [0, nthds)
    set<unsigned> ret;
    for (unsigned i = 0; i < nthds; i++) {
      const int node = numa_node_of_cpu(i);
      ALWAYS_ASSERT(node >= 0);
      ret.insert(node);
    }
    return vector<unsigned>(ret.begin(), ret.end());
  }

  static vector<unsigned>
  numa_node_to_cpus(unsigned node)
  {
    struct bitmask *bm = numa_allocate_cpumask();
    ALWAYS_ASSERT(!::numa_node_to_cpus(node, bm));
    vector<unsigned> ret;
    for (int i = 0; i < numa_num_configured_cpus(); i++)
      if (numa_bitmask_isbitset(bm, i))
        ret.push_back(i);
    numa_free_cpumask(bm);
    return ret;
  }

  static vector<unsigned>
  exclude(const vector<unsigned> &cpus, unsigned nthds)
  {
    vector<unsigned> ret;
    for (auto n : cpus)
      if (n < nthds)
        ret.push_back(n);
    return ret;
  }

};

void
ycsb_do_test(abstract_db *db, int argc, char **argv)
{
  //nkeys = size_t(scale_factor * 1000.0);
  //ALWAYS_ASSERT(nkeys > 0);

  // parse options
  optind = 1;
  while (1) {
    static struct option long_options[] = {
      {"workload-mix"           , required_argument , 0 , 'w'},
      {"reps-per-tx"            , required_argument, 0                , 'r' },
      {"rmw-additional-reads"   , required_argument, 0                , 'a' },
      {"rmw-read-ratio"         , required_argument, 0                , 't' },
      {"zipf-theta"             , required_argument, 0                , 'z' },
      {"initial-table-size"     , required_argument, 0                , 's' },
      {0, 0, 0, 0}
    };
    int option_index = 0;
    int c = getopt_long(argc, argv, "w:r:a:t:z:s:", long_options, &option_index);
    if (c == -1)
      break;
    switch (c) {
    case 0:
      if (long_options[option_index].flag != 0)
        break;
      abort();
      break;

    case 'w':
      {
        const vector<string> toks = split(optarg, ',');
        ALWAYS_ASSERT(toks.size() == ARRAY_NELEMS(g_txn_workload_mix));
        unsigned s = 0;
        for (size_t i = 0; i < toks.size(); i++) {
          unsigned p = strtoul(toks[i].c_str(), nullptr, 10);
          ALWAYS_ASSERT(p >= 0 && p <= 100);
          s += p;
          g_txn_workload_mix[i] = p;
        }
        ALWAYS_ASSERT(s == 100);
      }
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
      nkeys = strtoul(optarg, NULL, 10);
      break;

    case '?':
      /* getopt_long already printed an error message. */
      exit(1);

    default:
      abort();
    }
  }

  ALWAYS_ASSERT(nkeys);

  // Both must be non-zero because we use a trace.
  ALWAYS_ASSERT(ops_per_worker);
  ALWAYS_ASSERT(max_runtime);

  if (verbose) {
    cerr << "ycsb settings:" << endl;
    cerr << "  workload_mix: "
         << format_list(g_txn_workload_mix, g_txn_workload_mix + ARRAY_NELEMS(g_txn_workload_mix)) << endl
         << "  initial user table size:    " << nkeys << endl
         << "  operations per transaction: " << g_reps_per_tx << endl
         << "  additional reads after RMW: " << g_rmw_additional_reads << endl
         << "  read ratio in RMW:          " << g_rmw_read_ratio << endl
         << "  zipf theta:                 " << g_zipf_theta << endl;
  }

  ycsb_bench_runner r(db);
  r.run();
}
