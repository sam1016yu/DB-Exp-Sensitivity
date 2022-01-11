# [Aria: A Fast and Practical Deterministic OLTP Database](https://dl.acm.org/doi/10.14778/3407790.3407808)

**Yi Lu**, Xiangyao Yu, Lei Cao, Samuel Madden
*Proc. of the VLDB Endowment (PVLDB), Volume 13, Tokyo, Japan, 2020.*
[Aria original repo](https://github.com/luyi0619/aria)

## Dependencies

```sh
sudo apt-get update
sudo apt-get install -y zip make cmake g++ libjemalloc-dev libboost-dev libgoogle-glog-dev
```

## Build

```
./compile.sh
```

## Example test script for local tpcc test
see [test_tpcc_local](test_tpcc_local.sh)