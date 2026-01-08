# Section 23: Procfs Debug Interface

## Learning Outcomes

- Create debug interfaces via `/proc` filesystem
- Implement seq_file for efficient proc output
- Handle proc file reads and writes
- Use procfs for non-production debugging and statistics
- Compare procfs vs sysfs vs debugfs usage patterns

## Key Concepts

### Why Procfs?

- **Legacy debugging interface** (predates sysfs)
- **Variable-length output** (seq_file handles pagination)
- **Admin/debug only** (typically root-readable)
- **Easy creation** with `proc_create()`

### Seq_file Interface

```c
// Seq_file handlers
static int my_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Value: %d\n", my_value);
    seq_printf(m, "Count: %d\n", my_count);
    return 0;
}

static int my_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, my_proc_show, NULL);
}

static const struct file_operations proc_ops = {
    .open = my_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

proc_create("mydevice", 0444, NULL, &proc_ops);
```

### Write Handler for Proc

```c
static ssize_t my_proc_write(struct file *file, const char __user *buf,
                            size_t count, loff_t *ppos)
{
    char cmd[32];
    
    if (count >= sizeof(cmd)) return -EINVAL;
    
    if (copy_from_user(cmd, buf, count)) return -EFAULT;
    
    if (strncmp(cmd, "reset", 5) == 0) {
        my_count = 0;
        printk(KERN_INFO "Counter reset\n");
    }
    
    return count;
}

static const struct file_operations proc_ops = {
    .open = my_proc_open,
    .read = seq_read,
    .write = my_proc_write,
    .llseek = seq_lseek,
    .release = single_release,
};
```

## Complete Example: Statistics Collector via Procfs

```c
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>

struct device_stats {
    unsigned long events;
    unsigned long errors;
    unsigned long reads;
    unsigned long writes;
    unsigned long last_reset;
    struct mutex lock;
};

static struct device_stats stats = {
    .events = 0,
    .errors = 0,
    .reads = 0,
    .writes = 0,
    .lock = __MUTEX_INITIALIZER(stats.lock),
};

// Simulate incrementing stats
static void simulate_activity(void)
{
    mutex_lock(&stats.lock);
    stats.events++;
    
    if (random32() % 100 < 10) {
        stats.errors++;
    }
    
    if (random32() % 2) {
        stats.reads++;
    } else {
        stats.writes++;
    }
    
    mutex_unlock(&stats.lock);
}

// Proc show handler
static int stats_proc_show(struct seq_file *m, void *v)
{
    mutex_lock(&stats.lock);
    
    seq_printf(m, "Device Statistics\n");
    seq_printf(m, "=================\n");
    seq_printf(m, "Total Events:    %lu\n", stats.events);
    seq_printf(m, "Errors:          %lu (%.2f%%)\n", 
               stats.errors, 
               stats.events ? (100.0 * stats.errors / stats.events) : 0);
    seq_printf(m, "Read Ops:        %lu\n", stats.reads);
    seq_printf(m, "Write Ops:       %lu\n", stats.writes);
    
    unsigned long uptime_sec = (jiffies - stats.last_reset) / HZ;
    seq_printf(m, "Uptime:          %lu seconds\n", uptime_sec);
    
    mutex_unlock(&stats.lock);
    
    return 0;
}

static int stats_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, stats_proc_show, NULL);
}

// Proc write handler for commands
static ssize_t stats_proc_write(struct file *file, const char __user *buf,
                               size_t count, loff_t *ppos)
{
    char cmd[16];
    
    if (count >= sizeof(cmd)) return -EINVAL;
    if (copy_from_user(cmd, buf, count)) return -EFAULT;
    
    if (strncmp(cmd, "reset", 5) == 0) {
        mutex_lock(&stats.lock);
        stats.events = 0;
        stats.errors = 0;
        stats.reads = 0;
        stats.writes = 0;
        stats.last_reset = jiffies;
        mutex_unlock(&stats.lock);
        
        printk(KERN_INFO "Statistics reset\n");
    } else if (strncmp(cmd, "sim", 3) == 0) {
        simulate_activity();
        printk(KERN_INFO "Simulated activity\n");
    }
    
    return count;
}

static const struct file_operations stats_proc_ops = {
    .owner = THIS_MODULE,
    .open = stats_proc_open,
    .read = seq_read,
    .write = stats_proc_write,
    .llseek = seq_lseek,
    .release = single_release,
};

// Module init/exit

static struct proc_dir_entry *proc_entry;

static int __init stats_proc_init(void)
{
    proc_entry = proc_create("device_stats", 0644, NULL, &stats_proc_ops);
    if (!proc_entry) {
        printk(KERN_ERR "Failed to create proc entry\n");
        return -ENOMEM;
    }
    
    stats.last_reset = jiffies;
    
    printk(KERN_INFO "Procfs debug driver loaded\n");
    printk(KERN_INFO "View: cat /proc/device_stats\n");
    printk(KERN_INFO "Reset: echo reset > /proc/device_stats\n");
    
    return 0;
}

static void __exit stats_proc_exit(void)
{
    proc_remove(proc_entry);
    
    printk(KERN_INFO "Procfs debug driver unloaded\n");
}

module_init(stats_proc_init);
module_exit(stats_proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Student");
MODULE_DESCRIPTION("Device statistics via procfs");
```

## Makefile

```makefile
obj-m += stats_proc.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
 make -C $(KDIR) M=$(PWD) modules

clean:
 make -C $(KDIR) M=$(PWD) clean

test: all
 sudo insmod stats_proc.ko
 cat /proc/device_stats
 echo sim > /proc/device_stats
 cat /proc/device_stats
 echo reset > /proc/device_stats
 cat /proc/device_stats
 sudo rmmod stats_proc
```

## Build and Test

```bash
make
sudo insmod stats_proc.ko
cat /proc/device_stats
echo sim > /proc/device_stats
cat /proc/device_stats
echo reset > /proc/device_stats
sudo rmmod stats_proc
```

## Expected Output

```
Device Statistics
=================
Total Events:    0
Errors:          0 (0.00%)
Read Ops:        0
Write Ops:       0
Uptime:          5 seconds
```

## Important Notes

1. **Procfs is legacy** - new code should prefer sysfs for device attributes
2. **seq_file is required** for large outputs (handles pagination)
3. **copy_from_user required** for write operations (not kernel memory)
4. **Per-line limits** - seq_printf automatically handles buffer overflow
5. **Permissions matter** - 0444 makes read-only, 0644 allows write
6. **Cleanup critical** - proc_remove() before module exit
