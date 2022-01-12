#! /bin/bash


bin_dir=$1 #Specify the path of binary

launch_herd_clients() {
	for ((j=1; j<=12; j++)); do
		id=`expr $j - 1`
		ssh $USER@node$j "cd ${bin_dir} && ./run-machine.sh ${id}" &
		sleep .5
	done
}

launch_herd_servers() {
	cd ${bin_dir}
	./run-servers.sh > /dev/null 2>&1
	sleep 1
}

stop_herd () {
	cd ${bin_dir}
	./local-kill.sh
	for ((j=1; j<=12; j++)); do
		ssh $USER@node$j "cd /proj/osu-nfs-test-PG0/yujie/HERD && ./local-kill.sh"
	done
}


compile () {
	echo "recompile HERD"
	value_size=$1
	put_percent=$2
	num_key=$3
	theta=$4
	is_zipf=$5
	if [[ $is_zipf -eq 0 ]]; then
		zipf=0
	else
		zipf=$theta	
	fi
	cd ${bin_dir}
	eval "./do.sh $1 $2 $3 $zipf"
	echo "" >> output.txt
	echo "value_size:$1 put_percent:$2 num_keys:$3 zipf:$4"
	echo "value_size:$1 put_percent:$2 num_keys:$3 zipf:$4" >> output.txt
}

start_experiments () {
	stop_herd
	index=0
	value_sizes=(32 128)
	num_keys=(128 1024 2048 4096 8192)
	put_percents=(5 50)
	zipfs=(0 0.99)
	is_zipf=0
	for ((z=0; z<${#zipfs[@]}; z++)); do
		if [[ $z -ne 0 ]]; then
			is_zipf=1
		fi
		for n in ${num_keys[@]}; do
			mv /proj/osu-nfs-test-PG0/yujie/herd-zipf-$n-${zipfs[z]} /proj/osu-nfs-test-PG0/yujie/herd-zipf
			for v in ${value_sizes[@]}; do
				for p in ${put_percents[@]}; do
					compile $v $p `expr $n \* 1024` ${zipfs[z]} $is_zipf
					launch_herd_servers
					launch_herd_clients
					sleep 60
					stop_herd
				done	
			done
			mv /proj/osu-nfs-test-PG0/yujie/herd-zipf /proj/osu-nfs-test-PG0/yujie/herd-zipf-$n-${zipfs[z]} 
		done			
	done
}

#generate zipf data if needed (should generate the zipf data into a shared folder)
launch_generator() {
        zipf=$1
        keys=$2
        DIR=${bin_dir}/herd-zipf-$keys-$zipf
        mkdir $DIR
        cd ${bin_dir}/YCSB/src && java zipf $keys $zipf $DIR &
}


generate() {
        num_keys=(128 1024 2048 4096 8192)
        zipfs=(0.99)
        for z in ${zipfs[@]}; do
                for n in ${num_keys[@]}; do
                        launch $z $n
                        echo "launch $z $n"
                done
        done
}

generate
start_experiments
