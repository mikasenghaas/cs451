#!/bin/bash

if [ -z $1 ] || [ -z $2 ]; then
    echo "Usage: $0 <message_count> <num_processes> <timeout>"
    exit 1
fi
m=$1
p=$2
t=$3

# Run correctness.sh
bash tools/test.sh $m $p $t 0

# Estimate performance
echo -e "\nPerformance:"
python tools/performance.py perfect \
    -C logs/config \
    -H logs/hosts \
    -L logs