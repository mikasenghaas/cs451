#!/bin/bash

if [ -z $1 ] || [ -z $2 ] || [ -z $3 ] || [ -z $4 ]; then
    echo "Usage: $0 <command> <message_count> <num_processes> <timeout>"
    exit 1
fi

# Validate numeric arguments
if ! [[ "$2" =~ ^[0-9]+$ ]] || ! [[ "$3" =~ ^[0-9]+$ ]] || ! [[ "$4" =~ ^[0-9]+$ ]]; then
    echo "Error: message_count, num_processes, and timeout must be positive integers"
    exit 1
fi

command=$1
m=$2
p=$3
t=$4

# Run correctness.sh
bash tools/correctness.sh $command $m $p $t 0

# Estimate performance
echo -e "\nPerformance:"
python tools/performance.py $command \
    -C logs/config \
    -H logs/hosts \
    -L logs