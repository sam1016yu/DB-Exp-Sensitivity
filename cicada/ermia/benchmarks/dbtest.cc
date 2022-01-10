#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <utility>
#include <string>
#include <set>

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>

#include "../dbcore/sm-alloc.h"
#include "../dbcore/sm-config.h"
#include "../dbcore/sm-log-recover-impl.h"
#include "../dbcore/sm-thread.h"
#include "bench.h"
#include "ndb_wrapper.h"
//#include "kvdb_wrapper.h"
//#include "kvdb_wrapper_impl.h"

#if defined(SSI) && defined(SSN)
#error "SSI + SSN?"
#endif

using namespace std;
using namespace util;

static vector<string>
split_ws(const string &s)
{
  vector<string> r;
  istringstream iss(s);
  copy(istream_iterator<string>(iss),
       istream_iterator<string>(),
       back_inserter<vector<string>>(r));
  return r;
}

int
main(int argc, char **argv)
{
  abstract_db *db = NULL;
  void (*test_fn)(abstract_db *, int argc, char **argv) = NULL;
  string bench_type = "ycsb";
  char *curdir = get_current_dir_name();
  string bench_opts;
  free(curdir);
  int saw_run_spec = 0;
  string replay_mode("oid");

  while (1) {
    static struct option long_options[] =
    {
      {"verbose"                    , no_argument       , &verbose                   , 1}   ,
      {"parallel-loading"           , no_argument       , &enable_parallel_loading   , 1}   ,
      {"slow-exit"                  , no_argument       , &slow_exit                 , 1}   ,
      {"retry-aborted-transactions" , no_argument       , &retry_aborted_transaction , 1}   ,
      {"backoff-aborted-transactions" , no_argument     , &backoff_aborted_transaction , 1}   ,
      {"bench"                      , required_argument , 0                          , 'b'} ,
      {"scale-factor"               , required_argument , 0                          , 's'} ,
      {"num-threads"                , required_argument , 0                          , 't'} ,
      {"txn-flags"                  , required_argument , 0                          , 'f'} ,
      {"runtime"                    , required_argument , 0                          , 'r'} ,
      {"max-runtime"                , required_argument , 0                          , 'R'} ,
      {"ops-per-worker"             , required_argument , 0                          , 'n'} ,
      {"bench-opts"                 , required_argument , 0                          , 'o'} ,
      {"log-dir"                    , required_argument , 0                          , 'l'} ,
      {"log-segment-mb"             , required_argument , 0                          , 'e'} ,
      {"log-buffer-mb"              , required_argument , 0                          , 'u'} ,
      {"recovery-warm-up"           , required_argument , 0                          , 'w'} ,
      {"enable-chkpt"               , no_argument       , &enable_chkpt              , 1} ,
      {"null-log-device"            , no_argument       , &sysconf::null_log_device  , 1} ,
      {"parallel-recovery-by"       , required_argument , 0                          , 'c'},
      {"node-memory-gb"             , required_argument , 0                          , 'p'},
      {"enable-gc"                  , no_argument       , &sysconf::enable_gc        , 1},
      {"tmpfs-dir"                  , required_argument , 0                          , 'm'},
#if defined(SSI) || defined(SSN)
      {"safesnap"                   , no_argument       , &sysconf::enable_safesnap  , 1},
#ifdef SSI
      {"ssi-read-only-opt"          , no_argument       , &sysconf::enable_ssi_read_only_opt, 1},
#endif
#ifdef SSN
      {"ssn-read-opt-threshold"     , required_argument , 0                          , 'h'},
#endif
#endif
      {0, 0, 0, 0}
    };
    int option_index = 0;
    int c = getopt_long(argc, argv, "b:s:t:B:f:r:n:o:m:l:e:u:w:x:p:m:", long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
    case 0:
      if (long_options[option_index].flag != 0)
        break;
      abort();

    case 'c':
      replay_mode = string(optarg);
      if (replay_mode == "oid") {
        sysconf::recover_functor = new parallel_oid_replay;
      } else if (replay_mode == "file") {
        sysconf::recover_functor = new parallel_file_replay;
      } else {
        std::cout << "Invalid parallel replay mode: " << replay_mode << "\n";
        abort();
      }
      break;

    case 'p':
      sysconf::node_memory_gb = strtoul(optarg, NULL, 10);
      break;

    case 'b':
      bench_type = optarg;
      break;

    case 's':
      scale_factor = strtod(optarg, NULL);
      break;

    case 't':
      sysconf::worker_threads = strtoul(optarg, NULL, 10);
      ALWAYS_ASSERT(sysconf::worker_threads > 0);
      break;

#ifdef SSN
    case 'h':
      sysconf::ssn_read_opt_threshold = strtoul(optarg, NULL, 16);
      break;
#endif

    case 'f':
      txn_flags = strtoul(optarg, NULL, 10);
      break;

    case 'r':
      ALWAYS_ASSERT(!saw_run_spec);
      saw_run_spec = 1;
      runtime = strtoul(optarg, NULL, 10);
      ALWAYS_ASSERT(runtime > 0);
      run_mode = RUNMODE_TIME;
      break;

    case 'R':
      max_runtime = strtoul(optarg, NULL, 10);
      break;

    case 'w':
      if (strcmp(optarg, "eager") == 0)
        sysconf::recovery_warm_up_policy = sysconf::WARM_UP_EAGER;
      else if (strcmp(optarg, "lazy") == 0)
        sysconf::recovery_warm_up_policy = sysconf::WARM_UP_LAZY;
      else
        sysconf::recovery_warm_up_policy = sysconf::WARM_UP_NONE;
      break;

    case 'n':
      ALWAYS_ASSERT(!saw_run_spec);
      saw_run_spec = 1;
      ops_per_worker = strtoul(optarg, NULL, 10);
      ALWAYS_ASSERT(ops_per_worker > 0);
      run_mode = RUNMODE_OPS;
      break;

    case 'o':
      bench_opts = optarg;
      break;

    case 'l':
      sysconf::log_dir = std::string(optarg);
      break;

    case 'm':
      sysconf::tmpfs_dir = string(optarg);
      break;

    case 'e':
      sysconf::log_segment_mb = strtoul(optarg, NULL, 10);
      ALWAYS_ASSERT(sysconf::log_segment_mb);
      break;

    case 'u':
      sysconf::log_buffer_mb = strtoul(optarg, NULL, 10);
      ALWAYS_ASSERT(sysconf::log_buffer_mb);
      break;

    case '?':
      /* getopt_long already printed an error message. */
      exit(1);

    default:
      abort();
    }
  }

 if (bench_type == "ycsb")
    test_fn = ycsb_do_test;
  else if (bench_type == "tpcc")
    test_fn = tpcc_do_test;
  else if (bench_type == "tpce")
    test_fn = tpce_do_test;
  else
    ALWAYS_ASSERT(false);

  // parallel replay by oid partitions by default
  if (not sysconf::recover_functor) {
    sysconf::recover_functor = new parallel_oid_replay;
  }

  sysconf::init();
  if (sysconf::log_dir.empty()) {
    cerr << "[ERROR] no log dir specified" << endl;
    return 1;
  }

#ifndef NDEBUG
  cerr << "WARNING: benchmark built in DEBUG mode!!!" << endl;
#endif

#ifndef NDEBUG
  cerr << "WARNING: invariant checking is enabled - should disable for benchmark" << endl;
#ifdef PARANOID_CHECKING
  cerr << "  *** Paranoid checking is enabled ***" << endl;
#endif
#endif

  if (verbose) {
#ifdef SSI
    printf("System: SSI\n");
#elif defined(SSN)
#ifdef RC
    printf("System: RC+SSN\n");
#elif defined RC_SPIN
    printf("System: RC_SPIN+SSN\n");
#else
    printf("System: SI+SSN\n");
#endif
#else
    printf("System: SI\n");
#endif
#ifdef PHANTOM_PROT
    printf("Phantom protection: on\n");
#else
    printf("Phantom protection: off\n");
#endif
    cerr << "Database Benchmark:"                           << endl;
    cerr << "  pid: " << getpid()                           << endl;
    cerr << "settings:"                                     << endl;
    cerr << "  node-memory : " << sysconf::node_memory_gb << "GB" << endl;
    cerr << "  par-loading : " << enable_parallel_loading   << endl;
    cerr << "  slow-exit   : " << slow_exit                 << endl;
    cerr << "  retry-txns  : " << retry_aborted_transaction << endl;
    cerr << "  backoff-txns: " << backoff_aborted_transaction << endl;
    cerr << "  bench       : " << bench_type                << endl;
    cerr << "  scale       : " << scale_factor              << endl;
    cerr << "  num-threads : " << sysconf::worker_threads   << endl;
    cerr << "  numa-nodes  : " << sysconf::numa_nodes       << endl;
    cerr << "  txn-flags   : " << hexify(txn_flags)         << endl;
    if (run_mode == RUNMODE_TIME)
      cerr << "  runtime     : " << runtime                 << endl;
    else
      cerr << "  ops/worker  : " << ops_per_worker          << endl;
    cerr << "  max-runtime : " << max_runtime               << endl;
#ifdef USE_VARINT_ENCODING
    cerr << "  var-encode  : yes"                           << endl;
#else
    cerr << "  var-encode  : no"                            << endl;
#endif
    cerr << "  tmpfs-dir   : " << sysconf::tmpfs_dir        << endl;
    cerr << "  log-dir     : " << sysconf::log_dir          << endl;
    cerr << "  log-segment-mb: " << sysconf::log_segment_mb   << endl;
    cerr << "  log-buffer-mb: " << sysconf::log_buffer_mb    << endl;
    cerr << "  recovery-warm-up: ";
    if (sysconf::recovery_warm_up_policy == sysconf::WARM_UP_NONE)
      cerr << "none";
    else if (sysconf::recovery_warm_up_policy == sysconf::WARM_UP_LAZY)
      cerr << "lazy";
    else {
      ALWAYS_ASSERT(sysconf::recovery_warm_up_policy == sysconf::WARM_UP_EAGER);
      cerr << "eager";
    }
    cerr << endl;
    cerr << "  parallel-recover-by: " << replay_mode         << endl;
    cerr << "  enable-chkpt    : " << enable_chkpt           << endl;
    cerr << "  enable-gc       : " << sysconf::enable_gc     << endl;
    cerr << "  null-log-device : " << sysconf::null_log_device << endl;

    cerr << "system properties:" << endl;
    cerr << "  btree_internal_node_size: " << concurrent_btree::InternalNodeSize() << endl;
    cerr << "  btree_leaf_node_size    : " << concurrent_btree::LeafNodeSize() << endl;

#ifdef TUPLE_PREFETCH
    cerr << "  tuple_prefetch          : yes" << endl;
#else
    cerr << "  tuple_prefetch          : no" << endl;
#endif

#ifdef BTREE_NODE_PREFETCH
    cerr << "  btree_node_prefetch     : yes" << endl;
#else
    cerr << "  btree_node_prefetch     : no" << endl;
#endif
#if defined(SSN) || defined(SSI)
    cerr << "  SSN/SSI safe snapshot   : " << sysconf::enable_safesnap << endl;
#endif
#ifdef SSI
    cerr << "  SSI read-only optimization: " << sysconf::enable_ssi_read_only_opt << endl;
#endif
#ifdef SSN
    cerr << "  SSN read optimization threshold: 0x" << hex << sysconf::ssn_read_opt_threshold << dec << endl;
#endif
  }

  MM::prepare_node_memory();
  vector<string> bench_toks = split_ws(bench_opts);
  argc = 1 + bench_toks.size();
  char *new_argv[argc];
  new_argv[0] = (char *) bench_type.c_str();
  for (size_t i = 1; i <= bench_toks.size(); i++)
    new_argv[i] = (char *) bench_toks[i - 1].c_str();

  // Must have everything in CONF ready by this point (ndb-wrapper's ctor will use them)
  sysconf::sanity_check();
  db = new ndb_wrapper();
  test_fn(db, argc, new_argv);
  delete db;
  return 0;
}
