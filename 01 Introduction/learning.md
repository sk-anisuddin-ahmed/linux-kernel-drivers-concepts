# 01 Introduction to Linux Kernel Programming

You will understand what the Linux kernel is, why it's important, how it relates to devices and software, and how to safely start exploring it on your computer. You'll also learn basic Linux commands and system structure so you're ready for kernel-level exploration.

## What is the Linux Kernel?

Imagine your computer as a giant factory. There are machines (like your CPU, RAM, keyboard, etc.), and there are managers (apps like browsers, games, editors). Now, between them is the main boss who coordinates everything. This boss is called the Linux kernel.

The Linux kernel is the core part of the operating system. It has one job: to manage how software (your apps) interacts with hardware (your devices).

When you type something, click a mouse, or run a program, the kernel is what actually makes it happen. It decides:

- Which app can use the CPU
- Where to store files in RAM or disk
- How to send signals to your screen
- How to receive data from your keyboard or USB

In short, the kernel is the bridge between your hardware and your software.

**Key Points:**

- The kernel is always running in the background.
- It runs in a special mode called kernel space (separate from apps, which run in user space).
- It handles memory, process control, device communication, and more.

## What is a Kernel Module?

Let's say your Linux system is like a toy robot. You buy an extra arm that plugs in and gives it a new feature like waving. Similarly, kernel modules are "plug-in parts" for the Linux kernel. They add extra functionality without restarting the system.

A kernel module is a small program you can insert into the kernel while the system is running. You can also remove it anytime. For example:

- A module may add support for a new USB device.
- Another may enable a custom file system.
- Some modules are written by users (like you!) to learn or build new tools.

Later in this course, you will learn how to write your own modules, load them, and even break (and fix!) them to learn kernel debugging.

**Important Commands (we'll try later):**

- `lsmod` → Show loaded modules
- `insmod` → Insert a module
- `rmmod` → Remove a module
- `modinfo` → Show details of a module

## Why Use a Virtual Machine (VM)?

Because the Linux kernel is so powerful, bugs in kernel code can crash your system. So we need a safe space where we can test without fear.

The best way is to install Ubuntu Linux in a Virtual Machine (VM) using tools like VirtualBox. This creates a "computer inside your computer" where you can do all your experiments. If anything goes wrong, you just restart the VM — your main PC is totally safe.

**Why use a VM for kernel practice?**

- You can break things safely.
- You can create snapshots (restore points).
- You don't need to dual boot.
- Ideal for beginners and testing new modules.

## Command Center

In Linux, the Terminal (or shell) is where the real action happens — especially for kernel work. It's a black screen where you type commands, and the system responds.

You'll spend most of your time here, especially when writing kernel modules.

**Practice These Basics:**

| Command | What it Does |
|---------|--------------|
| `whoami` | Shows your username |
| `pwd` | Shows current folder path |
| `ls` | Lists files in the current folder |
| `cd foldername` | Change to a folder |
| `mkdir newfolder` | Create a new folder |
| `cd ..` | Go up one folder |

Try combining them:

```
mkdir mytest
cd mytest
touch file1.txt
ls
```

This creates a folder, enters it, makes a file, and lists it.

## What Are Processes?

A process is simply a running program. When you open a browser, one process is created. Open another tab? Another process. The kernel is responsible for creating, destroying, and managing processes.

Each process has:

- A unique ID (PID)
- Memory space
- CPU time
- State (running, sleeping, etc.)

Try this:

```
ps aux | head -10
```

This shows a list of active processes. You'll see:

- `PID` (Process ID)
- `USER` (Who owns it)
- `COMMAND` (What it's doing)

The kernel handles hundreds of such processes — switching between them rapidly.

## Kernel in Action: Try These Commands

Now, let's look at how to peek into the kernel's brain. Try these:

**uname -r**

This shows the version of the kernel you're using (e.g., `6.1.0-xyz`).

**lsmod**

Shows all currently loaded kernel modules.

**dmesg | less**

Shows kernel boot and runtime messages — this is how the kernel "talks".

**cd /proc**
**ls**

This enters a special folder where the kernel exposes live system data as files. For example:

- `/proc/cpuinfo` → CPU details
- `/proc/meminfo` → RAM usage
- `/proc/uptime` → System uptime

## Linux File System

The Linux file system starts at `/` (called "root") and spreads like a tree.

**Important folders:**

- `/home` – Your files live here
- `/bin`, `/sbin` – System programs
- `/dev` – Device files (e.g., `/dev/sda` for hard drive)
- `/proc` – Runtime system data from the kernel
- `/sys` – Interfaces for devices and kernel objects

**Try:**

```
ls /
```

**Explore:**

```
cd /proc
cat uptime
cat cpuinfo
```

The kernel turns real-time data into text files for you to read!
