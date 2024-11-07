#!/bin/bash

if [ -z $1 ] || [ -z $2 ] || [ -z $3 ]; then
    echo "Usage: $0 <message_count> <num_processes> <timeout> [attempts]"
    exit 1
fi
m=$1
p=$2
t=$3
a=${4:-0}  # Default attempts to 0 if not provided

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
    -m $m \
    -t $t \
    -a $a \

# Validate correctness
echo -e "\nCorrectness:"
python tools/correctness.py perfect \
    -C logs/config \
    -H logs/hosts \
    -L logs