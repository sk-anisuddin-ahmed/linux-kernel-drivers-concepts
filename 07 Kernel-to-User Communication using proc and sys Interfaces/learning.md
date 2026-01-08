# 07 Kernel-to-User Communication using /proc and /sys Interfaces

## Learning Content

### Why Do We Need Kernel-to-User Communication?

Kernel modules run in kernel space, and user applications run in user space. But often, we need them to talk. For example:

- A user might want to know what a driver is doing
- You might want to display internal values or statistics from the module
- Or allow limited control of module behavior from user space

To do this safely, the Linux kernel exposes virtual filesystems like `/proc` and `/sys`.

### What is /proc?

The `/proc` filesystem is a virtual filesystem that provides a window into the kernel. It's not on disk — it's created in RAM. When you run:

```bash
cat /proc/cpuinfo
```

You're actually reading live data from the kernel — not a file on disk.

You can create your own `/proc` entries in your kernel module to expose information like:

- Configuration values
- Status flags
- Debug logs
- Counters or hardware info

### What is /sys?

`/sys` is similar to `/proc`, but it's more structured and object-oriented. It's used mostly by device drivers and kernel subsystems. We already saw that:

```
/sys/module/your_module/parameters/
```

Contains your `module_param()` values.

## Step-by-Step: Create a /proc Entry from a Module

Let's create a module that:

- Creates a file `/proc/myinfo`
- Writes some internal data to it
- Allows users to read that data using `cat /proc/myinfo`

### 1. Create the Module Source File

```bash
mkdir ~/proc_module
cd ~/proc_module
nano proc_demo.c
```

Paste this code:

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define PROC_NAME "myinfo"

static int show_myinfo(struct seq_file *m, void *v)
{
    seq_printf(m, "Hello from kernel space!\n");
    seq_printf(m, "Module: proc_demo\n");
    seq_printf(m, "Status: Running and happy\n");
    return 0;
}

static int open_myinfo(struct inode *inode, struct file *file)
{
    return single_open(file, show_myinfo, NULL);
}

static const struct proc_ops myinfo_fops = {
    .proc_open    = open_myinfo,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init proc_demo_init(void)
{
    proc_create(PROC_NAME, 0, NULL, &myinfo_fops);
    printk(KERN_INFO "/proc/%s created\n", PROC_NAME);
    return 0;
}

static void __exit proc_demo_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "/proc/%s removed\n", PROC_NAME);
}

module_init(proc_demo_init);
module_exit(proc_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Kernel module with /proc entry");
```

### 2. Create the Makefile

```makefile
obj-m += proc_demo.o

all:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

### 3. Build, Load, and Test the Module

```bash
make
sudo insmod proc_demo.ko
```

Check dmesg:

```bash
dmesg | tail -10
```

Now read the `/proc` entry:

```bash
cat /proc/myinfo
```

You should see:

```
Hello from kernel space!
Module: proc_demo
Status: Running and happy
```

### 4. Remove the Module

```bash
sudo rmmod proc_demo
cat /proc/myinfo
# You'll see: No such file or directory
```

## What is seq_file?

The Linux kernel provides a helper called `seq_file` to write output line-by-line in a memory-safe way. In our code:

- `show_myinfo()` prints into the `/proc` file
- `seq_printf()` safely prints lines
- `single_open()` sets up the file
- The `proc_ops` structure links it all

This may feel advanced, but you'll get more comfortable the more you use it.

## Key Concepts

### proc_create() Function

Creates a new `/proc` entry. Syntax:

```c
struct proc_dir_entry *proc_create(const char *name, umode_t mode,
                                    struct proc_dir_entry *parent,
                                    const struct proc_ops *proc_ops);
```

- `name`: Name of the `/proc` file
- `mode`: File permissions (e.g., 0 for read-only)
- `parent`: Parent directory (NULL for root)
- `proc_ops`: File operations structure

### remove_proc_entry() Function

Removes a `/proc` entry:

```c
void remove_proc_entry(const char *name, struct proc_dir_entry *parent);
```

### proc_ops Structure

Defines how to handle file operations:

```c
struct proc_ops {
    .proc_open    = function to open file
    .proc_read    = function to read data
    .proc_lseek   = function to seek in file
    .proc_release = function to release resources
};
```

## Advantages of /proc Interface

- Real-time data: Data is generated on-the-fly, not pre-computed
- No disk I/O: Virtual filesystem in memory only
- Easy debugging: Can inspect module state without special tools
- User-friendly: Any user can use `cat` to read data
- Flexible: Can display any kernel data you want

## Important Notes

- Always use `seq_printf()` for safe printing, not `sprintf()`
- Call `remove_proc_entry()` in your exit function to clean up
- Don't block in `/proc` read functions — keep them fast
- Use appropriate file permissions (typically 0 for read-only)
- Your `/proc` entry disappears when the module is unloaded
