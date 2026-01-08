# 15 Poll and Select Support

## Overview

Blocking I/O puts the process to sleep until data arrives. But what if an application needs to monitor **multiple devices simultaneously** and respond to whichever has data first?

Enter **poll()** and **select()** — system calls that let a process wait for events on multiple file descriptors at once.

To support them, your driver implements the `.poll` handler in `file_operations`.

By the end of this section, you will:

- Understand poll masks and events
- Implement the `.poll` file operation
- Use poll to monitor multiple devices
- Understand how poll/select work together

## Poll Masks

Poll works with **event masks** — bitmasks indicating which events occurred:

| Mask | Meaning |
|------|---------|
| `POLLIN` | Data available to read |
| `POLLOUT` | Device ready for write |
| `POLLERR` | Error condition |
| `POLLHUP` | Hung up (connection closed) |
| `POLLNVAL` | Invalid file descriptor |

## Implementing Poll

Add a `.poll` handler to your `file_operations`:

```c
static unsigned int dev_poll(struct file *file, struct poll_table_struct *wait)
{
    unsigned int mask = 0;

    // Register wait queue for this file
    poll_wait(file, &my_wait_queue, wait);

    // Check conditions and return appropriate mask
    if (data_available)
        mask |= POLLIN;  // Readable

    if (buffer_has_space)
        mask |= POLLOUT;  // Writable

    return mask;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .poll = dev_poll,
    .release = dev_release,
};
```

## How Poll Works

1. Application calls `poll()` with multiple file descriptors
2. Kernel calls `.poll()` on each device
3. Each device's poll registers itself on its wait queue
4. Kernel sleeps until an event occurs
5. When event occurs, `wake_up()` wakes the poll
6. Kernel re-calls poll on all devices to get final mask
7. Poll returns to application with event masks

## Step-by-Step: Poll-Enabled Driver

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/poll.h>

#define DEVICE_NAME "polldev"

static int major;
static struct class *myclass = NULL;
static struct device *mydevice = NULL;
static char device_buffer[256];
static int data_available = 0;

static DECLARE_WAIT_QUEUE_HEAD(read_queue);
static DECLARE_WAIT_QUEUE_HEAD(write_queue);

static unsigned int dev_poll(struct file *file, struct poll_table_struct *wait)
{
    unsigned int mask = 0;

    // Register on both queues
    poll_wait(file, &read_queue, wait);
    poll_wait(file, &write_queue, wait);

    // Check read condition
    if (data_available)
        mask |= POLLIN | POLLRDNORM;

    // Check write condition (always writable for simplicity)
    mask |= POLLOUT | POLLWRNORM;

    return mask;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    wait_event_interruptible(read_queue, data_available);

    if (len > 256)
        len = 256;

    copy_to_user(buf, device_buffer, len);
    data_available = 0;

    return len;
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    if (len > 255)
        len = 255;

    copy_from_user(device_buffer, buf, len);
    device_buffer[len] = '\0';

    data_available = 1;
    wake_up_interruptible(&read_queue);

    return len;
}

static int dev_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .poll = dev_poll,
    .release = dev_release,
};

static int __init polldev_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0)
        return major;

    myclass = class_create(THIS_MODULE, "polldevices");
    if (IS_ERR(myclass)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(myclass);
    }

    mydevice = device_create(myclass, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(mydevice)) {
        class_destroy(myclass);
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(mydevice);
    }

    printk(KERN_INFO "Poll device driver loaded\n");
    return 0;
}

static void __exit polldev_exit(void)
{
    device_destroy(myclass, MKDEV(major, 0));
    class_destroy(myclass);
    unregister_chrdev(major, DEVICE_NAME);
}

module_init(polldev_init);
module_exit(polldev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Poll/select support");
```

## Important Notes

1. **Always call poll_wait()**: Register your wait queues even if events are ready
2. **Return appropriate masks**: Include both generic (POLLIN) and specific (POLLRDNORM) masks
3. **Combine with blocking I/O**: Use poll + blocking read/write together
4. **Poll doesn't wait in driver**: It only registers queues; kernel does the waiting
5. **Signal handling**: If blocked in poll, wake it with a signal
6. **Edge vs Level triggered**: Mask tells kernel about current state (level), not changes (edge)
