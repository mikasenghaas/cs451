import os
import sys
from typing import List

def check_sender(outputs: List[str], id: int, n: int):
    all_outputs = set(outputs)
    for msg_id in range(1, n+1):
        msg = f"b {msg_id}"
        assert msg in all_outputs, f"Validation failed for sender with id={id}: {msg} not sent"

def check_receiver(outputs: List[str], id: int, n: int):
    senders = [i for i in range(1, id)]
    all_outputs = set(outputs)
    for sender_id in senders:
        for msg_id in range(1, n+1):
            msg = f"d {sender_id} {msg_id}"
            assert msg in all_outputs, f"Validation failed for receiver with id={id}: {msg} not received"

def main():
    if len(sys.argv) != 4:
        print("Usage: python perfect_link.py <config_file> <host_file> <output_dir>")
        sys.exit(1)

    # Read arguments
    config_file = sys.argv[1]
    host_file = sys.argv[2]
    output_dir = sys.argv[3]

    assert os.path.exists(config_file), f"Config file {config_file} does not exist"
    assert os.path.exists(host_file), f"Host file {host_file} does not exist"
    assert os.path.exists(output_dir), f"Output directory {output_dir} does not exist"

    # Open config file
    with open(config_file, "r") as f:
        config = f.read()

    # Parse config
    message_count = int(config.split()[0])
    receiver_id = int(config.split()[1])

    # Host file
    with open(host_file, "r") as f:
        hosts = f.readlines()

    num_hosts = len(hosts)

    msg = f"Output files for all processes must exist"

    assert all(os.path.exists(os.path.join(output_dir, f"proc{i:02d}.output")) for i in range(1, num_hosts + 1)), msg

    # Open output files for each process
    for process_id in range(1, num_hosts + 1):
        with open(os.path.join(output_dir, f"proc{process_id:02d}.output"), "r") as f:
            outputs = list(map(str.strip, f.readlines()))
            if process_id == receiver_id:
                check_receiver(outputs, process_id, message_count)
            else:
                check_sender(outputs, process_id, message_count)

    print("Perfect link validation passed ✅")

if __name__ == "__main__":
    main()
