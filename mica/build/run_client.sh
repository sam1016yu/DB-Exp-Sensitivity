#!/bin/bash

ips=("128.105.145.217" "128.105.145.220")
ptime=15
ttime=75

for dataset in 0 1 2
do
for skew in "uniform" "skewed"
do
for get in 0.50 0.95
do

testfile=$(ls conf_workload_${dataset}_${skew}_${get}_*_0.00_1)
sudo ./netbench_server conf_machines_${dataset}_EREW_0.5 server 0 0 conf_prepopulation_empty > log_server_${testfile} 2>&1 &
sleep ${ptime}

sudo ssh ${ips[0]} "cd /mnt/mica-cloudlab/build;./netbench_client conf_machines_${dataset}_EREW_0.5 client0 0 0 ${testfile} & sleep ${ttime}; pkill netbench" > ./log_client0_${testfile} 2>&1 &
sudo ssh ${ips[1]} "cd /mnt/mica-cloudlab/build;./netbench_client conf_machines_${dataset}_EREW_0.5 client1 0 0 ${testfile} & sleep ${ttime}; pkill netbench" > ./log_client1_${testfile} 2>&1 &

sleep $ttime
sudo pkill netbench

sleep 5

done
done
done
