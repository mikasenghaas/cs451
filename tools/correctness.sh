#!/bin/bash

# Validate input arguments
if [ -z "$1" ] || [ -z "$2" ] || [ -z "$3" ] || [ -z "$4" ]; then
    echo "Usage: $0 <command> <message_count> <num_processes> <timeout> [attempts]"
    exit 1
fi

# Validate numeric arguments
if ! [[ "$2" =~ ^[0-9]+$ ]] || ! [[ "$3" =~ ^[0-9]+$ ]] || ! [[ "$4" =~ ^[0-9]+$ ]]; then
    echo "Error: message_count, num_processes, and timeout must be positive integers"
    exit 1
fi

# Validate attempts if provided
if [ ! -z "$5" ] && ! [[ "$5" =~ ^[0-9]+$ ]]; then
    echo "Error: attempts must be a positive integer"
    exit 1
fi

command=$1
m=$2
p=$3
t=$4
a=${5:-0}  # Default attempts to 0 if not provided

# Build project
bash cpp_template/build.sh

# Generate fresh logs directory
rm -rf logs 2>/dev/null
mkdir logs 2>/dev/null

# Run stress test and get pid
python tools/stress.py $command \
    -r cpp_template/run.sh \
    -l logs \
    -p $p \
    -m $m \
    -t $t \
    -a $a \

# Validate correctness
echo -e "\nCorrectness:"
python tools/correctness.py $command \
    -C logs/config \
    -H logs/hosts \
    -L logs