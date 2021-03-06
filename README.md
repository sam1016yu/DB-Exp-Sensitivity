# DB-Exp-Sensitivity

[A Study of Database Performance Sensitivity to Experiment Settings](http://vldb.org/pvldb/vol15/p1439-wang.pdf)

Yang Wang, Miao Yu, Yujie Hui, Fang Zhou, Yuyang Huang, Rui Zhu, Xueyuan Ren, Tianxi Li, and Xiaoyi Lu.

**Proceedings of the VLDB Endowment, Vol. 15, No. 7**

## Testbed hardware configurations

see [hardware_config](hardware_config.md)

## Source code

Please refer to the README in each folder to see how to setup environment, build and run each work. 

## Settings, raw data and plot script for figures in the paper

You can find the correlated raw data in the plot script. (All the plot script produced by [gnuplot](http://www.gnuplot.info))

### TPC-C

- 1a: Silo (32 worker threads)
  - [setting script](silo/settings_1a.sh), [plot script](rawdata/silo/silo_tpcc_remote_combined.p)
- 1b: Aria (12 worker threads and default cross-warehouse rate)
  - [setting script](aria/settings_1b.sh), [plot script](rawdata/aria/aria_tpcc_dist_15.p)
- 1c: Janus (default cross-warehouse rate).
  - [setting script](janus/run_batch.sh), [plot script](rawdata/janus/janus_6s2u_vs_12s4u_dist.p)
- 1d: Cicada (default cross-warehouse rate)
  - [setting script](cicada/run_exp_custom.py), [plot script](rawdata/cicada/tpcc_compare_ware.p)
- 1e: Star (12 worker threads per node)
  - [setting script](star/run-star.sh), [plot script](rawdata/star/star_tpcc.p)
- 1f: GAM (4 worker threads per node)
  - [setting script](gam/database/tpcc/run-gam.sh), [plot script](rawdata/gam/gam_c4.p)
- 3: Throughput of Janus and 2PL with extra network latencies (6 warehouses and default cross-warehouse rate)
  - [setting script](janus/run_batch.sh), [plot script](rawdata/janus/janus_6s2u_10ms_vs_100ms.p)
- 4: Throughputs of H2 with interactive transactions and stored procedure
  - [setting script](H2/), [plot script](rawdata/h2/h2_whhistogram.p)
- 5a : Silo (#warehouse=#worker and no cross-warehouse transactions).
  - [setting script](silo/settings_5a.sh), [plot script](rawdata/silo/silo_tpcc_local_5vs2.p)
- 5b: Cicada (#warehouse=#worker and default cross-warehouse rate)
  - [setting script](cicada/run_exp_custom.py), [plot script](rawdata/cicada/tpcc_1w1th_5vs2.p)
- 5c: Janus (6 warehouses and default cross-warehouse rate).
  - [setting script](janus/run_batch.sh), [plot script](rawdata/janus/janus_6s_2u_dist_5vs2.p)

### YCSB
- 6a: Cicada with 50% reads and 10M KVs
  - [setting script](cicada/run_exp_custom.py), [plot script](rawdata/cicada/6b_ycsb_28thd_write_10M.p)
- 6b: Cicada with 50% reads and 40M KVs
  - [setting script](cicada/run_exp_custom.py), [plot script](rawdata/cicada/6b_ycsb_28thd_write_40M.p)
- 6c: Cicada with 95% reads and 10M KVs
  - [setting script](cicada/run_exp_custom.py), [plot script](rawdata/cicada/6c_ycsb_28thd_read_10M.p)
- 6d: HERD with uniform distribution
  - [setting script](HERD/run-herd.sh), [plot script](rawdata/herd/herd_uniform.p)
- 6e: HERD with Zipfian (0.99)
  - [setting script](HERD/run-herd.sh), [plot script](rawdata/herd/herd_zipf.p)
- 6f: TAPIR with 50% read and Zipfian (0.99)
  - [setting script](tapir/store/tools/settings_5f.sh), [plot script](rawdata/tapir/tapir-max-throughput.gpi)
