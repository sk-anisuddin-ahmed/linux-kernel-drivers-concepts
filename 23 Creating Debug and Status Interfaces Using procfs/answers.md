# Section 23: Procfs Debug Interface – Assignment

## Task 1: Theory Questions

1. **Procfs vs Sysfs**: Procfs is legacy, variable-length output with seq_file; sysfs is structured device attributes. Procfs good for monitoring, sysfs for device control.

2. **Seq_file Purpose**: Handles pagination for large proc outputs, preventing buffer overflow. seq_printf automatically manages internal buffer.

3. **Write Handler Pattern**: copy_from_user() to transfer user data safely, parse command string, update kernel state with proper locking.

4. **Proc Permissions**: 0444 = read-only (root), 0644 = read-write for root. Higher bits ignored for proc files.

5. **Cleanup Requirements**: proc_remove() in module exit, must remove before module unload to prevent dangling references.

## Task 2: IRQ Counter via Procfs

Implement `/proc/irq_stats` tracking interrupt counts per device:

```c
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>

struct irq_stats {
    unsigned long irq_count[4];
    struct mutex lock;
};

static struct irq_stats irq_data = {
    .lock = __MUTEX_INITIALIZER(irq_data.lock),
};

static int irq_proc_show(struct seq_file *m, void *v)
{
    mutex_lock(&irq_data.lock);
    
    seq_printf(m, "IRQ Statistics\n");
    seq_printf(m, "==============\n");
    for (int i = 0; i < 4; i++) {
        seq_printf(m, "IRQ %d: %lu\n", i, irq_data.irq_count[i]);
    }
    
    unsigned long total = 0;
    for (int i = 0; i < 4; i++) {
        total += irq_data.irq_count[i];
    }
    seq_printf(m, "Total: %lu\n", total);
    
    mutex_unlock(&irq_data.lock);
    return 0;
}

static int irq_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, irq_proc_show, NULL);
}

static ssize_t irq_proc_write(struct file *file, const char __user *buf,
                             size_t count, loff_t *ppos)
{
    char cmd[32];
    
    if (count >= sizeof(cmd)) return -EINVAL;
    if (copy_from_user(cmd, buf, count)) return -EFAULT;
    
    if (strncmp(cmd, "reset", 5) == 0) {
        mutex_lock(&irq_data.lock);
        for (int i = 0; i < 4; i++) {
            irq_data.irq_count[i] = 0;
        }
        mutex_unlock(&irq_data.lock);
    }
    
    return count;
}

static const struct file_operations irq_proc_ops = {
    .open = irq_proc_open,
    .read = seq_read,
    .write = irq_proc_write,
    .llseek = seq_lseek,
    .release = single_release,
};

static int __init irq_proc_init(void)
{
    proc_create("irq_stats", 0644, NULL, &irq_proc_ops);
    printk(KERN_INFO "IRQ stats proc driver loaded\n");
    return 0;
}

static void __exit irq_proc_exit(void)
{
    proc_remove_entry("irq_stats", NULL);
    printk(KERN_INFO "IRQ stats driver unloaded\n");
}

module_init(irq_proc_init);
module_exit(irq_proc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("IRQ statistics via procfs");
```

**Usage**: `cat /proc/irq_stats` displays interrupt counts, `echo reset > /proc/irq_stats` resets.

## Task 3: Complex Statistics with Seq_file

Implement multi-entry proc interface for device activity tracking:

```c
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>

#define MAX_DEVICES 4

struct device_activity {
    unsigned long opens;
    unsigned long closes;
    unsigned long reads;
    unsigned long writes;
    unsigned long errors;
};

static struct device_activity devices[MAX_DEVICES];
static struct mutex dev_lock = __MUTEX_INITIALIZER(dev_lock);

static int dev_proc_show(struct seq_file *m, void *v)
{
    int i;
    
    mutex_lock(&dev_lock);
    
    seq_printf(m, "Device Activity Report\n");
    seq_printf(m, "======================\n\n");
    
    for (i = 0; i < MAX_DEVICES; i++) {
        seq_printf(m, "Device %d:\n", i);
        seq_printf(m, "  Opens:   %lu\n", devices[i].opens);
        seq_printf(m, "  Closes:  %lu\n", devices[i].closes);
        seq_printf(m, "  Reads:   %lu\n", devices[i].reads);
        seq_printf(m, "  Writes:  %lu\n", devices[i].writes);
        seq_printf(m, "  Errors:  %lu\n\n", devices[i].errors);
    }
    
    mutex_unlock(&dev_lock);
    return 0;
}

static int dev_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, dev_proc_show, NULL);
}

static const struct file_operations dev_proc_ops = {
    .open = dev_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int __init dev_proc_init(void)
{
    proc_create("device_activity", 0444, NULL, &dev_proc_ops);
    
    // Initialize sample data
    for (int i = 0; i < MAX_DEVICES; i++) {
        devices[i].opens = 10 + i;
        devices[i].reads = 100 + (i * 20);
        devices[i].writes = 50 + (i * 10);
        devices[i].errors = i;
    }
    
    printk(KERN_INFO "Device activity proc driver loaded\n");
    return 0;
}

static void __exit dev_proc_exit(void)
{
    proc_remove_entry("device_activity", NULL);
    printk(KERN_INFO "Device activity driver unloaded\n");
}

module_init(dev_proc_init);
module_exit(dev_proc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Device activity statistics via procfs");
```

**Usage**: `cat /proc/device_activity` shows detailed statistics for all devices.
