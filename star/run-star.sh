#! /bin/bash
bin_dir=$3 #specify the directory of binary

# if num_threads=(1 4 16)
# minimum parition: 2_servers=(2 8 32) 4_servers(4 16 64) 8_servers=(8 32 128)
# 2 servers: 1 thread (16 32 128 256) 4 thread (16 32 128 256) 16 thread (32 128 256)
# 4 servers: 1 thread (16 32 128 256) 4 thread (16 32 128 256) 16 thread (64 128 256)
# 8 servers: 1 thread (16 32 128 256) 4 thread (32 128 256) 16 thread (128 256)

launch_tpcc () {
	dist_ratio=$1
 	partition_num=$2
 	thread_num=$3
	start=$4
	end=$5
	id=1
  	script="cd ${bin_dir} && ./bench_tpcc ${USER_ARGS} --partition_num=$2 --threads=$3 --neworder_dist=$1 --payment_dist=$1" 

 	echo "start master:"$script" --id=0 > /dev/null 2>&1 &"
  	eval "$script --id=0 > /dev/null 2>&1 &" 
  	sleep 3
  	for ((x=`expr $start + 1`;x<=${end};x++)); do #exclude this node then start from $start+1
    		host=$USER@node${x}
    		echo "start worker: ssh ${host} "$script" --id=$id  > /dev/null 2>&1 &"
    		ssh ${host} "cd ${bin_dir} && ./bench_tpcc ${USER_ARGS} --partition_num=$2 --threads=$3 --neworder_dist=$1 --payment_dist=$1 --id=$id > /dev/null 2>&1 " &
    		sleep 3
		id=`expr $id + 1`
  	done
  	wait
  	echo "done for ${dist_ratio}" 
  	sleep 60 #wait enough time to make sure the port is released

}

run_tpcc () {
	start=$1 #0-based index of machine number, e,g, node0's ip is 10.10.1.1, include this node.
	end=$2
	num_servers=`expr $end - $start + 1`
	echo "$num_servers servers"
	ips=""
	for ((i=`expr $start + 1`;i<=`expr $end + 1`;++i)); do
		ips="${ips}10.10.1.$i:11111;"
	done
	ips="${ips%?}"	
	num_partitions=(48 96 192 240)
	num_threads=(12)
	USER_ARGS="--logtostderr=1 --servers='$ips' --protocol=Star --partitioner=hash2 --query=mixed"
	for ((i=0; i<${#num_partitions[@]}; i++)); do
    		for ((j=0; j<${#num_threads[@]}; j++)); do
      			thread=${num_threads[j]}
			minimum_partition=`expr $num_servers \* $thread`
			if [ ${num_partitions[i]} -lt ${minimum_partition} ]; then
				continue
		        fi	
			echo "" >> output.txt
      			echo "partition_num:${num_partitions[i]} threads:$thread servers:$num_servers" >> output.txt
			echo "partition_num:${num_partitions[i]} threads:$thread"
      			dist_ratios=(10 20 30 40 50 60 70 80 90 100)
      			for dist_ratio in ${dist_ratios[@]}; do
        			launch_tpcc ${dist_ratio} ${num_partitions[i]} ${thread} ${start} ${end}
      			done
      		echo "DONE: partition num: ${num_partitions[i]} thread num: ${thread}"  
    		done
  	done
}


stop() {
  start=$1
  end=$2
  echo $start
  echo $end
  sudo killall bench_tpcc
  for ((i=`expr $start + 1`; i<=$end; i++)); do
    ssh -p 22 $USER@node${i} 'sudo killall bench_tpcc'
  done
}


START=$1
END=$2
stop $START $END
run_tpcc $START $END 
