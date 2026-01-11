# 01 Introduction to Linux Kernel Programming - Q&A

## Assignments

### Practical Tasks

1. **Find your kernel version:** `uname -r`

   ```
   ahmed@ahmed:~$ uname -r
   6.8.0-90-generic
   ```

2. **List 5 running processes:** `ps aux | head -5`

   ```
   ahmed@ahmed:~$ ps aux | head -5
   USER         PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
   root           1  0.0  0.2 167964  8820 ?        Ss   Jan08   0:01 /sbin/init splash
   root           2  0.0  0.0      0     0 ?        S    Jan08   0:00 [kthreadd]
   root           3  0.0  0.0      0     0 ?        S    Jan08   0:00 [pool_workqueue_release]
   root           4  0.0  0.0      0     0 ?        I<   Jan08   0:00 [kworker/R-rcu_g]
   ```

3. **View loaded kernel modules:** `lsmod`

   ```
   ahmed@ahmed:~$ lsmod
   Module                  Size  Used by
   tls                   155648  0
   xt_recent              24576  0
   tcp_diag               12288  0
   udp_diag               12288  0
   inet_diag              28672  2 tcp_diag,udp_diag
   ```

4. **Explore system uptime:** `cat /proc/uptime`

   ```
   ahmed@ahmed:~$ cat /proc/uptime
   80119.30 239306.10
   ```

5. **Create a folder and file:**

   ```bash
   mkdir ~/mytest
   cd ~/mytest
   touch sample.txt
   ```

---

## Questions & Answers

### Q1: What is the Linux Kernel?

The Linux Kernel is the core of the OS that manages hardware resources like CPU, memory, and devices. It runs in a privileged "kernel space" and acts as a bridge between hardware and user applications running in restricted "user space". The kernel provides scheduling algorithms and memory management with huge driver support across different architectures.

### Q2: What is a Kernel Module?

A kernel module is code that can be dynamically loaded into the kernel at runtime using `insmod` or `modprobe`, without needing to reboot. They're commonly used for device drivers and optional features that can be added or removed as needed.

### Q3: Why Should Beginners Use a VM for Kernel Experiments?

VMs provide a safe sandbox where experimentation is possible without affecting the main system. If issues occur, snapshots can be reverted easily. They are cost-effective, isolated, and allow testing of multiple configurations without risking data loss.

### Q4: What is the Purpose of the /proc Folder?

The /proc folder is a virtual filesystem that exposes real-time kernel and system information through files like `/proc/cpuinfo`, `/proc/meminfo`, and `/proc/[pid]/` for individual processes. It's essential for monitoring the system and debugging, with files generated on-the-fly without consuming disk space. It's loaded into RAM during boot.

### Q5: Open the /dev Folder and Identify

**Which files are related to your hard drive?**

- `/dev/sda`
- `/dev/sda1`
- Mainly files starting with b - command ls -la | grep ^b

**Which might belong to your keyboard or mouse?**

- `/dev/input/event0`
- `/dev/input/mouse0`
- Mainly files starting with c - command ls -la | grep ^c
