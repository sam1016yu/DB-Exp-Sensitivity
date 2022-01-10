#!/bin/bash

# for g++-5
sudo apt-get update
sudo apt-get install -y software-properties-common
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test

# common
sudo apt-get update
sudo apt-get install -y build-essential cmake git g++-5 libjemalloc-dev libnuma-dev libdb6.0++-dev libgoogle-perftools-dev papi-tools psmisc python3 python3-pip

cd ..
git clone https://github.com/google/cityhash.git
cd cityhash

./configure
make all check CXXFLAGS="-g -O3"
sudo make install

cd ../cicada-exp-sigmod2017

./build_cicada.sh
./build_ermia.sh
./build_foedus.sh
./build_silo.sh





# for non-interactive experiment execution
echo "`whoami` ALL=(ALL:ALL) NOPASSWD:ALL" | sudo tee -a /etc/sudoers

# for third-party engines
echo "`whoami` - memlock unlimited" | sudo tee -a /etc/security/limits.conf
echo "`whoami` - nofile 655360" | sudo tee -a /etc/security/limits.conf
echo "`whoami` - nproc 655360" | sudo tee -a /etc/security/limits.conf
echo "`whoami` - rtprio 99" | sudo tee -a /etc/security/limits.conf

sudo groupadd hugeshm
sudo usermod -a -G hugeshm `whoami`

sudo reboot


