* These are TPC-C transactions that are transformd from Benchbase transactions to call the stored procedures in H2 server.

* When executing TPC-C stored procedures, replace original transaction files in Benchbase with these SP-call files in the folder below: 

```bash
benchbase/src/main/java/com/oltpbenchmark/benchmarks/tpcc/procedures/
```
