#!/bin/bash

set +e
# set -x


silo_tpcc_neworder(){
    # new order only
    # silo 
    threads=$1  # number of threads
    warehouse=$2  # number of WH
    remote_per=$3 # percentage of cross-WH txns
    echo "Testing Silo TPCC new_order only|Threads:$threads|#Warehouse:$warehouse|RemoteNewOrder:$remote_per"
    timeout -s SIGINT 20m ./out-perf.masstree/benchmarks/dbtest --bench tpcc --db-type ndb-proto2 --num-threads $threads --scale-factor $warehouse --txn-flags 1 --runtime 60 --bench-opts --workload-mix\ 100,0,0,0,0\ --new-order-remote-item-pct\ $remote_per --numa-memory 200G
}

pt_tpcc_neworder(){
    # new order only
    # partitioned store
    threads=$1  # number of threads
    warehouse=$2  # number of WH
    remote_per=$3  # percentage of cross-WH txns
    echo "Testing Partitioned Store TPCC new_order only|Threads:$threads|#Warehouse:$warehouse|RemoteNewOrder:$remote_per"
    timeout -s SIGINT 20m ./out-perf.masstree/benchmarks/dbtest --bench tpcc --db-type kvdb-st --num-threads $threads --scale-factor $warehouse --txn-flags 1 --runtime 60 --bench-opts --workload-mix\ 100,0,0,0,0\ --enable-separate-tree-per-partition\ --enable-partition-locks\ --new-order-remote-item-pct\ $remote_per --numa-memory 200G
    }



threads=32
for WH in 32 160
do
for remote_per in {0..100..10}
do
 silo_tpcc_neworder $threads $WH $remote_per
 pt_tpcc_neworder $threads $WH $remote_per

done
done