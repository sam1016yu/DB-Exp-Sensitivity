import sys

record_count=sys.argv[1]  # unit is M
read_ratio=sys.argv[2]
distribution=sys.argv[3]  # uniform or zipfian


with open("/users/miaoyu/tapir/ycsb-t/workloads/custom",'w') as f:
    f.write("recordcount={}\n".format(int(float(record_count)*1e6)))
    f.write("operationcount=1000000\n")
    f.write("workload=com.yahoo.ycsb.workloads.CoreWorkload\n")
    f.write("readallfields=true\n")
    f.write("readproportion={}\n".format(read_ratio))
    f.write("updateproportion={}\n".format(1-float(read_ratio)))
    f.write("scanproportion=0\n")
    f.write("insertproportion=0\n")
    f.write("requestdistribution={}\n".format(distribution))
    f.write("dotransactions=true\n")

