import os
import sys
import argparse
from typing import List

def check_sender(outputs: List[str], id: int, n: int):
    all_outputs = set(outputs)
    for msg_id in range(1, n+1):
        msg = f"b {msg_id}"
        assert msg in all_outputs, f"Validation failed for sender with id={id}: {msg} not sent"

def check_receiver(outputs: List[str], num_hosts: int, id: int, n: int):
    senders = [i for i in range(1, num_hosts+1) if i != id]
    all_outputs = set(outputs)
    assert len(all_outputs) == len(outputs), "Duplicate messages received"
    received = {sender_id: 0 for sender_id in senders}
    for sender_id in senders:
        for msg_id in range(1, n+1):
            msg = f"d {sender_id} {msg_id}"
            if msg in all_outputs:
                received[sender_id] += 1
    return received

def check_beb(outputs: List[str], num_hosts: int, message_count: int, host_id: int):
    outputs_set = set(outputs)
    messages = list(range(1, message_count+1))
    broadcast_messages = [f"b {m}" for m in messages]
    deliver_messages = [f"d {h} {m}" for h in range(1, num_hosts+1) for m in messages]
    
    # Check all broadcasts were sent
    for bm in broadcast_messages:
        assert bm in outputs_set, f"[Host {host_id}] Broadcast message {bm} was not sent"

    # Check all delivers were received
    for dm in deliver_messages:
        assert dm in outputs_set, f"[Host {host_id}] Message {dm} was not delivered"

    assert len(deliver_messages) == len(set(deliver_messages)), f"[Host {host_id}] Duplicate messages delivered"

    return True

def check_fifo(outputs: List[str], num_hosts: int, host_id: int):
    # Check broadcast order
    broadcasted = [msg for msg in outputs if msg.startswith("b")]
    last_seq = 0
    for msg in broadcasted:
        seq = int(msg.split()[-1])
        assert seq == last_seq + 1, f"[{host_id}] Broadcast message {seq} before {last_seq+1}"
        last_seq = seq

    # Check delivery order per host
    hosts = list(range(1, num_hosts+1))
    for host in hosts:
        delivered = [msg for msg in outputs if int(msg.split()[1]) == host and msg.startswith("d")]
        
        last_seq = 0
        for msg in delivered:
            seq = int(msg.split()[-1])
            assert seq == last_seq + 1, f"[{host_id}] Got message {seq} before {last_seq+1} from host {host}"
            last_seq = seq

    return True

def main(args):
    config_file = args.config
    host_file = args.host_file
    log_dir = args.log_dir

    assert os.path.exists(config_file), f"Config file {config_file} does not exist"
    assert os.path.exists(host_file), f"Host file {host_file} does not exist"
    assert os.path.exists(log_dir), f"Output directory {log_dir} does not exist"

    if args.command == "perfect":
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

        assert all(os.path.exists(os.path.join(log_dir, f"proc{i:02d}.output")) for i in range(1, num_hosts + 1)), msg

        # Open output files for each process
        for process_id in range(1, num_hosts + 1):
            with open(os.path.join(log_dir, f"proc{process_id:02d}.output"), "r") as f:
                outputs = list(map(str.strip, f.readlines()))
                if process_id == receiver_id:
                    num_received = check_receiver(outputs, num_hosts, process_id, message_count)
                else:
                    check_sender(outputs, process_id, message_count)

        for s, m in num_received.items():
            print(f"Received {m} ({m/message_count*100:.2f}%) messages from {s}")

        print("Perfect link validation passed ✅")
    elif args.command == "fifo":
        # Open config file
        with open(config_file, "r") as f:
            config = f.read()

        # Parse config
        message_count = int(config.split()[0])

        # Host file
        with open(host_file, "r") as f:
            hosts = f.readlines()

        num_hosts = len(hosts)

        msg = f"Output files for all processes must exist"

        assert all(os.path.exists(os.path.join(log_dir, f"proc{i:02d}.output")) for i in range(1, num_hosts + 1)), msg

        # Check BEB validation
        for host_id in range(1, num_hosts + 1):
            with open(os.path.join(log_dir, f"proc{host_id:02d}.output"), "r") as f:
                outputs = list(map(str.strip, f.readlines()))
                check_beb(outputs, num_hosts, message_count, host_id)
                check_fifo(outputs, num_hosts, host_id)
        
        print("BEB validation passed ✅")
        print("FIFO validation passed ✅")
    else:
        raise ValueError(f"Invalid command: {args.command}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    sub_parsers = parser.add_subparsers(dest="command", help="validate a given milestone")
    sub_parsers.required = True
    parser_perfect = sub_parsers.add_parser("perfect", help="validate perfect links")
    parser_fifo = sub_parsers.add_parser("fifo", help="validate fifo broadcast")
    parser_agreement = sub_parsers.add_parser("agreement", help="validate lattice agreement")

    for subparser in [parser_perfect, parser_fifo, parser_agreement]:
        subparser.add_argument(
            "-C",
            "--config",
            required=True,
            dest="config",
            help="Path to the config file",
        )

        subparser.add_argument(
            "-H",
            "--host-file",
            required=True,
            dest="host_file",
            help="Path to the host file",
        )

        subparser.add_argument(
            "-L",
            "--log-dir",
            required=True,
            dest="log_dir",
            help="Directory containing stdout, stderr and outputs generated by the processes",
        )

    main(parser.parse_args())
