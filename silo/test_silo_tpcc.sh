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



silo_tpcc_all(){
    # 5 types transactions
    # silo 
    threads=$1  # number of threads
    warehouse=$2  # number of WH
    echo "Testing Silo TPCC all transaction|Threads:$threads|#Warehouse:$warehouse"
    timeout -s SIGINT 20m ./out-perf.masstree/benchmarks/dbtest --bench tpcc --db-type ndb-proto2 --num-threads $threads --scale-factor $warehouse --txn-flags 1 --runtime 60 --numa-memory 200G
}


pt_tpcc_all(){
    # 5 types transactions
     # partitioned store
    threads=$1  # number of threads
    warehouse=$2  # number of WH
    echo "Testing  Partitioned Store TPCC  all transaction|Threads:$threads|#Warehouse:$warehouse"
    timeout -s SIGINT 20m ./out-perf.masstree/benchmarks/dbtest --bench tpcc --db-type kvdb-st --num-threads $threads --scale-factor $warehouse --txn-flags 1 --runtime 60 --bench-opts \ --enable-separate-tree-per-partition\ --enable-partition-locks --numa-memory 200G
}



silo_tpcc_NP(){
     # 2 types transactions
     # silo 
    threads=$1  # number of threads
    warehouse=$2  # number of WH
    echo "Testing Silo TPCC 2T only|Threads:$threads|#Warehouse:$threads"
    timeout -s SIGINT 20m ./out-perf.masstree/benchmarks/dbtest --bench tpcc --db-type ndb-proto2 --num-threads $threads --scale-factor $threads --txn-flags 1 --runtime 60 --bench-opts --workload-mix\ 50,50,0,0,0 --numa-memory 200G
}


pt_tpcc_NP(){
      # 2 types transactions
       # partitioned store
    threads=$1  # number of threads
    warehouse=$2  # number of WH
    echo "Testing Partitioned Store TPCC 2T only|Threads:$threads|#Warehouse:$threads"
    timeout -s SIGINT 20m ./out-perf.masstree/benchmarks/dbtest --bench tpcc --db-type kvdb-st --num-threads $threads --scale-factor $warehouse --txn-flags 1 --runtime 60 --bench-opts --workload-mix\ 50,50,0,0,0\ --enable-separate-tree-per-partition\ --enable-partition-locks --numa-memory 200G
}





silo_all(){
    threads=$1  # number of threads
    warehouse=$2
    silo_tpcc_all $threads $warehouse
    date
    # pt_tpcc_all $threads $warehouse
    silo_tpcc_NP $threads $warehouse
    date
    # pt_tpcc_NP $threads $warehouse

    for remote_per in 0 10 20 30 40 50 60 70 80 90 100
    do
        silo_tpcc_neworder $threads $warehouse $remote_per
        date
        # pt_tpcc_neworder $threads $warehouse $remote_per
    done
}


pt_all(){
    threads=$1
    warehouse=$2
    for remote_per in 20 30 40 50 60 70 80 90 100
    do
        # silo_tpcc_neworder $threads $warehouse $remote_per
        pt_tpcc_neworder $threads $warehouse $remote_per
        date
    done
}

silo_all 10 20
pt_all 10 20



