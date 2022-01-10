#!/bin/bash

set +e
# set -x


silo_tpcc_neworder(){
    threads=$1
    warehouse=$2
    remote_per=$3
    echo "Testing Silo TPCC new_order only|Threads:$threads|#Warehouse:$warehouse|RemoteNewOrder:$remote_per"
    timeout -s SIGINT 20m ./out-perf.masstree/benchmarks/dbtest --bench tpcc --db-type ndb-proto2 --num-threads $threads --scale-factor $warehouse --txn-flags 1 --runtime 60 --bench-opts --workload-mix\ 100,0,0,0,0\ --new-order-remote-item-pct\ $remote_per --numa-memory 200G
}

pt_tpcc_neworder(){
    threads=$1
    warehouse=$2
    remote_per=$3
    echo "Testing Partitioned Store TPCC new_order only|Threads:$threads|#Warehouse:$warehouse|RemoteNewOrder:$remote_per"
    timeout -s SIGINT 20m ./out-perf.masstree/benchmarks/dbtest --bench tpcc --db-type kvdb-st --num-threads $threads --scale-factor $warehouse --txn-flags 1 --runtime 60 --bench-opts --workload-mix\ 100,0,0,0,0\ --enable-separate-tree-per-partition\ --enable-partition-locks\ --new-order-remote-item-pct\ $remote_per --numa-memory 200G
    }

silo_tpcc_all(){
    threads=$1
    warehouse=$2
    echo "Testing Silo TPCC all transaction|Threads:$threads|#Warehouse:$warehouse"
    timeout -s SIGINT 20m ./out-perf.masstree/benchmarks/dbtest --bench tpcc --db-type ndb-proto2 --num-threads $threads --scale-factor $warehouse --txn-flags 1 --runtime 60 --numa-memory 200G
}


pt_tpcc_all(){
    threads=$1
    warehouse=$2
    echo "Testing  Partitioned Store TPCC  all transaction|Threads:$threads|#Warehouse:$warehouse"
    timeout -s SIGINT 20m ./out-perf.masstree/benchmarks/dbtest --bench tpcc --db-type kvdb-st --num-threads $threads --scale-factor $warehouse --txn-flags 1 --runtime 60 --bench-opts \ --enable-separate-tree-per-partition\ --enable-partition-locks --numa-memory 200G
}



silo_tpcc_NP(){
    threads=$1
    warehouse=$2
    echo "Testing Silo TPCC 2T only|Threads:$threads|#Warehouse:$threads"
    timeout -s SIGINT 20m ./out-perf.masstree/benchmarks/dbtest --bench tpcc --db-type ndb-proto2 --num-threads $threads --scale-factor $threads --txn-flags 1 --runtime 60 --bench-opts --workload-mix\ 50,50,0,0,0 --numa-memory 200G
}


pt_tpcc_NP(){
    threads=$1
    warehouse=$2
    echo "Testing Partitioned Store TPCC 2T only|Threads:$threads|#Warehouse:$threads"
    timeout -s SIGINT 20m ./out-perf.masstree/benchmarks/dbtest --bench tpcc --db-type kvdb-st --num-threads $threads --scale-factor $warehouse --txn-flags 1 --runtime 60 --bench-opts --workload-mix\ 50,50,0,0,0\ --enable-separate-tree-per-partition\ --enable-partition-locks --numa-memory 200G
}




silo_all(){
    threads=$1
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
    # silo_tpcc_all $threads $warehouse
    # pt_tpcc_all $threads $warehouse
    # date
    # # silo_tpcc_NP $threads $warehouse
    # pt_tpcc_NP $threads $warehouse
    # date

    for remote_per in 20 30 40 50 60 70 80 90 100
    do
        # silo_tpcc_neworder $threads $warehouse $remote_per
        pt_tpcc_neworder $threads $warehouse $remote_per
        date
    done
}


mkdir -p logs

for round in 1 2 3
do
    logname="./logs/TPCC_1129_round${round}.out"

    # date | tee -a $logname
    # for thd in 1 2 4 8 16 28
    # do
    #     for WH in 512
    #     do
    #         silo_all $thd $WH 2>&1 | tee -a $logname
    #     done
    # done


    date | tee -a $logname
    for thd in 1 28
    do
    for WH in 1 2 4 8 16 32 64 128 256 512
    do
    pt_all $thd $WH 2>&1 | tee -a $logname
    done
    done
done


# for th in 20 24 28 32
# do
#     silo_tpcc_NP $th 2>&1 | tee -a ./logs/tpcc_NP.out
# done



# truncate -s 0 65.out
# pt_tpcc_neworder 28 65 50 2>&1 | tee -a 65.out


# truncate -s 0 64.out
# pt_tpcc_neworder 28 64 50 2>&1 | tee -a 64.out


# truncate -s 0 128.out
# pt_tpcc_neworder 28 128 50 2>&1 | tee -a 128.out

# truncate -s 0 256.out
# silo_tpcc_neworder 16 256 50 200 2>&1 | tee -a 256.out


# truncate -s 0 512.out
# silo_tpcc_neworder 16 1800 50 #2>&1 | tee -a 512.out


# truncate -s 0 512_4.out
# silo_tpcc_neworder 4 500 10 2>&1 | tee -a 512_4.out

# silo_tpcc_neworder 8 128 50 2>&1 | tee -a 128_silo.out


# silo_tpcc_neworder 16 100 50 5 2>&1 | tee -a 5G100.out

# silo_tpcc_neworder 16 100 50 8 2>&1 | tee -a 8G100.out

# silo_tpcc_neworder 16 100 50 10 2>&1 | tee -a 10G100.out

# silo_tpcc_neworder 16 100 50 12 2>&1 | tee -a 12G100.out

# silo_tpcc_neworder 28 1200 50 200 # 2>&1 | tee -a 100G100.out


# silo_tpcc_neworder 16 1800 50 200 2>&1 | tee -a 1800.out

# silo_tpcc_neworder 16 2000 50 200 2>&1 | tee -a 2000.out

# silo_tpcc_neworder 16 2200 50 200 2>&1 | tee -a 2500.out

# silo_tpcc_neworder 16 2200 50 220 2>&1 | tee -a 2500.out




