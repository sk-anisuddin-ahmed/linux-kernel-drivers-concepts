# Section 24: Device File Operations – Assignment

## Task 1: Theory Questions

1. **Seek Operations**: llseek implements SEEK_SET (absolute), SEEK_CUR (relative), SEEK_END (from end). Must validate bounds to prevent buffer overflow.

2. **Fasync Handler**: fasync_helper manages async queue; kill_fasync sends SIGIO signal to all waiting processes when data available.

3. **Interruptible Locks**: mutex_lock_interruptible() allows Ctrl+C to interrupt; returns -ERESTARTSYS to userspace which retries automatically.

4. **Concurrent Access**: Multiple readers/writers need serialization. Mutex protects buffer consistency; read_queue for blocked readers.

5. **Copy Functions**: copy_to_user/copy_from_user safely transfer data between kernel and userspace, returning count of bytes NOT copied.

## Task 2: Seekable Buffer Device

Implement character device supporting seek and positional read/write:

```c
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define BUFFER_SIZE 4096

struct buffer_device {
    struct cdev cdev;
    char buffer[BUFFER_SIZE];
    size_t buffer_len;
    struct mutex lock;
    dev_t dev_num;
};

static struct buffer_device buf_dev;

static loff_t buf_llseek(struct file *file, loff_t offset, int whence)
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
            new_pos = buf_dev.buffer_len + offset;
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

static ssize_t buf_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    size_t to_read;
    
    if (mutex_lock_interruptible(&buf_dev.lock)) {
        return -ERESTARTSYS;
    }
    
    if (*ppos >= buf_dev.buffer_len) {
        mutex_unlock(&buf_dev.lock);
        return 0;
    }
    
    to_read = min(count, buf_dev.buffer_len - (size_t)*ppos);
    
    if (copy_to_user(buf, buf_dev.buffer + *ppos, to_read)) {
        mutex_unlock(&buf_dev.lock);
        return -EFAULT;
    }
    
    *ppos += to_read;
    mutex_unlock(&buf_dev.lock);
    
    return to_read;
}

static ssize_t buf_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    size_t to_write;
    
    if (mutex_lock_interruptible(&buf_dev.lock)) {
        return -ERESTARTSYS;
    }
    
    if (*ppos >= BUFFER_SIZE) {
        mutex_unlock(&buf_dev.lock);
        return -ENOSPC;
    }
    
    to_write = min(count, BUFFER_SIZE - (size_t)*ppos);
    
    if (copy_from_user(buf_dev.buffer + *ppos, buf, to_write)) {
        mutex_unlock(&buf_dev.lock);
        return -EFAULT;
    }
    
    *ppos += to_write;
    if (*ppos > buf_dev.buffer_len) {
        buf_dev.buffer_len = *ppos;
    }
    
    mutex_unlock(&buf_dev.lock);
    
    return to_write;
}

static const struct file_operations buf_fops = {
    .owner = THIS_MODULE,
    .read = buf_read,
    .write = buf_write,
    .llseek = buf_llseek,
};

static int __init buffer_init(void)
{
    alloc_chrdev_region(&buf_dev.dev_num, 0, 1, "buffer_dev");
    cdev_init(&buf_dev.cdev, &buf_fops);
    cdev_add(&buf_dev.cdev, buf_dev.dev_num, 1);
    
    mutex_init(&buf_dev.lock);
    
    printk(KERN_INFO "Buffer device loaded\n");
    return 0;
}

static void __exit buffer_exit(void)
{
    cdev_del(&buf_dev.cdev);
    unregister_chrdev_region(buf_dev.dev_num, 1);
    printk(KERN_INFO "Buffer device unloaded\n");
}

module_init(buffer_init);
module_exit(buffer_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Seekable buffer device");
```

**Usage**: Write data, then seek to different positions and read back.

## Task 3: Async Notification Driver

Implement driver supporting fasync for signal-based I/O notification:

```c
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

struct async_device {
    struct cdev cdev;
    char buffer[256];
    size_t buffer_len;
    struct fasync_struct *async_queue;
    struct mutex lock;
    dev_t dev_num;
};

static struct async_device async_dev;

static ssize_t async_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    size_t to_read;
    
    if (mutex_lock_interruptible(&async_dev.lock)) {
        return -ERESTARTSYS;
    }
    
    to_read = min(count, async_dev.buffer_len);
    
    if (copy_to_user(buf, async_dev.buffer, to_read)) {
        mutex_unlock(&async_dev.lock);
        return -EFAULT;
    }
    
    async_dev.buffer_len = 0;
    mutex_unlock(&async_dev.lock);
    
    return to_read;
}

static ssize_t async_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    size_t to_write;
    
    if (mutex_lock_interruptible(&async_dev.lock)) {
        return -ERESTARTSYS;
    }
    
    to_write = min(count, sizeof(async_dev.buffer) - 1);
    
    if (copy_from_user(async_dev.buffer, buf, to_write)) {
        mutex_unlock(&async_dev.lock);
        return -EFAULT;
    }
    
    async_dev.buffer_len = to_write;
    
    // Signal async readers
    kill_fasync(&async_dev.async_queue, SIGIO, POLL_IN);
    
    mutex_unlock(&async_dev.lock);
    
    return to_write;
}

static int async_fasync(int fd, struct file *file, int on)
{
    return fasync_helper(fd, file, on, &async_dev.async_queue);
}

static const struct file_operations async_fops = {
    .owner = THIS_MODULE,
    .read = async_read,
    .write = async_write,
    .fasync = async_fasync,
};

static int __init async_init(void)
{
    alloc_chrdev_region(&async_dev.dev_num, 0, 1, "async_dev");
    cdev_init(&async_dev.cdev, &async_fops);
    cdev_add(&async_dev.cdev, async_dev.dev_num, 1);
    
    mutex_init(&async_dev.lock);
    
    printk(KERN_INFO "Async device loaded\n");
    return 0;
}

static void __exit async_exit(void)
{
    cdev_del(&async_dev.cdev);
    unregister_chrdev_region(async_dev.dev_num, 1);
    printk(KERN_INFO "Async device unloaded\n");
}

module_init(async_init);
module_exit(async_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Async notification device");
```

**Usage**: Enable fasync with fcntl(fd, F_SETFL, O_ASYNC), then receive SIGIO signals on write.
