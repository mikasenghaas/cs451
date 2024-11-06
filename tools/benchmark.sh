#!/bin/bash

if [ -z $1 ]; then
    echo "Usage: $0 <num_processes>"
    exit 1
fi
p=$1

# Build project
bash cpp_template/build.sh

# Generate fresh logs directory
rm -rf logs 2>/dev/null
mkdir logs 2>/dev/null

# Run stress test and get pid
python tools/stress.py perfect \
    -r cpp_template/run.sh \
    -l logs \
    -p $p \
    -m 100000 \
    -t 30 \
    -a 0 \

# Validate correctness
echo -e "\nCorrectness:"
python tools/correctness.py perfect \
    -C logs/config \
    -H logs/hosts \
    -L logs

# Estimate performance
echo -e "\nPerformance:"
python tools/performance.py perfect \
    -C logs/config \
    -H logs/hosts \
    -L logs