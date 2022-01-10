#!/bin/bash

set +e
# set -x


silo_ycsb(){
    threads=$1
    keys=$2
    memory=$3
    echo "Testing ycsb silo|Threads:$threads|Keys:$keys|Mem:$memory"
    ./out-perf.masstree/benchmarks/dbtest --bench ycsb --db-type ndb-proto2 --num-threads $threads --scale-factor $keys --txn-flags 1 --runtime 60 --bench-opts --workload-mix\ 80,0,20,0 --parallel-loading --numa-memory $memory
}



# for key_num in 320000 640000
# do
#     for thd in 1 4 8 12 16 20 24 28 32
#     do
#         silo_ycsb $thd $key_num 240G 2>&1 | tee -a ./logs/ycsb.out
#     done
# done

# for key_num in 1000 2000 4000 8000 10000 20000 40000 80000 100000
# do
#     for thd in 1 4 8 12 16 20 24 28 32
#     do
#         silo_ycsb $thd $key_num 240G 2>&1 | tee -a ./logs/ycsb.out
#     done
# done


silo_ycsb 28 1048576 240G