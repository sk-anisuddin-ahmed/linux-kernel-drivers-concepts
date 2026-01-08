# Section 24: Device File Operations – Advanced Read/Write

## Learning Outcomes

- Implement advanced file operations (seek, fcntl)
- Support non-blocking and asynchronous I/O
- Implement fasync for signal-based notifications
- Handle concurrent access with proper locking
- Optimize for performance-critical applications

## File Operations Structure

```c
static const struct file_operations dev_fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
    .llseek = dev_llseek,
    .unlocked_ioctl = dev_ioctl,
    .fasync = dev_fasync,
    .poll = dev_poll,
};
```

## Complete Example

```c
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/wait.h>

#define DEVICE_NAME "advanced_dev"
#define BUFFER_SIZE 1024

struct advanced_device {
    struct cdev cdev;
    char buffer[BUFFER_SIZE];
    size_t buffer_len;
    struct mutex lock;
    wait_queue_head_t read_queue;
    struct fasync_struct *async_queue;
    struct device *device;
    dev_t dev_num;
};

static struct advanced_device adv_dev;

static loff_t dev_llseek(struct file *file, loff_t offset, int whence)
{
    loff_t new_pos;
    
    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = file->f_pos + offset;
            break;
        case SEEK_END:
            new_pos = adv_dev.buffer_len + offset;
            break;
        default:
            return -EINVAL;
    }
    
    if (new_pos < 0 || new_pos > BUFFER_SIZE) {
        return -EINVAL;
    }
    
    file->f_pos = new_pos;
    return new_pos;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    size_t to_read;
    
    if (mutex_lock_interruptible(&adv_dev.lock)) {
        return -ERESTARTSYS;
    }
    
    if (*ppos >= adv_dev.buffer_len) {
        mutex_unlock(&adv_dev.lock);
        return 0;
    }
    
    to_read = min(count, adv_dev.buffer_len - (size_t)*ppos);
    
    if (copy_to_user(buf, adv_dev.buffer + *ppos, to_read)) {
        mutex_unlock(&adv_dev.lock);
        return -EFAULT;
    }
    
    *ppos += to_read;
    mutex_unlock(&adv_dev.lock);
    
    return to_read;
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    size_t to_write;
    
    if (mutex_lock_interruptible(&adv_dev.lock)) {
        return -ERESTARTSYS;
    }
    
    if (*ppos >= BUFFER_SIZE) {
        mutex_unlock(&adv_dev.lock);
        return -ENOSPC;
    }
    
    to_write = min(count, BUFFER_SIZE - (size_t)*ppos);
    
    if (copy_from_user(adv_dev.buffer + *ppos, buf, to_write)) {
        mutex_unlock(&adv_dev.lock);
        return -EFAULT;
    }
    
    *ppos += to_write;
    if (*ppos > adv_dev.buffer_len) {
        adv_dev.buffer_len = *ppos;
    }
    
    // Signal async waiters
    kill_fasync(&adv_dev.async_queue, SIGIO, POLL_IN);
    wake_up(&adv_dev.read_queue);
    
    mutex_unlock(&adv_dev.lock);
    
    return to_write;
}

static int dev_fasync(int fd, struct file *file, int on)
{
    return fasync_helper(fd, file, on, &adv_dev.async_queue);
}

static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device opened\n");
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    dev_fasync(-1, file, 0);
    printk(KERN_INFO "Device closed\n");
    return 0;
}

static int __init advanced_init(void)
{
    struct class *dev_class;
    int ret;
    
    alloc_chrdev_region(&adv_dev.dev_num, 0, 1, DEVICE_NAME);
    
    cdev_init(&adv_dev.cdev, &dev_fops);
    adv_dev.cdev.owner = THIS_MODULE;
    cdev_add(&adv_dev.cdev, adv_dev.dev_num, 1);
    
    dev_class = class_create(THIS_MODULE, DEVICE_NAME);
    adv_dev.device = device_create(dev_class, NULL, adv_dev.dev_num, NULL, DEVICE_NAME);
    
    mutex_init(&adv_dev.lock);
    init_waitqueue_head(&adv_dev.read_queue);
    
    printk(KERN_INFO "Advanced device driver loaded\n");
    return 0;
}

static void __exit advanced_exit(void)
{
    cdev_del(&adv_dev.cdev);
    unregister_chrdev_region(adv_dev.dev_num, 1);
    printk(KERN_INFO "Advanced device driver unloaded\n");
}

module_init(advanced_init);
module_exit(advanced_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Advanced file operations example");

static const struct file_operations dev_fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
    .llseek = dev_llseek,
    .fasync = dev_fasync,
};
```

## Key Concepts

| Operation | Purpose |
|-----------|---------|
| llseek | Position file pointer (SEEK_SET/CUR/END) |
| fasync | Enable async signal notification |
| kill_fasync | Send SIGIO to async waiters |
| interruptible locks | Allow Ctrl+C to interrupt |

## Important Notes

1. **Seek bounds checking** critical to prevent buffer overflow
2. **copy_to_user/copy_from_user** required for user-kernel data transfer
3. **Interruptible locks** improve user experience (Ctrl+C responsive)
4. **kill_fasync** signals are optional but powerful for event-driven code
5. **Lock ordering** matters when combining multiple synchronization primitives
6. **File position** automatically incremented for sequential access
