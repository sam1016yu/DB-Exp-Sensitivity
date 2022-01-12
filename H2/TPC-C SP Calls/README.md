These are TPC-C transactions that are transformd from OLTPBench transactions to call the stored procedures in H2 server.

When executing the TPC-C stored procedures, replace original transaction files in Benchbase with these SP-calls files in folder: 

benchbase/src/main/java/com/oltpbenchmark/benchmarks/tpcc/procedures/
