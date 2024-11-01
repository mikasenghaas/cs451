#!/bin/bash

# Build the application
log_file="example/output/example.log"
bash build.sh >> $log_file

# Remove previous outputs
rm -f example/output/*.output 2>/dev/null
rm -f example/output/*.log 2>/dev/null
rm -f example/output/log 2>/dev/null

# Read config
num_processes=$(cat example/configs/perfect-links.config | cut -d ' ' -f2)
echo "Number of processes: $num_processes" >> $log_file 

# Store PIDs for cleanup
pids=()

# Start processes
for i in $(seq $num_processes -1 1); do
    bin/da_proc --id $i --hosts example/hosts --output example/output/$i.output example/configs/perfect-links.config > example/output/$i.log &
    pids+=($!)
done
echo "PIDs: ${pids[@]}" >> $log_file

# Wait for specified duration
echo "Waiting for timeout" >> $log_file
sleep 1

echo "Sending SIGINT to all processes" >> $log_file
# Send SIGINT to all processes
for pid in "${pids[@]}"; do
    kill -SIGINT $pid 2>/dev/null
done

echo "Done" >> $log_file

# Validate the output files
python tools/validate/perfect_link.py example/configs/perfect-links.config example/output
