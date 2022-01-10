#!/bin/bash

set +e
# set -x


silo_tpcc_neworder(){
    threads=$1
    warehouse=$2
    remote_per=$3
    echo "Testing Silo TPCC new_order only|Threads:$threads|#Warehouse:$warehouse|RemoteNewOrder:$remote_per"
    timeout -s SIGINT 10m ./out-perf.masstree/benchmarks/dbtest --bench tpcc --db-type ndb-proto2 --num-threads $threads --scale-factor $warehouse --txn-flags 1 --runtime 60 --bench-opts --workload-mix\ 100,0,0,0,0\ --new-order-remote-item-pct\ $remote_per --numa-memory 240G
}

silo_tpcc_all(){
    threads=$1
    warehouse=$2
    echo "Testing Silo TPCC all transaction|Threads:$threads|#Warehouse:$warehouse"
    timeout -s SIGINT 10m ./out-perf.masstree/benchmarks/dbtest --bench tpcc --db-type ndb-proto2 --num-threads $threads --scale-factor $warehouse --txn-flags 1 --runtime 60 --numa-memory 240G
}


silo_tpcc_NP(){
    threads=$1
    warehouse=$2
    mem=240G
    echo "Testing Silo TPCC 2T only|Threads:$threads|#Warehouse:$warehouse"
    timeout -s SIGINT 10m ./out-perf.masstree/benchmarks/dbtest --bench tpcc --db-type ndb-proto2 --num-threads $threads --scale-factor $warehouse --txn-flags 1 --runtime 60 --bench-opts --workload-mix\ 50,50,0,0,0 --numa-memory ${mem}
}





silo_ycsb(){
    threads=$1
    keys=$2
    memory=$3
    rw=$4
    echo "Testing ycsb silo|Threads:$threads|Keys:$keys|Mem:$memory|RW:$rw"
    ./out-perf.masstree/benchmarks/dbtest --bench ycsb --db-type ndb-proto2 --num-threads $threads --scale-factor $keys --txn-flags 1 --runtime 60 --bench-opts --workload-mix\ $rw --parallel-loading --numa-memory $memory
}





# for key_num in 128 256 512 1024 2048
# for key_num in 2048 8192
# do
#     for thd in 1 2 4 8 16 28
#     do
#         for read in 0 10 20 30 40 50 60 70 80 90 100
#         do
#             write=$((100-$read))
#             silo_ycsb $thd $key_num 240G ${read},0,${write},0 2>&1 | tee -a ./logs/ycsb_0913.out
#         done
#     done
# done

# date | tee -a ./logs/TPCC_0916_remote.out
# for thd in 1 2 4 8 16 28
# do
# for WH in 1 2 4 8 16 32 64 128 256 320 512 
# do
# for P in 0 10 20 30 40 50
# do
# silo_tpcc_neworder $thd $WH $P 2>&1 | tee -a ./logs/TPCC_0916_remote.out
# date | tee -a ./logs/TPCC_0916_remote.out
# done
# done
# done




for thd in 2 14
do
for read in  50 100
do
    write=$((100-$read))
    silo_ycsb ${thd} 2097512 240G ${read},0,${write},0 2>&1 | tee -a ./logs/ycsb_0922.out
done
done