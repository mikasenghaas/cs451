#!/usr/bin/env python3

import argparse
import os, sys
import time
import threading, subprocess
import itertools


import signal
import random
import time
from enum import Enum

PROCESSES_BASE_IP = 11000


def positive_int(value):
    ivalue = int(value)
    if ivalue <= 0:
        raise argparse.ArgumentTypeError("{} is not positive integer".format(value))
    return ivalue

def non_negative_int(value):
    ivalue = int(value)
    if ivalue < 0:
        raise argparse.ArgumentTypeError("{} is not non-negative integer".format(value))
    return ivalue


class ProcessState(Enum):
    RUNNING = 1
    STOPPED = 2
    TERMINATED = 3


class ProcessInfo:
    def __init__(self, handle):
        self.lock = threading.Lock()
        self.handle = handle
        self.state = ProcessState.RUNNING

    @staticmethod
    def state_to_signal(state):
        if state == ProcessState.RUNNING:
            return signal.SIGCONT

        if state == ProcessState.STOPPED:
            return signal.SIGSTOP

        if state == ProcessState.TERMINATED:
            return signal.SIGTERM

    @staticmethod
    def state_to_signal_str(state):
        if state == ProcessState.RUNNING:
            return "SIGCONT"

        if state == ProcessState.STOPPED:
            return "SIGSTOP"

        if state == ProcessState.TERMINATED:
            return "SIGTERM"

    @staticmethod
    def valid_state_transition(current, desired):
        if current == ProcessState.TERMINATED:
            return False

        if current == ProcessState.RUNNING:
            return desired == ProcessState.STOPPED or desired == ProcessState.TERMINATED

        if current == ProcessState.STOPPED:
            return desired == ProcessState.RUNNING

        return False


class AtomicSaturatedCounter:
    def __init__(self, saturation, initial=0):
        self._saturation = saturation
        self._value = initial
        self._lock = threading.Lock()

    def reserve(self):
        with self._lock:
            if self._value < self._saturation:
                self._value += 1
                return True
            else:
                return False


class Validation:
    def __init__(self, procs, msgs):
        self.processes = procs
        self.messages = msgs

    def generate_perfect_links_config(self, directory):
        hostsfile = os.path.join(directory, "hosts")
        configfile = os.path.join(directory, "config")

        with open(hostsfile, "w") as hosts:
            for i in range(1, self.processes + 1):
                hosts.write("{} localhost {}\n".format(i, PROCESSES_BASE_IP + i))

        with open(configfile, "w") as config:
            config.write("{} 1\n".format(self.messages))

        return (hostsfile, configfile)

    def generate_fifo_config(self, directory):
        hostsfile = os.path.join(directory, "hosts")
        configfile = os.path.join(directory, "config")

        with open(hostsfile, "w") as hosts:
            for i in range(1, self.processes + 1):
                hosts.write("{} localhost {}\n".format(i, PROCESSES_BASE_IP + i))

        with open(configfile, "w") as config:
            config.write("{}\n".format(self.messages))

        return (hostsfile, configfile)


class LatticeAgreementValidation:
    def __init__(self, processes, proposals, max_proposal_size, distinct_values):
        self.procs = processes
        self.props = proposals
        self.mps = max_proposal_size
        self.dval = distinct_values

    def generate(self, directory):
        hostsfile = os.path.join(directory, "hosts")

        with open(hostsfile, "w") as hosts:
            for i in range(1, self.procs + 1):
                hosts.write("{} localhost {}\n".format(i, PROCESSES_BASE_IP + i))

        maxint = 2**31 - 1
        seeded_rand = random.Random(42)
        try:
            values = seeded_rand.sample(range(0, maxint + 1), self.dval)
        except ValueError:
            print("Cannot have to many distinct values")
            sys.exit(1)

        configfiles = []
        for pid in range(1, self.procs + 1):
            configfile = os.path.join(directory, "proc{:02d}.config".format(pid))
            configfiles.append(configfile)

            with open(configfile, "w") as config:
                config.write("{} {} {}\n".format(self.props, self.mps, self.dval))

                for i in range(self.props):
                    proposal = seeded_rand.sample(
                        values, seeded_rand.randint(1, self.mps)
                    )
                    config.write(" ".join(map(str, proposal)))
                    config.write("\n")

        return (hostsfile, configfiles)


