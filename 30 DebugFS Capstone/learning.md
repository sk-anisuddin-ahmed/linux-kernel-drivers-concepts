# Section 30: DebugFS – Advanced Kernel Debugging

## DebugFS Basics

DebugFS provides kernel debugging interface via `/sys/kernel/debug/` (requires CONFIG_DEBUG_FS).

### Creating DebugFS Files

```c
static int debug_value = 42;

static int debug_value_get(void *data, u64 *val)
{
    *val = debug_value;
    return 0;
}

static int debug_value_set(void *data, u64 val)
{
    debug_value = val;
    return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_fops, debug_value_get, debug_value_set, "%llu\n");

struct dentry *debugfs_dir = debugfs_create_dir("my_driver", NULL);
debugfs_create_file("debug_value", 0644, debugfs_dir, NULL, &debug_fops);
```

### DebugFS vs Other Interfaces

| Interface | Purpose | Permissions |
|-----------|---------|-------------|
| procfs | Legacy monitoring | User-readable |
| sysfs | Device attributes | Text-based config |
| debugfs | Kernel debugging | Admin-only, flexible |

## Complete Example: Comprehensive Debug Interface

```c
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>

#define SAMPLE_SIZE 256

struct debug_device {
    struct dentry *debugfs_dir;
    int debug_level;
    int samples[SAMPLE_SIZE];
    int sample_idx;
    unsigned long boot_time;
    struct mutex lock;
    unsigned long read_count;
    unsigned long write_count;
    unsigned long error_count;
};

static struct debug_device debug_dev = {
    .debug_level = 0,
    .sample_idx = 0,
    .read_count = 0,
    .write_count = 0,
    .error_count = 0,
};

// Simple u64 attribute
static int debug_level_get(void *data, u64 *val)
{
    *val = debug_dev.debug_level;
    return 0;
}

static int debug_level_set(void *data, u64 val)
{
    if (val > 3) return -EINVAL;
    debug_dev.debug_level = val;
    return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_level_fops, debug_level_get, debug_level_set, "%llu\n");

// Statistics read handler
static int stats_show(struct seq_file *m, void *v)
{
    mutex_lock(&debug_dev.lock);
    
    unsigned long uptime = jiffies - debug_dev.boot_time;
    
    seq_printf(m, "Driver Statistics\n");
    seq_printf(m, "=================\n");
    seq_printf(m, "Uptime:        %lu jiffies (%lu seconds)\n", uptime, uptime / HZ);
    seq_printf(m, "Read Ops:      %lu\n", debug_dev.read_count);
    seq_printf(m, "Write Ops:     %lu\n", debug_dev.write_count);
    seq_printf(m, "Errors:        %lu\n", debug_dev.error_count);
    
    if (debug_dev.read_count + debug_dev.write_count > 0) {
        int error_rate = (debug_dev.error_count * 100) / 
                        (debug_dev.read_count + debug_dev.write_count);
        seq_printf(m, "Error Rate:    %d%%\n", error_rate);
    }
    
    seq_printf(m, "Samples:       %d/%d\n", debug_dev.sample_idx, SAMPLE_SIZE);
    
    mutex_unlock(&debug_dev.lock);
    return 0;
}

static int stats_open(struct inode *inode, struct file *file)
{
    return single_open(file, stats_show, NULL);
}

static const struct file_operations stats_fops = {
    .open = stats_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

// Samples hexdump
static int samples_show(struct seq_file *m, void *v)
{
    int i;
    
    mutex_lock(&debug_dev.lock);
    
    seq_printf(m, "Samples (count=%d):\n", debug_dev.sample_idx);
    
    for (i = 0; i < debug_dev.sample_idx; i++) {
        if (i % 16 == 0) {
            seq_printf(m, "[%04d]: ", i);
        }
        
        seq_printf(m, "%04x ", debug_dev.samples[i]);
        
        if ((i + 1) % 16 == 0) {
            seq_printf(m, "\n");
        }
    }
    
    if (debug_dev.sample_idx % 16 != 0) {
        seq_printf(m, "\n");
    }
    
    mutex_unlock(&debug_dev.lock);
    return 0;
}

static int samples_open(struct inode *inode, struct file *file)
{
    return single_open(file, samples_show, NULL);
}

static const struct file_operations samples_fops = {
    .open = samples_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

// Module init/exit

static int __init debugfs_init(void)
{
    debug_dev.boot_time = jiffies;
    debug_dev.debugfs_dir = debugfs_create_dir("my_driver", NULL);
    
    if (!debug_dev.debugfs_dir) {
        printk(KERN_ERR "Failed to create debugfs directory\n");
        return -ENOMEM;
    }
    
    // Create debugfs files
    debugfs_create_file("debug_level", 0644, debug_dev.debugfs_dir, NULL, &debug_level_fops);
    debugfs_create_file("statistics", 0444, debug_dev.debugfs_dir, NULL, &stats_fops);
    debugfs_create_file("samples", 0444, debug_dev.debugfs_dir, NULL, &samples_fops);
    
    // Create boolean flags
    debugfs_create_bool("debug_enabled", 0644, debug_dev.debugfs_dir, &debug_dev.debug_level);
    
    // Create u64 values
    debugfs_create_u64("read_count", 0444, debug_dev.debugfs_dir, &debug_dev.read_count);
    debugfs_create_u64("write_count", 0444, debug_dev.debugfs_dir, &debug_dev.write_count);
    debugfs_create_u64("error_count", 0444, debug_dev.debugfs_dir, &debug_dev.error_count);
    
    mutex_init(&debug_dev.lock);
    
    printk(KERN_INFO "DebugFS interface created at /sys/kernel/debug/my_driver/\n");
    return 0;
}

static void __exit debugfs_exit(void)
{
    debugfs_remove_recursive(debug_dev.debugfs_dir);
    printk(KERN_INFO "DebugFS interface removed\n");
}

module_init(debugfs_init);
module_exit(debugfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Student");
MODULE_DESCRIPTION("DebugFS advanced debugging interface");
```

## Usage Examples

```bash
# Mount debugfs
sudo mount -t debugfs none /sys/kernel/debug

# View statistics
cat /sys/kernel/debug/my_driver/statistics

# Set debug level
echo 2 > /sys/kernel/debug/my_driver/debug_level

# View samples
cat /sys/kernel/debug/my_driver/samples

# Dynamic value changes
echo 1 > /sys/kernel/debug/my_driver/debug_enabled
```

## Key Concepts

| Feature | Purpose |
|---------|---------|
| DEFINE_SIMPLE_ATTRIBUTE | Getter/setter for u64 values |
| debugfs_create_file | Create custom file with fops |
| debugfs_create_bool | Boolean flag visualization |
| debugfs_remove_recursive | Clean up entire debugfs tree |
| seq_file | Paginated output for large data |

## Important Notes

1. **DebugFS is optional** - not present if CONFIG_DEBUG_FS not enabled
2. **No stability guarantees** - interfaces can change between kernel versions
3. **Admin-only access** - typically requires root (intentional security boundary)
4. **Performance** - safe for high-frequency instrumentation
5. **Testing aid** - ideal for development and troubleshooting, not production user interfaces
6. **Recursive cleanup** - debugfs_remove_recursive prevents memory leaks
