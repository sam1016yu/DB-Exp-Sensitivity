#!/bin/bash

trap '{
for i in `seq 1 20`; do 
ssh node${i} "sudo killall -9 server; sudo killall -9 timeserver"; 
done 

echo "Killed all server and timeserver"
exit 1
}' INT


# set -x


num_client_thd=$1
nshard=$2     # number of shards
num_replica=$3
log_name=$4




# Paths to source code and logfiles.
srcdir="$HOME/tapir"
logdir="$HOME/tapir"
client="benchClient"    # Which client (benchClient, retwisClient, etc)
store="tapirstore"      # Which store (strongstore, weakstore, tapirstore)
mode="txn-l"            # Mode for storage system.



prev_server_port=`cat shard0.config | grep -o ':[0-9]*' | grep -o '[0-9]*' | head -1`
server_port=$(( $prev_server_port + 1 ))



for ((s = 0; s < $nshard ;s++)); do
echo "f 1" > shard${s}.config
for (( r = 1; r <= $num_replica; r++ )); do
node=$(($s * $num_replica + $r))
echo "replica node${node}:$server_port" >> shard${s}.config
done
done

echo "f 1" > shard.tss.config
for (( i = $nshard*$num_replica+1 ; i <= 2*$nshard*$num_replica ; i++ )); do
	echo "replica node${i}:$server_port" >> shard.tss.config
done


for (( i = 1; i <= 20; i++ )); do
	for (( j = 0; j < $nshard; j++ )); do
		scp shard${j}.config node${i}:$HOME/tapir/store/tools
	done
	scp shard.tss.config node${i}:$HOME/tapir/store/tools
done

echo "Starting TimeStampServer replicas.."
$srcdir/store/tools/start_replica.sh tss $srcdir/store/tools/shard.tss.config \
  "$srcdir/timeserver/timeserver" $logdir

for ((i=0; i<$nshard; i++))
do
	echo "Starting shard$i replicas.."
	$srcdir/store/tools/start_replica.sh shard$i $srcdir/store/tools/shard$i.config \
	"$srcdir/store/$store/server -m $mode" $logdir
done

# Wait a bit for all replicas to start up
sleep 2


cd ../../ycsb-t
./run-tapir.sh $num_client_thd $nshard $log_name

echo "YCSB has finished."

for i in `seq 1 20`; do 
	ssh node${i} "sudo killall -9 server; sudo killall -9 timeserver"; 
done 

echo "Killed all server and timeserver"

sleep 5