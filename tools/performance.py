import re
import os
import argparse
from datetime import datetime
from typing import List

def main(args):
    log_dir = args.log_dir
    assert os.path.exists(log_dir), f"Log directory {log_dir} does not exist"

    if args.command == "perfect" or args.command == "fifo":
        config_file = os.path.join(log_dir, "config")
        host_file = os.path.join(log_dir, "hosts")
        assert os.path.exists(config_file), f"Config file {config_file} does not exist"
        assert os.path.exists(host_file), f"Host file {host_file} does not exist"
    else:
        host_file = os.path.join(log_dir, "hosts")
        num_processes = int(os.popen(f"wc -l < {host_file}").read().strip())
        config_files = [os.path.join(log_dir, f"proc{i:02d}.config") for i in range(1, num_processes+1)]
        for config_file in config_files:
            assert os.path.exists(config_file), f"Config file {config_file} does not exist"

    if args.command == "perfect":
        config_file = os.path.join(log_dir, "config")
        host_file = os.path.join(log_dir, "hosts")
        assert os.path.exists(config_file), f"Config file {config_file} does not exist"
        assert os.path.exists(host_file), f"Host file {host_file} does not exist"


    if args.command == "perfect":
        # Get receiver id
        with open(config_file, "r") as f:
            receiver_id = int(f.read().split()[1])

        # Open output file for receiver (for delivery)
        receiver_output = os.path.join(log_dir, f"proc{receiver_id:02d}.output")
        receiver_stdout = os.path.join(log_dir, f"proc{receiver_id:02d}.stdout")
        
        # Get end time from last modification of receiver output file
        stats = os.stat(receiver_output)
        end_time = datetime.fromtimestamp(stats.st_mtime)

        # Get start time from receiver stdout
        with open(receiver_stdout, "r") as f:
            content = f.read()
            # Find starting with "Timestamp:" using regex
            match = re.search(r"Timestamp: (\d+)", content)
            start_time = datetime.fromtimestamp(int(match.group(1)) / 1000)

        # Get time difference (in seconds)
        time_diff = (end_time - start_time).total_seconds()

        # Get number of delivered messages
        with open(receiver_output, "r") as f:
            num_delivered_messages = len(f.readlines())

        # Print results
        print(f"Start time: {start_time}")
        print(f"End time: {end_time}")
        print(f"Total Messages: {num_delivered_messages}")
        print(f"Total Time: {time_diff:.2f}s")
        print(f"Average Throughput: {num_delivered_messages/time_diff:.2f} messages/s")
    elif args.command == "fifo":
        # Open hosts file
        throughputs = []

        # Host file
        with open(host_file, "r") as f:
            num_hosts = len(f.readlines())

        # Open output files for each process
        for process_id in range(1, num_hosts + 1):
            process_output = os.path.join(log_dir, f"proc{process_id:02d}.output")
            process_stdout = os.path.join(log_dir, f"proc{process_id:02d}.stdout")

            # Get start time from process stdout
            with open(process_stdout, "r") as f:
                content = f.read()
                # Find starting with "Timestamp:" using regex
                match = re.search(r"Timestamp: (\d+)", content)
                start_time = datetime.fromtimestamp(int(match.group(1)) / 1000)

            # Get end time from last modification of process output file
            stats = os.stat(process_output)
            end_time = datetime.fromtimestamp(stats.st_mtime)

            # Get number of delivered messages
            with open(process_output, "r") as f:
                outputs = list(map(str.strip, f.readlines()))
                delivery_messages = [o for o in outputs if o.startswith("d")]
                total_messages = len(delivery_messages)

            total_time = (end_time - start_time).total_seconds()
            throughput = total_messages / total_time
            throughputs.append(throughput)
            print(f"Process {process_id} delivered {total_messages} messages in {total_time:.2f}s ({throughput:.2f} messages/s)")

        print(f"Average Throughput: {sum(throughputs)/len(throughputs):.2f} messages/s")
    elif args.command == "agreement":
        print("Lattice agreement validation not yet implemented 🚧")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    sub_parsers = parser.add_subparsers(dest="command", help="validate a given milestone")
    sub_parsers.required = True
    parser_perfect = sub_parsers.add_parser("perfect", help="validate perfect links")
    parser_fifo = sub_parsers.add_parser("fifo", help="validate fifo broadcast")
    parser_agreement = sub_parsers.add_parser("agreement", help="validate lattice agreement")

    for subparser in [parser_perfect, parser_fifo, parser_agreement]:
        subparser.add_argument(
            "--log-dir",
            required=True,
            dest="log_dir",
            help="Directory containing stdout, stderr and outputs generated by the processes",
        )

    main(parser.parse_args())
