# Janus: Consolidating Concurrency Control and Consensus for Commits under Conflicts

- [paper link](http://mpaxos.com/pub/janus-osdi16.pdf)
- [original repo](https://github.com/NYU-NEWS/janus)

## how to set up environment and build

Install dependencies:

```
sudo apt-get update
sudo apt-get install -y \
    git \
    pkg-config \
    build-essential \
    clang \
    libapr1-dev libaprutil1-dev \
    libboost-all-dev \
    libyaml-cpp-dev \
    python-dev \
    python-pip \
    libgoogle-perftools-dev
sudo pip install -r requirements.txt
```

Build:

```
./waf configure build -t
```


## How to inject latency with netem
```
tc qdisc add dev eth0 root netem delay 100ms
```
see [here](https://wiki.linuxfoundation.org/networking/netem) for more *netem* exampls

## How to run Janus and competitors

1. Configure cluster ip in [hosts.yml](config/hosts.yml)
2. see wrapper script example in [run_single](run_single.sh) and [run_batch](run_batch.sh)