#!/bin/bash

set +e

# num_threads=12

test_aria(){
    warehouse=$1
    threads=$2
    dist=$3
    echo "Aria|#Warehouse:$warehouse|Threads:$threads|Dist:$dist"
    timeout -s SIGTSTP 10m ./bench_tpcc --logtostderr=1 --id=0 --servers="127.0.0.1:9000" --protocol=Aria --partition_num=$warehouse --threads=$threads --batch_size=500 --query=mixed --neworder_dist=$dist --payment_dist=$dist
}


test_Bohm(){
    warehouse=$1
    threads=$2
    dist=$3
    echo "Bohm|#Warehouse:$warehouse|Threads:$threads|Dist:$dist"
    timeout -s SIGTSTP 10m ./bench_tpcc --logtostderr=1 --id=0 --servers="127.0.0.1:9000" --protocol=Bohm --partition_num=$warehouse --threads=$threads --batch_size=500 --query=mixed --neworder_dist=$dist --payment_dist=$dist --mvcc=True --bohm_single_spin=True --same_batch=False
}



test_Pwv(){
    warehouse=$1
    threads=$2
    dist=$3
    echo "Pwv|#Warehouse:$warehouse|Threads:$threads|Dist:$dist"
    timeout -s SIGTSTP 10m ./bench_tpcc --logtostderr=1 --id=0 --servers="127.0.0.1:9000" --protocol=Pwv --partition_num=$warehouse --threads=$threads --batch_size=500 --query=mixed --neworder_dist=$dist --payment_dist=$dist --same_batch=False
}



test_Calvin(){
    warehouse=$1
    threads=$2
    dist=$3
    locks=$4
    echo "Calvin|#Warehouse:$warehouse|Threads:$threads|Dist:$dist|Locks:$locks"
    timeout -s SIGTSTP 10m ./bench_tpcc --logtostderr=1 --id=0 --servers="127.0.0.1:9000" --protocol=Calvin --partition_num=$warehouse --threads=$threads --batch_size=500 --query=mixed --neworder_dist=$dist --payment_dist=$dist --lock_manager=$locks --replica_group=1 --same_batch=False
}



test_ariaFB(){
    warehouse=$1
    threads=$2
    dist=$3
    locks=$4
    echo "AriaFB|#Warehouse:$warehouse|Threads:$threads|Dist:$dist|Locks:$locks"
    timeout -s SIGTSTP 10m ./bench_tpcc --logtostderr=1 --id=0 --servers="127.0.0.1:9000" --protocol=AriaFB --partition_num=$warehouse --threads=$threads --batch_size=500 --query=mixed --neworder_dist=$dist --payment_dist=$dist --same_batch=False --ariaFB_lock_manager=$locks
}


tpcc_all(){
    for wh in 1 2 4 6 8 10 12 36 60 84 108 132 156 180
        num_threads=12
        dist=15
        test_aria $wh $num_threads $dist
        test_Bohm $wh $num_threads $dist
        test_Pwv $wh $num_threads $dist
        for locks in 1 2 4 6
        do
            if [ $(($locks+1)) -gt $num_threads ]
            then
                continue
            fi
            test_Calvin $wh $num_threads $dist $locks
            test_ariaFB $wh $num_threads $dist $locks
        done
    done
}


tpcc_all 2>&1 | tee -a ./tpcc_new_run${run}.out