class StressTest:
    def __init__(self, procs, concurrency, attempts, attempts_ratio):
        self.processes = len(procs)
        self.processes_info = dict()
        for (logical_pid, handle) in procs:
            self.processes_info[logical_pid] = ProcessInfo(handle)
        self.concurrency = concurrency
        self.attempts = attempts
        self.attempts_ratio = attempts_ratio

        max_terminated_processes = (
            self.processes // 2
            if self.processes % 2 == 1
            else (self.processes - 1) // 2
        )
        self.terminated_procs = AtomicSaturatedCounter(max_terminated_processes)

    def stress(self):
        select_proc = list(range(1, self.processes + 1))
        random.shuffle(select_proc)

        select_op = (
            [ProcessState.STOPPED] * int(1000 * self.attempts_ratio["STOP"])
            + [ProcessState.RUNNING] * int(1000 * self.attempts_ratio["CONT"])
            + [ProcessState.TERMINATED] * int(1000 * self.attempts_ratio["TERM"])
        )
        random.shuffle(select_op)

        successful_attempts = 0
        while successful_attempts < self.attempts:
            proc = random.choice(select_proc)
            op = random.choice(select_op)
            info = self.processes_info[proc]

            with info.lock:
                if ProcessInfo.valid_state_transition(info.state, op):

                    if op == ProcessState.TERMINATED:
                        reserved = self.terminated_procs.reserve()
                        if reserved:
                            print(
                                "Sending {} to process {}".format(
                                    ProcessInfo.state_to_signal_str(op), proc
                                )
                            )
                            select_proc.remove(proc)
                        else:
                            continue

                    time.sleep(float(random.randint(50, 500)) / 1000.0)
                    info.handle.send_signal(ProcessInfo.state_to_signal(op))
                    info.state = op
                    successful_attempts += 1
                    print(
                        "Sending {} to process {}".format(
                            ProcessInfo.state_to_signal_str(op), proc
                        )
                    )

                    # if op == ProcessState.TERMINATED and proc not in terminated_procs:
                    #     if len(terminated_procs) < max_terminated_processes:

                    #         terminated_procs.add(proc)

                    # if len(terminated_procs) == max_terminated_processes:
                    #     break

    def remaining_unterminated_processes(self):
        remaining = []
        for pid, info in self.processes_info.items():
            with info.lock:
                if info.state != ProcessState.TERMINATED:
                    remaining.append(pid)

        return None if len(remaining) == 0 else remaining

    def terminate_all_processes(self):
        for _, info in self.processes_info.items():
            with info.lock:
                if info.state != ProcessState.TERMINATED:
                    if info.state == ProcessState.STOPPED:
                        info.handle.send_signal(
                            ProcessInfo.state_to_signal(ProcessState.RUNNING)
                        )

                    info.handle.send_signal(
                        ProcessInfo.state_to_signal(ProcessState.TERMINATED)
                    )

        return False

    def continue_stopped_processes(self):
        for _, info in self.processes_info.items():
            with info.lock:
                if info.state != ProcessState.TERMINATED:
                    if info.state == ProcessState.STOPPED:
                        info.handle.send_signal(
                            ProcessInfo.state_to_signal(ProcessState.RUNNING)
                        )

    def run(self):
        if self.concurrency > 1:
            threads = [
                threading.Thread(target=self.stress) for _ in range(self.concurrency)
            ]
            [p.start() for p in threads]
            [p.join() for p in threads]
        else:
            self.stress()


