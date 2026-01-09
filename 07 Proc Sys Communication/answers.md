# 07 Proc Communication - Assignment Tasks

## Assignment Tasks

**Task 1: Short Answer Questions**

1. **What is the /proc filesystem and how is it different from a regular filesystem?**
   The `/proc` filesystem is a virtual filesystem that displays kernel runtime information (cpuinfo, meminfo, process data, etc.) without consuming disk space. Unlike regular filesystems, `/proc` is dynamically generated from kernel memory, exists only while the system is running, and disappears after shutdown.

2. **Explain what `seq_printf()` does and why it's preferred over regular `sprintf()`**
   `seq_printf()` is used in proc driver creation in the show method. `sprintf()` is irrelevant in this context.

3. **What is the purpose of `proc_create()` and what are its main parameters?**
   `proc_create(name, permissions, parent_dir, file_ops)` creates a `/proc` file entry with the given name and permissions and associates the file with file_operations structure.

4. **What happens if you forget to call `remove_proc_entry()` in your exit function?**
   The `/proc` entry remains registered even after the module is unloaded, causing kernel crashes when users try to access the removed module's functions.

5. **How does the kernel handle multiple users reading `/proc/myinfo` simultaneously?**
   The kernel uses proper locking mechanisms (mutexes or spin locks) in the file operations to serialize access.

**Task 2: Custom Data Display**

Modify the `proc_demo` module to:

1. Display the number of times the `/proc/myinfo` file has been read
2. Show a timestamp (using `jiffies`) when the file was first accessed
3. Add a module parameter `author_name` and display it in the output

**Implementation**

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>
#include <linux/printk.h>

static int access_count = 0;
static unsigned long start_time;

static int info_show(struct seq_file *file, void* d)
{
    access_count++;
    unsigned long elapsed = (jiffies - start_time) / HZ;
    
    seq_printf(file, "Name: Anis\n");
    seq_printf(file, "Age: 27\n");
    seq_printf(file, "Address: Bangalore\n");
    seq_printf(file, "Access Count: %d\n", access_count);
    seq_printf(file, "Seconds Since Load: %ld\n", elapsed);
    return 0;
}

static int info_open(struct inode* inode, struct file* file)
{
    return single_open(file, info_show, NULL);
}

static struct proc_ops info_fops = {
    .proc_open = info_open,
    .proc_read = seq_read,
    .proc_release = seq_release
};

static int __init proc_demo_init(void)
{
    start_time = jiffies;
    proc_create("myinfo", 0644, NULL, &info_fops);
    pr_info("Proc demo module loaded\n");
    return 0;
}

static void __exit proc_demo_exit(void)
{
    remove_proc_entry("myinfo", NULL);
    pr_info("Proc accessed %d times\n", access_count);
}

MODULE_LICENSE("GPL");
module_init(proc_demo_init);
module_exit(proc_demo_exit);
```

**Output**

```bash
root@ahmed:/proc/Anis# cat *
Bangalore
27
Seconds Passed Since Module Loaded: 28
Anis
```

**Task 3: Explore Built-in /proc Entries**

Run the following commands and examine the output:

```bash
cat /proc/uptime
cat /proc/version
cat /proc/modules
cat /proc/meminfo
cat /proc/cpuinfo
```

**Answers:**

1. **What type of information does each file provide?**
   - `/proc/uptime`: System uptime and idle time in seconds
   - `/proc/version`: Kernel version, compiler, and build information
   - `/proc/modules`: All loaded kernel modules with sizes and usage counts
   - `/proc/meminfo`: Memory statistics (total, free, buffers, cached, swap)
   - `/proc/cpuinfo`: CPU information (count, architecture, flags, speeds)

2. **Which file shows all currently loaded modules?**
   - `/proc/modules` displays all loaded kernel modules

3. **Which file shows your kernel version?**
   - `/proc/version` displays the kernel version and build details

4. **What information can you get from `/proc/meminfo`?**
   - Total RAM, available memory, free memory, buffers, cached memory, swap total/free, and other memory statistics

**Task 4: Create a Counter /proc Entry**

Create a new kernel module that:

1. Creates a `/proc/counter` entry
2. Implements a counter that increments every time the module is initialized (keep it global)
3. Displays the counter value when read
4. Shows how many times it's been read
5. Clean up properly on module exit

**Implementation:**

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static int init_count = 0;
static int read_count = 0;

static int counter_show(struct seq_file *file, void* d)
{
    read_count++;
    seq_printf(file, "Module Init Count: %d\n", init_count);
    seq_printf(file, "File Read Count: %d\n", read_count);
    seq_printf(file, "Total Accesses: %d\n", init_count + read_count);
    return 0;
}

static int counter_open(struct inode* inode, struct file* file)
{
    return single_open(file, counter_show, NULL);
}

static struct proc_ops counter_fops = {
    .proc_open = counter_open,
    .proc_read = seq_read,
    .proc_release = seq_release
};

static int __init counter_init(void)
{
    init_count++;
    proc_create("counter", 0644, NULL, &counter_fops);
    pr_info("Counter module loaded (Init count: %d)\n", init_count);
    return 0;
}

static void __exit counter_exit(void)
{
    remove_proc_entry("counter", NULL);
    pr_info("Counter module unloaded (Total reads: %d)\n", read_count);
}

MODULE_LICENSE("GPL");
module_init(counter_init);
module_exit(counter_exit);
```
