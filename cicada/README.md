# Cicada(SIGMOD 2017)


- [Paper link](https://dl.acm.org/doi/10.1145/3035918.3064015)
- [Original Repo](https://github.com/efficient/cicada-exp-sigmod2017)

Hardware requirements
---------------------

 * Dual-socket Intel CPU >= Haswell
   * Interleaved CPU core ID mapping (even numbered cores on CPU 0, odd numbered cores on CPU 1)
   * Turbo Boost disabled for more accurate core scalability measurement
   * Hyperthreading enabled (though experiments do not use it directly)
 * DRAM >= 128 GiB
   * Ensure to use all memory channels (while keeping the maximum frequency) for full bandwidth
 * Disk space >= 15 GB
   * SSD recommended

Base OS
-------

 * Ubuntu 14.04 LTS amd64 server

Installing packages + Configure systems + Build all systems
-------------------
see [set_env](set_env.sh)

Running all experiments
-----------------------

	EXPNAME=MYEXP
	./run_exp.py exp_data_$EXPNAME run
