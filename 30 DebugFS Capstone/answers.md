# Section 30: DebugFS Capstone – Advanced Debugging Assignment

## Task 1: Theory Questions

1. **DebugFS Purpose**: Kernel debugging interface for developers. Flexible, admin-only, not part of stable API. No performance overhead when unused.

2. **DEFINE_SIMPLE_ATTRIBUTE**: Macro generating getter/setter handlers for u64 values. Simplifies u64 file creation with automatic formatting.

3. **Seq_file in DebugFS**: Enables large output handling with automatic pagination. Single_open wrapper for simple outputs.

4. **Recursive Cleanup**: debugfs_remove_recursive() removes entire directory tree. Critical for preventing memory leaks on module unload.

5. **Permission Model**: Files typically 0644 (user-readable) or 0444 (read-only). Root required for write access to debug parameters.

## Task 2: Performance Monitoring via DebugFS

Implement performance counter interface:

```c
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/atomic.h>

static atomic_t perf_read_ops;
static atomic_t perf_write_ops;
static atomic_t perf_errors;

static int perf_show(struct seq_file *m, void *v)
{
    int reads = atomic_read(&perf_read_ops);
    int writes = atomic_read(&perf_write_ops);
    int errors = atomic_read(&perf_errors);
    int total = reads + writes;
    
    seq_printf(m, "Performance Counters\n");
    seq_printf(m, "====================\n");
    seq_printf(m, "Read Operations:  %d\n", reads);
    seq_printf(m, "Write Operations: %d\n", writes);
    seq_printf(m, "Errors:           %d\n", errors);
    seq_printf(m, "Total Operations: %d\n", total);
    
    if (total > 0) {
        seq_printf(m, "Error Rate:       %.2f%%\n", (100.0 * errors) / total);
    }
    
    return 0;
}

static int perf_open(struct inode *inode, struct file *file)
{
    return single_open(file, perf_show, NULL);
}

static const struct file_operations perf_fops = {
    .open = perf_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int __init perf_init(void)
{
    struct dentry *dir = debugfs_create_dir("perf_monitor", NULL);
    
    debugfs_create_file("statistics", 0444, dir, NULL, &perf_fops);
    debugfs_create_atomic_t("reads", 0444, dir, &perf_read_ops);
    debugfs_create_atomic_t("writes", 0444, dir, &perf_write_ops);
    debugfs_create_atomic_t("errors", 0444, dir, &perf_errors);
    
    printk(KERN_INFO "Performance monitor loaded\n");
    return 0;
}

static void __exit perf_exit(void)
{
    debugfs_remove_recursive(debugfs_lookup("perf_monitor", NULL));
    printk(KERN_INFO "Performance monitor unloaded\n");
}

module_init(perf_init);
module_exit(perf_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Performance counter interface");
```

**Usage**: `cat /sys/kernel/debug/perf_monitor/statistics`

## Task 3: Comprehensive Device State Debugger

```c
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>

#define MAX_STATE_ITEMS 16

struct device_state {
    int state_values[MAX_STATE_ITEMS];
    int active;
    unsigned long last_update;
    struct mutex lock;
    struct dentry *debugfs_dir;
};

static struct device_state dev_state;

static int state_show(struct seq_file *m, void *v)
{
    int i;
    
    mutex_lock(&dev_state.lock);
    
    seq_printf(m, "Device State\n");
    seq_printf(m, "============\n");
    seq_printf(m, "Active:       %s\n", dev_state.active ? "YES" : "NO");
    seq_printf(m, "Last Update:  %lu jiffies ago\n", jiffies - dev_state.last_update);
    seq_printf(m, "\nState Values:\n");
    
    for (i = 0; i < MAX_STATE_ITEMS; i++) {
        seq_printf(m, "  [%2d] = 0x%08x\n", i, dev_state.state_values[i]);
    }
    
    mutex_unlock(&dev_state.lock);
    return 0;
}

static int state_open(struct inode *inode, struct file *file)
{
    return single_open(file, state_show, NULL);
}

static const struct file_operations state_fops = {
    .open = state_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int __init debug_init(void)
{
    int i;
    
    dev_state.debugfs_dir = debugfs_create_dir("device_debug", NULL);
    
    debugfs_create_file("state", 0444, dev_state.debugfs_dir, NULL, &state_fops);
    debugfs_create_bool("active", 0644, dev_state.debugfs_dir, &dev_state.active);
    
    // Initialize sample values
    for (i = 0; i < MAX_STATE_ITEMS; i++) {
        dev_state.state_values[i] = 0x1000 + i;
    }
    
    dev_state.active = 1;
    dev_state.last_update = jiffies;
    mutex_init(&dev_state.lock);
    
    printk(KERN_INFO "Device debug interface loaded\n");
    return 0;
}

static void __exit debug_exit(void)
{
    debugfs_remove_recursive(dev_state.debugfs_dir);
    printk(KERN_INFO "Device debug interface unloaded\n");
}

module_init(debug_init);
module_exit(debug_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Comprehensive device state debugger");
```

**Usage**: `cat /sys/kernel/debug/device_debug/state`

**Completion**: All 30 sections covering foundational (procfs) → intermediate (devices) → advanced (interrupts, power, debugging) Linux kernel device driver programming patterns!
