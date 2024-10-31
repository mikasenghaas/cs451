# CS451 (Distributed Algorithms) 2024 - EPFL

The goal of this practical project is to implement building blocks in decentralized systems:

  - Perfect Links (Milestone #1),
  - FIFO Uniform Reliable Broadcast (Milestone #2),
  - Lattice Agreement (Milestone #3)

## Installation Guide

This is my personal guide to have a nice workflow for this project from a Mac ARM machine.

### Setting up the Virtual Machine
1. Download the VM image from [here](https://drive.google.com/file/d/13Cx0s23CIlxz8t_AHrNfE-68fYh5OPNy/view?usp=sharing) and extract it
2. Install UTM (Virtual Machine Manager) via Homebrew with `brew install utm`
3. Enable SSH on UTM (following this [guide](https://arteen.linux.ucla.edu/ssh-into-utm-vm.html))
  - Click on your target VM in the UTM UI.
  - Navigate to the top right button (Settings) and click it.
  - Go to Network and set Network Mode to Emulated VLAN.
  - Click on Port Forwarding (it should show up under Network).
  - Click 'New' and select Protocol: TCP
  - Set up Guest Port (second field) to 22 and Host Port (fourth field) to your choice (I use 2222).
4. Open UTM and import the downloaded image by dragging it into the UTM interface
5. Start the virtual machine

### Configuring SSH Access
First, we need to set up SSH on the virtual machine:

1. Log into the VM and open a terminal
2. Set the keyboard layout to US:
   ```bash
   setxkbmap us
   ```

3. Install and configure SSH:
   ```bash
   # Update package list and install SSH
   sudo apt update
   sudo apt install ssh
   
   # Start SSH service and enable it on startup
   sudo systemctl start ssh
   sudo systemctl enable ssh 
   sudo systemctl status sshd
   ```

4. Shutdown the VM gracefully to apply changes:
   ```bash
   sudo shutdown now
   ```

### Setting up Git and SSH Keys
After configuring SSH on the VM, set up Git and SSH keys from your local machine:

1. Copy your SSH public key to the VM:
   ```bash
   ssh-copy-id -i ~/.ssh/id_rsa.pub -p 2222 dcl@localhost
   ```

2. Configure Git on the VM by copying your local configurations:
   ```bash
   scp -P 2222 .gitconfig dcl@localhost:~/.gitconfig
   scp -P 2222 .sshconfig dcl@localhost:~/.ssh/config
   scp -P 2222 ~/.ssh/github-personal dcl@localhost:~/.ssh/github-personal
   ```

3. Restart the VM and connect to it via SSH:
   ```bash
   ssh user@localhost -p 2222
   ```

I then develop on the VM directly via VSCode's Remote-SSH connection.