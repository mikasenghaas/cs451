#!/bin/bash

# Change the current working directory to the location of the present file
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )" 

# Build the application
log_file="../example/output/example.stdout"
bash $DIR/build.sh >> $log_file

# Remove previous outputs
rm -f ../example/output/*.output 2>/dev/null
rm -f ../example/output/*.stdout 2>/dev/null

# Read hosts
num_processes=$(cat ../example/hosts | wc -l)
echo "Number of processes: $num_processes" >> $log_file 

# Read config
receiver_id=$(cat ../example/configs/perfect-links.config | cut -d ' ' -f2)
echo "Receiver ID: $receiver_id" >> $log_file 

# Store PIDs for cleanup
pids=()

# Start processes
for i in $(seq $num_processes -1 1); do
    $DIR/bin/da_proc --id $i --hosts ../example/hosts --output ../example/output/proc$(printf "%02d" $i).output ../example/configs/perfect-links.config > ../example/output/proc$(printf "%02d" $i).stdout &
    pids+=($!)
done
echo "PIDs: ${pids[@]}" >> $log_file

# Wait for until ENTER is pressed
echo "Press Enter when all processes have finished processing messages."
read -n 1 -s

echo "Sending SIGINT to all processes" >> $log_file
# Send SIGINT to all processes
for pid in "${pids[@]}"; do
    kill -SIGINT $pid 2>/dev/null
done

echo "Done" >> $log_file

# Validate the output files
python ../tools/validate/perfect_link.py ../example/configs/perfect-links.config ../example/hosts ../example/output
