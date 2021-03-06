#!/bin/bash

sudo apt-get update
sudo apt-get install -y software-properties-common
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install -y build-essential cmake git g++-5 libjemalloc-dev libnuma-dev libdb6.0++-dev libgoogle-perftools-dev papi-tools libaio-dev
MASSTREE=1 MODE=factor-gc make -j dbtest
MODE=perf make -j dbtest
