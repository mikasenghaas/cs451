#!/bin/bash

bash cpp_template/build.sh

python tools/stress.py fifo \
    -r cpp_template/run.sh \
    -l example/output \
    -p 2 \
    -m 10 \
    -t 10 \
    -a 0
