#!/bin/bash
duration=30
prefix="1103test"

set -v

function write_concurrent {
	# number of concurrent clients
	echo -e "n_concurrent: $1\n" > /tmp/concurrent.yml
}

function new_experiment {
	# clean logs
	rm -rf tmp/* log/* 
	tar -czvf ~/${1}.tgz archive && rm -rf archive && mkdir -p archive
	printf '=%.0s' {1..40}
	echo
	echo "end $1"
	printf '=%.0s' {1..40}
	echo
}



function TPCC_wrapper {
	shards=$1    # number of shards
	cpu=$2	     # cpu used per server node
	replica=$3   # number of replica per shard
	alg=$4       #CC Algorithm alias used by run_all.py, see run_TPCC for choices 
	alg_name=$5  #CC Algorithm name, see run_TPCC for chioces
	echo "|shards:${shards}|cpus:${cpu}|replica:${replica}|Alg:${alg_name}"
	date
	concurrent=100   # amplification factor for client
	exp_name=${prefix}_tpcc_Alg${alg_name}_${shards}shards_${cpu}cpus_${concurrent}cc_${replica}rep
	write_concurrent $concurrent
	# number of -c is the client numbers tested (times the concurrent number currently stored in /tmp/concurrent.yml)
	timeout -s SIGKILL 12m ./run_all.py -g -hh config/hosts.yml -cc config/client_closed.yml -cc /tmp/concurrent.yml -cc config/tpcc.yml -cc config/tapir.yml -b tpcc -m ${alg} -c 1 -c 2 -c 5 -c 10 -c 20 -c 40  -s $shards -u $cpu -r $replica -d $duration $exp_name
	new_experiment $exp_name
	concurrent=1     # amplification factor for client
	exp_name=${prefix}_tpcc_Alg${alg_name}_${shards}shards_${cpu}cpus_${concurrent}cc_${replica}rep
	write_concurrent $concurrent
	timeout -s SIGKILL 10m ./run_all.py -g -hh config/hosts.yml -cc config/client_closed.yml -cc /tmp/concurrent.yml -cc config/tpcc.yml -cc config/tapir.yml -b tpcc -m ${alg}   -c 1 -c 10 -c 20 -c 40 -c 80 -s $shards -u $cpu -r $replica -d $duration $exp_name
	new_experiment $exp_name
}


function run_TPCC {
	shards=$1
	cpu=$2
	replica=$3
	TPCC_wrapper $shards $cpu $replica brq:brq Janus
	TPCC_wrapper $shards $cpu $replica 2pl_ww:multi_paxos 2PL
	TPCC_wrapper $shards $cpu $replica occ:multi_paxos OCC
	TPCC_wrapper $shards $cpu $replica tapir:tapir TAPIR
}




function run_tests {

	for Ncpu in 1 2 3 4
	do
		for rep in 1 3 5 7
		do
			# number of warehouse = numer of shards
			# number of warehouse * replication per warehouse / cpu per node = number of server nodes needed
			for ((WH=1; WH*rep/Ncpu<=27; WH=WH*2))
			do
				date
				echo "CPU:${Ncpu}|rep:${rep}|WH:${WH}"
				run_TPCC $WH $Ncpu $rep
			done
		done
	done

}



run_tests 2>&1 | tee -a ~/${prefix}_tpcc_batch_run.log