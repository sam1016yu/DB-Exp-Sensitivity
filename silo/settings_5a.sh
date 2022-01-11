#!/bin/bash

silo_tpcc_all(){
    # 5 types transactions
    # silo 
    threads=$1  # number of threads
    warehouse=$2  # number of WH
    echo "Testing Silo TPCC all transaction|Threads:$threads|#Warehouse:$warehouse"
    timeout -s SIGINT 20m ./out-perf.masstree/benchmarks/dbtest --bench tpcc --db-type ndb-proto2 --num-threads $threads --scale-factor $warehouse --txn-flags 1 --runtime 60 --numa-memory 200G
}




silo_tpcc_NP(){
     # 2 types transactions
     # silo 
    threads=$1  # number of threads
    warehouse=$2  # number of WH
    echo "Testing Silo TPCC 2T only|Threads:$threads|#Warehouse:$threads"
    timeout -s SIGINT 20m ./out-perf.masstree/benchmarks/dbtest --bench tpcc --db-type ndb-proto2 --num-threads $threads --scale-factor $threads --txn-flags 1 --runtime 60 --bench-opts --workload-mix\ 50,50,0,0,0 --numa-memory 200G
}




for $thd in 1 4 8 12 16 20 24 28 32
do
 silo_tpcc_all $thd $thd
 silo_tpcc_NP  $thd $thd
done