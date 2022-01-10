#!/bin/bash


while read p; do
    re="\(([^)]+)\)"
    if [[ $p =~ $re ]]; then  addr2line -a ${BASH_REMATCH[1]} -e ./build/deptran_server | tee  ; fi
    # echo "$p"
# done < ~/archive/log/proc-host10.err 
done < ./test.err