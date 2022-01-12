#!/bin/bash
make clean
VALUE_SIZE=$1 PUT_PERCENT=$2 NUM_KEYS=$3 NUM_KEYS_=`expr ${NUM_KEYS} - 1` ZIPF=$4 make
