# DB-Exp-Sensitivity

This is the repository for VLDB'22 submission ***A Study of Database Performance Sensitivity to Experiment Settings***.

## Testbed hardware configurations

see [hardware_config](hardware_config.md)

## Source code

Please refer to the README in each folder to see how to setup environment, build and run each work. 

## Settings corresponding to figures in the paper

*add file specifically for the settings used in each figures, and link to the file here*

### TPC-C

- 1a: [Silo (32 worker threads)]()
- 1b: [Aria (12 worker threads and default cross-warehouse rate)](aria/settings_1b.sh)
- 1c: [Janus (default cross-warehouse rate).](janus/run_batch.sh)
- 1d: [Cicada (default cross-warehouse rate)](cicada/run_exp_custom.py)
- 1e: [Star (12 worker threads per node)]()
- 1f: [GAM (4 worker threads per node)]()
- 3: [Throughput of Janus and 2PL with extra network latencies (6 warehouses and default cross-warehouse rate)]()
- 4: [Throughputs of H2 with interactive transactions and stored procedure]()
- 5a : [Silo (#warehouse=#worker and no cross-warehouse transactions).]()
- 5b: [Cicada (#warehouse=#worker and default cross-warehouse rate)](cicada/run_exp_custom.py)
- 5c: [Janus (6 warehouses and default cross-warehouse rate).](janus/run_batch.sh)

### YCSB
- 6a: [Cicada with 50% reads and 10M KVs](cicada/run_exp_custom.py)
- 6b: [Cicada with 50% reads and 40M KVs](cicada/run_exp_custom.py)
- 6c: [Cicada with 95% reads and 10M KVs](cicada/run_exp_custom.py)
- 6d: [HERD with uniform distribution]()
- 6e: [HERD with Zipfian (0.99)]()
- 6f: [TAPIR with 50% read and Zipfian (0.99)]()