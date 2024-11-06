#!/bin/bash

bash cpp_template/build.sh

python tools/stress.py perfect \
    -r cpp_template/run.sh \
    -l example/output \
    -p 3 \
    -m 10 \
    -t 10 \
    -a 0
