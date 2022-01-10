#!/bin/bash

MASSTREE=1 MODE=factor-gc DEBUG=1 make -j dbtest
MODE=perf DEBUG=1 make -j dbtest
