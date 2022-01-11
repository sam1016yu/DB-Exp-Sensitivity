#! /bin/bash
launch () {
  dist_ratio=$1
  start=$2
  end=$3
  script="./tpcc ${USER_ARGS} -d${dist_ratio}"
  ./tpcc ${USER_ARGS} -d${dist_ratio} > /dev/null 2>&1 &

  for ((x=`expr $start + 1`;x<=$end;x++)); do
    host=$USER@node${x}
    ssh ${ssh_opts} ${host} "./tpcc ${USER_ARGS} -d${dist_ratio} > /dev/null 2>&1" &
  done
  wait
  echo "done for ${dist_ratio}"
  sleep 50
}


run_tpcc () {
  start=$1 #0-based index of node to run, e.g., node0, node1...
  end=$2
  num_servers=`expr $end - $start + 1`
  num_whs_per_node=(4 8 16)
  num_threads=(4)
  num_whs=(0 0 0)
  for ((i=0;i<${#num_whs[@]}; i++)); do
    num_whs[$i]=`expr ${num_whs_per_node[i]} \* $num_servers`
    echo ${num_whs[i]}
  done
  for ((i=0; i<${#num_whs[@]}; i++)); do
    for ((j=0; j<${#num_threads[@]}; j++)); do
      thread=${num_threads[j]}
	echo "" >> output.txt
      USER_ARGS="-p11111 -sf${num_whs[i]} -sf1 -c${thread} -t200000"
      dist_ratios=(0 10 20 30 40 50 60 70 80 90 100)
      echo "warehouses:${num_whs[i]} threads:${thread} servers:$num_servers scale:1" >> output.txt
      echo "USER_ARGS: ${USER_ARGS}"
      for dist_ratio in ${dist_ratios[@]}; do
        launch ${dist_ratio} $start $end
      done
      echo "DONE: warehouse num: ${num_whs[i]} thread num: ${thread}"
    done
  done
}

run_default () {
  start=$1 #0-based index of node to run, e.g., node0, node1...
  end=$2
  echo  "" >> output.txt
  USER_ARGS="-p11111 -sf32 -sf10 -c4 -t200000"
  dist_ratios=(0 10 20 30 40 50 60 70 80 90 100)
  echo "warehouses:32 threads:4 servers:8 scale:10" >> output.txt
  echo "USER_ARGS: ${USER_ARGS}"
  for dist_ratio in ${dist_ratios[@]}; do
    launch ${dist_ratio} $start $end
  done
}

stop() {
  start=$1
  end=$2
  echo $start
  echo $end
  sudo killall tpcc
  for ((i=`expr $start + 1`; i<=$end; i++)); do
    ssh -p 22 $USER@node${i} 'sudo killall tpcc'
  done
}

stop 0 7 
run_default 0 7
run_tpcc 0 7
