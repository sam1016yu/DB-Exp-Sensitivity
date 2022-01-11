#!/bin/bash

set +e


silo_ycsb(){
    threads=$1  # number of worker threads
    keys=$2    # number of KV pairs (unit: k pairs, 100 means 100k KV pairs)
    memory=$3  # memory allocated
    echo "Testing ycsb silo|Threads:$threads|Keys:$keys|Mem:$memory"
    ./out-perf.masstree/benchmarks/dbtest --bench ycsb --db-type ndb-proto2 --num-threads $threads --scale-factor $keys --txn-flags 1 --runtime 60 --bench-opts --workload-mix\ 80,0,20,0 --parallel-loading --numa-memory $memory
}



silo_ycsb 28 100 240G