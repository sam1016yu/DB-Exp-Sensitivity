# drtm

Please check the original repository: https://github.com/SJTU-IPADS/drtm.git for more detailed information and introduction.



## Dependencies

- `zeromq` 4.0.5 or higher

- Intel processors with RTM enabled

- Mellanox OFED v3.0-2.0.1 stack or higher

- ptpd

- SSMalloc



## Build

In project main directory, do:

```
make clean && make C=3 dbtest -j
```



## Run

For our test environment, please check README in the project main directory.

To run dbtest:

```
dbtest --num-threads <nthreads> --scale_factor <sf> --ops-per-worker <ops> --bench-opts <bench-opts> --config <hostfile> --total-partition <npartitions> --current_partition <partition_id> --bench tpcc --db-type ndb-proto2 --txn-flags 1 --retry-aborted-transactions
```

For example, in a cluster of 2 nodes, to run first server with default TPCC transaction ratios:

```
dbtest --num-threads 4 --scale-factor 4 --bench tpcc --db-type ndb-proto2 --txn-flags 1 --ops-per-worker 10000 --bench-opts "--w 45,43,4,4,4 -r 0" --verbose --retry-aborted-transactions --config hostfile --total-partition 2 --current-partition 0
```

And second server:
```
dbtest --num-threads 4 --scale-factor 4 --bench tpcc --db-type ndb-proto2 --txn-flags 1 --ops-per-worker 10000 --bench-opts "--w 45,43,4,4,4 -r 0" --verbose --retry-aborted-transactions --config hostfile --total-partition 2 --current-partition 1
```

Please check the README in original repository for hostfile format.