#!/bin/bash

# Validate input arguments
if [ -z "$1" ]; then
    echo "Usage: $0 {perfect|fifo|agreement} ..."
    exit 1
fi

command=$1
if [ "$command" == "perfect" ] || [ "$command" == "fifo" ]; then
    if [ -z "$2" ] || [ -z "$3" ] || [ -z "$4" ]; then
        echo "Usage: $0 {perfect|fifo} <message_count> <num_processes> <timeout> [attempts]"
        exit 1
    fi
    m=$2
    p=$3
    t=$4
    a=${5:-0}  # Default attempts to 0 if not provided
elif [ "$command" == "agreement" ]; then
    if [ -z "$2" ] || [ -z "$3" ] || [ -z "$4" ] || [ -z "$5" ]; then
        echo "Usage: $0 agreement <num_processes> <num_proposals> <max_proposal_size> <max_distinct_values> <timeout> [attempts]"
        exit 1
    fi
    p=$2
    n=$3
    vs=$4
    ds=$5
    t=$6
    a=${7:-0}  # Default attempts to 0 if not provided
else
    echo "Error: command must be in { perfect, fifo, agreement }"
    exit 1
fi

# Link correct source code
rm cpp_template/src/src/main.cpp 2>/dev/null
if [ "$command" == "perfect" ]; then
    ln cpp_template/src/src/m1.cpp cpp_template/src/src/main.cpp
elif [ "$command" == "fifo" ]; then
    ln cpp_template/src/src/m2.cpp cpp_template/src/src/main.cpp
elif [ "$command" == "agreement" ]; then
    ln cpp_template/src/src/m3.cpp cpp_template/src/src/main.cpp
fi

# Build project
bash cpp_template/build.sh

# Generate fresh logs directory
rm -rf logs 2>/dev/null
mkdir logs 2>/dev/null

# Run stress test and get pid
if [ "$command" == "perfect" ] || [ "$command" == "fifo" ]; then
    python tools/stress.py $command \
        -r cpp_template/run.sh \
        -l logs \
        -p $p \
        -m $m \
        -t $t \
        -a $a
elif [ "$command" == "agreement" ]; then
    python tools/stress.py $command \
        -r cpp_template/run.sh \
        -l logs \
        -p $p \
        -n $n \
        -v $vs \
        -d $ds \
        -t $t \
        -a $a
fi

# Validate correctness
echo -e "\nCorrectness:"
python tools/correctness.py $command \
    -C logs/config \
    -H logs/hosts \
    -L logs

# Estimate performance
echo -e "\nPerformance:"
python tools/performance.py $command \
    -C logs/config \
    -H logs/hosts \
    -L logs