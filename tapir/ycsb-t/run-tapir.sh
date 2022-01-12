#! /usr/bin/env bash
num_client=$1
num_shards=$2
logname=$3


# Copy the shared library to libs folder.
# mkdir -p libs
# cp ../libtapir/libtapir.so ./libs/
# 
# Make the tapir binding using maven
# mvn clean package

# Load the records in Tapir
java -cp tapir-interface/target/tapir-interface-0.1.4.jar:core/target/core-0.1.4.jar:tapir/target/tapir-binding-0.1.4.jar:javacpp/target/javacpp.jar \
-Djava.library.path=libs/ com.yahoo.ycsb.Client -P workloads/custom \
-load -threads 20 -db com.yahoo.ycsb.db.TapirClient -s \
-p tapir.configpath=../store/tools/shard -p tapir.nshards=$num_shards -p tapir.closestreplica=0 > load.log 2>&1


# Run the YCSB workload
# ssh node4 "sudo perf record -g -a -o perf.data sleep 120" & > /dev/null
java -cp tapir-interface/target/tapir-interface-0.1.4.jar:core/target/core-0.1.4.jar:tapir/target/tapir-binding-0.1.4.jar:javacpp/target/javacpp.jar \
-Djava.library.path=libs/ com.yahoo.ycsb.Client -P workloads/custom \
-t -threads $num_client -db com.yahoo.ycsb.db.TapirClient \
-p tapir.configpath=../store/tools/shard -p tapir.nshards=$num_shards -p tapir.closestreplica=0 > $logname 2>&1