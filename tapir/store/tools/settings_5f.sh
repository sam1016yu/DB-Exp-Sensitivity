#!/bin/bash


trap '{
for i in `seq 1 20`; do 
ssh node${i} "sudo killall -9 server; sudo killall -9 timeserver"; 
done 

echo "Killed all server and timeserver"
exit 1
}' INT

do_test(){
for keys in 1 10 20 30 40
do
    # for read in 0 0.2 0.4 0.6 0.8 1
    for read in 0.5
    do
    python3 gen_workload.py $keys $read zipfian
        for nshard in 1
        do
            for nrep in 3
            do
                for thd in  5 10 15 20 25
                do 
                    date
                    echo "|Keys:$keys|read:$read|shards:$nshard|replica:$nrep|client threads:$thd"
                    fname="keys_${keys}_read_${read}_nshard_${nshard}_nrep_${nrep}_thd_${thd}.log"
                    timeout -s SIGINT 15m ./tapir_autoconfig.sh $thd $nshard $nrep $fname
                done
            done
        done
    done
done
}



for i in `seq 1 20`; do 
	ssh node${i} "sudo killall -9 server; sudo killall -9 timeserver"; 
done 

do_test 2>&1 | tee -a ../../logs/ycsb_1006.out