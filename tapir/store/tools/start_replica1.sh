#!/bin/bash

trap '{
  echo "\nKilling all clients.. Please wait..";
  for host in ${clients[@]}
  do
	ssh $host "killall -9 $client";
	ssh $host "killall -9 $client";
  done

  echo "\nKilling all replics.. Please wait..";
  for host in ${servers[@]}
  do
	ssh $host "killall -9 server";
  done
  exit 1
}' INT

# Paths to source code and logfiles.
srcdir="$HOME/tapir"
logdir="$HOME/tapir"

# Machines on which replicas are running.
replicas=("node3" "node1" "node2")

# Machines on which clients are running.
clients=("node0")

client="benchClient"    # Which client (benchClient, retwisClient, etc)
store="tapirstore"      # Which store (strongstore, weakstore, tapirstore)
mode="txn-l"            # Mode for storage system.

nshard=1     # number of shards
nclient=1    # number of clients to run (per machine)
nkeys=1000000 # number of keys to use
rtime=10     # duration to run

tlen=2       # transaction length
wper=0       # writes percentage
err=0        # error
skew=0       # skew
zalpha=-1    # zipf alpha (-1 to disable zipf and enable uniform)

# Print out configuration being used.
echo "Configuration:"
echo "Shards: $nshard"
echo "Clients per host: $nclient"
echo "Threads per client: $nthread"
echo "Keys: $nkeys"
echo "Transaction Length: $tlen"
echo "Write Percentage: $wper"
echo "Error: $err"
echo "Skew: $skew"
echo "Zipf alpha: $zalpha"
echo "Skew: $skew"
echo "Client: $client"
echo "Store: $store"
echo "Mode: $mode"

prev_server_port=`cat shard0.config | grep -o ':[0-9]*' | grep -o '[0-9]*' | head -1`
server_port=$(( $prev_server_port + 1 ))

num_replica=3

echo "f 1" > shard0.config
for (( i = 4; i <= 2 * $num_replica; i++ )); do
	echo "replica node${i}:$server_port" >> shard0.config
done

echo "f 1" > shard.tss.config
for (( i = 1 + 2 * $num_replica; i <= 3 * $num_replica; i++ )); do
	echo "replica node${i}:$server_port" >> shard.tss.config
done

for (( i = 0; i < 10; i++ )); do
	scp shard0.config node${i}:$HOME/tapir/store/tools
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

num_client=$1

cd ../../ycsb-t
./run-tapir.sh $num_client

echo "YCSB has finished."

for i in `seq 1 9`; do 
	ssh node${i} "sudo killall -9 server; sudo killall -9 timeserver"; 
done 

echo "Killed all server and timeserver"

sleep 60