def start_processes(processes, runscript, hosts_file_path, config_file_paths, output_dir):
    runscript_path = os.path.abspath(runscript)
    if not os.path.isfile(runscript_path):
        raise Exception(f"`{runscript_path}` is not a file")

    if os.path.basename(runscript_path) != "run.sh":
        raise Exception(f"`{runscript_path}` is not a runscript")

    output_dir_path = os.path.abspath(output_dir)
    if not os.path.isdir(output_dir_path):
        raise Exception(f"`{output_dir_path}` is not a directory")

    base_dir, _ = os.path.split(runscript_path)
    bin_cpp = os.path.join(base_dir, "bin", "da_proc")
    bin_java = os.path.join(base_dir, "bin", "da_proc.jar")

    if os.path.exists(bin_cpp):
        cmd = [bin_cpp]
    elif os.path.exists(bin_java):
        cmd = ["java", "-jar", bin_java]
    else:
        raise Exception(
            f"`{runscript_path}` could not find a binary to execute. Make sure you build before validating"
        )

    procs = []
    for pid, config_path in zip(
        range(1, processes + 1), itertools.cycle(config_file_paths)
    ):
        cmd_ext = [
            "--id",
            str(pid),
            "--hosts",
            hosts_file_path,
            "--output",
            os.path.join(output_dir_path, "proc{:02d}.output".format(pid)),
            config_path,
        ]

        stdout_fd = open(
            os.path.join(output_dir_path, "proc{:02d}.stdout".format(pid)), "w"
        )
        stderr_fd = open(
            os.path.join(output_dir_path, "proc{:02d}.stderr".format(pid)), "w"
        )

        procs.append(
            (pid, subprocess.Popen(cmd + cmd_ext, stdout=stdout_fd, stderr=stderr_fd))
        )

    return procs


def distribution_type(value):
    try:
        # Parse string like "STOP:0.48,CONT:0.48,TERM:0.04"
        pairs = [pair.split(':') for pair in value.split(',')]
        dist = {k: float(v) for k, v in pairs}
        
        # Validate the distribution
        required_keys = {'STOP', 'CONT', 'TERM'}
        if set(dist.keys()) != required_keys:
            raise ValueError("Must include STOP, CONT, and TERM")
        if not abs(sum(dist.values()) - 1.0) < 0.001:
            raise ValueError("Probabilities must sum to 1.0")
        return dist
    except:
        raise argparse.ArgumentTypeError("Must be in format 'STOP:0.48,CONT:0.48,TERM:0.04'")


def main(parser_results):
    cmd = parser_results.command
    runscript = parser_results.runscript
    logs_dir = parser_results.logs_dir
    processes = parser_results.processes

    if not os.path.isdir(logs_dir):
        raise ValueError(f"Directory `{logs_dir}` does not exist")

    if cmd == "perfect":
        validation = Validation(processes, parser_results.messages)
        hosts_file, config_file = validation.generate_perfect_links_config(logs_dir)
        config_files = [config_file]
    elif cmd == "fifo":
        validation = Validation(processes, parser_results.messages)
        hosts_file, config_file = validation.generate_fifo_config(logs_dir)
        config_files = [config_file]
    elif cmd == "agreement":
        proposals = parser_results.proposals
        pmv = parser_results.proposal_max_values
        pdv = parser_results.proposals_distinct_values

        if pmv > pdv:
            print(
                "The distinct proposal values must at least as many as the maximum values per proposal"
            )
            sys.exit(1)

        validation = LatticeAgreementValidation(processes, proposals, pmv, pdv)
        hosts_file, config_files = validation.generate(logs_dir)
    else:
        raise ValueError("Unrecognized command")

    try:
        # Start the processes and get their PIDs
        procs = start_processes(processes, runscript, hosts_file, config_files, logs_dir)

        # Create the stress test
        st = StressTest(
            procs,
            args.concurrency,
            args.attempts,
            args.attempts_distribution
        )

        for (logical_pid, proc_handle) in procs:
            print(
                "Process with logicalPID {} has PID {}".format(
                    logical_pid, proc_handle.pid
                )
            )

        st.run()
        print("Stress test done.")

        print("Resuming stopped processes.")
        st.continue_stopped_processes()

        if args.timeout is None:
            input("Press `Enter` when all processes have finished processing messages.")
        else:
            print(f"Waiting for {args.timeout} seconds before terminating")
            time.sleep(args.timeout)

        unterminated = st.remaining_unterminated_processes()
        if unterminated is not None:
            st.terminate_all_processes()

        mutex = threading.Lock()

        def wait_for_process(logical_pid, proc_handle, mutex):
            proc_handle.wait()

            with mutex:
                print(
                    "Process {} exited with {}".format(
                        logical_pid, proc_handle.returncode
                    )
                )

        # Monitor which processes have exited
        monitors = [
            threading.Thread(
                target=wait_for_process, args=(logical_pid, proc_handle, mutex)
            )
            for (logical_pid, proc_handle) in procs
        ]
        [p.start() for p in monitors]
        [p.join() for p in monitors]

    finally:
        if procs is not None:
            for _, p in procs:
                p.kill()


