#!/bin/bash

bash cpp_template/build.sh

python tools/stress.py agreement \
    -r cpp_template/run.sh \
    -l example/output \
    -p 3 \
    -n 1 \
    -v 5 \
    -d 5 \
    -a 0
