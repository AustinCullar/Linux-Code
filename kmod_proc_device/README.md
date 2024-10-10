Kernel Module Creating Proc Device

# Overview
This directory contains code for building a Linux kernel module and a userspace
'agent', which communicates with the kernel module through a proc device.

Environment:
- Ubuntu 22.04.5 LTS
- Linux kernel version: 6.8.0-45-generic
- VirtualBox VM

# Building
This code is built using GNU make. Once the repo has been cloned, you should be
able to enter the `Linux-Code/kmod_proc_device` directory and run `make` to
build both the kmod and the userspace agent.

Once built, you should find the kmod at `kernel/kernel_util.c` and the userspace
agent at `userspace/agent`.

# Running the Application
Note: you will need to execute this application as `root`.

To run the code here, execute the userspace `agent` application providing the
location of the kmod file as follows:

```
$ userspace/agent kernel/kernel_util.ko
```

The agent will load the kmod file via the `insmod` system call, and then write
some data to the newly created proc file, `/proc/kernel_util`. The results can
be viewed on the console (stdout) as well as in the kernel logs using `dmesg`.

```
$ userspace/agent kernel/kernel_util.ko
Starting agent
Attempting to open proc device /proc/kernel_util
Received response: response 1
Attempting to open proc device /proc/kernel_util
Received response: response 2
```

Output in `dmesg`:
```
[ 9358.042667] kernel_util init
[ 9358.042789] data received in message:
[ 9358.042797] 		msg->msg_type: 1
[ 9358.042798] 		msg->buf_len: 12
[ 9358.042799] 		msg->buf: command one
[ 9358.042800] Received MONITOR command
[ 9358.043534] response_type: 1
[ 9358.043969] data received in message:
[ 9358.043971] 		msg->msg_type: 2
[ 9358.043971] 		msg->buf_len: 12
[ 9358.043972] 		msg->buf: command two
[ 9358.043973] Received UNMONITOR command
[ 9358.044903] response_type: 2
[ 9358.045344] kernel_util exit
```

Alternatively, you can load the kmod alone by running `insmod kernel_util.ko`.
This will allow you to view the proc device at `/proc/kernel_util`.

# Kernel module
The kernel module in this code is simply named "kernel_util". Upon loading, it
creates a proc device: `/proc/kernel_util`. This is a "pseudo-file", meaning
that although it appears in a directory listing as if it were a normal file, it
actually just acts as an interface to the kernel.

Any data written to the `/proc/kernel_util` file will be received by
`kernel_util` module for processing. Currently, the `kernel_util` kmod is
written to only expect data in the form of `struct message` (see
`include/common.h`), but you could have the kmod ingest any kind of data you
like from the proc device.

# Usersace agent
The userspace agent is a simple program that loads the kernel module using the
`insmod` system call, sends a couple of messages to the proc device, and then
finally unloads the kernel module with the `rmmod` system call.

Execute the agent by running:
```
$ ./agent <path to .ko file>
```