if __name__ == "__main__":
    # Create argument parser
    parser = argparse.ArgumentParser()

    # Add subparsers for each milestone
    sub_parsers = parser.add_subparsers(dest="command", help="Stress a given milestone")
    sub_parsers.required = True
    parser_perfect = sub_parsers.add_parser("perfect", help="Stress perfect links")
    parser_fifo = sub_parsers.add_parser("fifo", help="Stress fifo broadcast")
    parser_agreement = sub_parsers.add_parser("agreement", help="Stress lattice agreement")

    for subparser in [parser_perfect, parser_fifo, parser_agreement]:
        subparser.add_argument(
            "-r",
            "--runscript",
            required=True,
            dest="runscript",
            help="Path to run.sh",
        )

        subparser.add_argument(
            "-l",
            "--logs",
            required=True,
            dest="logs_dir",
            help="Directory to store stdout, stderr and outputs generated by the processes",
        )

        subparser.add_argument(
            "-p",
            "--processes",
            required=True,
            type=positive_int,
            dest="processes",
            help="Number of processes that broadcast",
        )

        subparser.add_argument(
            "-a",
            "--attempts",
            type=non_negative_int,
            default=8,
            dest="attempts",
            help="Number of attempts to run the stress test",
        )

        subparser.add_argument(
            "-c",
            "--concurrency",
            type=positive_int,
            default=8,
            dest="concurrency",
            help="Number of threads to run the stress test",
        )

        subparser.add_argument(
            "-ad",
            "--attempts-distribution",
            type=distribution_type,
            default="STOP:0.48,CONT:0.48,TERM:0.04",
            dest="attempts_distribution",
            help="Distribution of the attempts (format: STOP:0.48,CONT:0.48,TERM:0.04)",
        )

    for subparser in [parser_perfect, parser_fifo]:
        subparser.add_argument(
            "-m",
            "--messages",
            required=True,
            type=positive_int,
            dest="messages",
            help="Maximum number (because it can crash) of messages that each process can broadcast",
        )

    parser_perfect.add_argument(
        "-t",
        "--timeout",
        type=positive_int,
        dest="timeout",
        help="Timeout in seconds after which the program will terminate",
    )
    
    parser_agreement.add_argument(
        "-n",
        "--proposals",
        required=True,
        type=positive_int,
        dest="proposals",
        help="Maximum number (because it can crash) of proposal that each process can make",
    )

    parser_agreement.add_argument(
        "-v",
        "--proposal-values",
        required=True,
        type=positive_int,
        dest="proposal_max_values",
        help="Maximum size of the proposal set that each process proposes",
    )

    parser_agreement.add_argument(
        "-d",
        "--distinct-values",
        required=True,
        type=positive_int,
        dest="proposals_distinct_values",
        help="The number of distinct values among all proposals",
    )

    args = parser.parse_args()

    main(args)
