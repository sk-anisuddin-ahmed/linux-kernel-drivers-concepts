# 14 Blocking I/O - Assignment Answers and Tasks

## Task 1: Understand Wait Queues

1. **What is a wait queue and why is it needed?**
   - A wait queue is a kernel data structure that holds processes (in sleeping state) waiting for an event. It's needed to allow processes to sleep efficiently without consuming CPU, so they can be woken when data becomes available.

2. **What does wait_event_interruptible() do?**
   - It puts the calling process to sleep until a condition becomes true. If a signal arrives (like Ctrl+C), it returns -ERESTARTSYS, allowing the process to handle the signal before returning from the syscall.

3. **What's the difference between wake_up() and wake_up_interruptible()?**
   - `wake_up()` wakes all sleepers including uninterruptible ones. `wake_up_interruptible()` only wakes processes that are marked as interruptible (safe for signal handling).

4. **Why must you check the return value of wait_event_interruptible()?**
   - Because it can return -ERESTARTSYS if interrupted by a signal, indicating the syscall should be restarted or the error should be propagated to user space.

5. **What would happen if you called wake_up() twice?**
   - The first wake_up() wakes all sleeping processes. The second wake_up() does nothing (there are no more processes sleeping) and returns without error. This is safe.

## Task 2: Add Timeout Support

Extend the driver to use `wait_event_interruptible_timeout()` so reads timeout after 5 seconds:

**Key Requirements:**

- Use `wait_event_interruptible_timeout()` instead of `wait_event_interruptible()`
- Return 0 (EOF) on timeout
- Handle both signal and timeout returns properly

**Sample Solution:**

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/jiffies.h>

#define DEVICE_NAME "timeout"
#define BUFFER_SIZE 256
#define READ_TIMEOUT_MS 5000  // 5 seconds

static int major;
static struct class *myclass = NULL;
static char device_buffer[BUFFER_SIZE];
static int data_available = 0;

static DECLARE_WAIT_QUEUE_HEAD(read_queue);

static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[%s] Device opened\n", DEVICE_NAME);
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    unsigned long timeout_jiffies = msecs_to_jiffies(READ_TIMEOUT_MS);
    long ret;

    // Wait with timeout
    ret = wait_event_interruptible_timeout(read_queue, data_available, timeout_jiffies);
    
    if (ret == -ERESTARTSYS) {
        printk(KERN_INFO "[%s] Read interrupted by signal\n", DEVICE_NAME);
        return -ERESTARTSYS;
    } else if (ret == 0) {
        printk(KERN_INFO "[%s] Read timed out after %d ms\n", DEVICE_NAME, READ_TIMEOUT_MS);
        return 0;  // Return EOF on timeout
    }

    // Data is available
    if (len > BUFFER_SIZE)
        len = BUFFER_SIZE;

    if (copy_to_user(buf, device_buffer, len))
        return -EFAULT;

    data_available = 0;
    printk(KERN_INFO "[%s] Data read (%zu bytes)\n", DEVICE_NAME, len);
    return len;
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    if (len > BUFFER_SIZE - 1)
        len = BUFFER_SIZE - 1;

    if (copy_from_user(device_buffer, buf, len))
        return -EFAULT;

    device_buffer[len] = '\0';

    data_available = 1;
    wake_up_interruptible(&read_queue);

    printk(KERN_INFO "[%s] Data written\n", DEVICE_NAME);
    return len;
}

static int dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[%s] Device closed\n", DEVICE_NAME);
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

static int __init timeout_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0)
        return major;

    myclass = class_create(THIS_MODULE, "timeoutdevs");
    if (IS_ERR(myclass)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(myclass);
    }

    device_create(myclass, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    printk(KERN_INFO "Timeout driver loaded (5 sec timeout)\n");
    return 0;
}

static void __exit timeout_exit(void)
{
    device_destroy(myclass, MKDEV(major, 0));
    class_destroy(myclass);
    unregister_chrdev(major, DEVICE_NAME);
}

module_init(timeout_init);
module_exit(timeout_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Blocking I/O with timeout");
```

**Test it:**

```bash
sudo insmod timeout.ko
cat /dev/timeout
# Waits 5 seconds, then returns empty (EOF)

# In another terminal:
echo "data" > /dev/timeout
# Cat immediately returns with data
```

## Task 3: Multiple Readers on Same Wait Queue

Create a driver that:

1. Allows multiple processes to sleep on the same wait queue
2. When data arrives, all readers wake up
3. Only the first reader gets the data, others go back to sleep

**Sample Solution:**

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/mutex.h>

#define DEVICE_NAME "multiread"
#define BUFFER_SIZE 256

static int major;
static struct class *myclass = NULL;
static char device_buffer[BUFFER_SIZE];
static int data_available = 0;
static struct mutex read_lock;

static DECLARE_WAIT_QUEUE_HEAD(read_queue);

static int dev_open(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    // Loop until we get data (others might consume it before us)
    while (1) {
        // Wait for data availability
        if (wait_event_interruptible(read_queue, data_available))
            return -ERESTARTSYS;

        // Try to get the lock and data
        if (mutex_lock_interruptible(&read_lock))
            return -ERESTARTSYS;

        if (data_available) {
            // We got it!
            if (len > BUFFER_SIZE)
                len = BUFFER_SIZE;

            if (copy_to_user(buf, device_buffer, len)) {
                mutex_unlock(&read_lock);
                return -EFAULT;
            }

            data_available = 0;
            mutex_unlock(&read_lock);
            return len;
        }

        // Another reader got it first, go back to sleep
        mutex_unlock(&read_lock);
        printk(KERN_INFO "Lost race, waiting again...\n");
    }
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    if (len > BUFFER_SIZE - 1)
        len = BUFFER_SIZE - 1;

    if (copy_from_user(device_buffer, buf, len))
        return -EFAULT;

    device_buffer[len] = '\0';

    mutex_lock(&read_lock);
    data_available = 1;
    mutex_unlock(&read_lock);

    // Wake all readers - they'll compete for data
    wake_up_interruptible(&read_queue);

    printk(KERN_INFO "Data written, %lu readers woken\n", read_queue.nr_exclusive);
    return len;
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
    .release = dev_release,
};

static int __init multiread_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0)
        return major;

    myclass = class_create(THIS_MODULE, "multireaddevs");
    if (IS_ERR(myclass)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(myclass);
    }

    device_create(myclass, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    mutex_init(&read_lock);
    printk(KERN_INFO "Multi-reader driver loaded\n");
    return 0;
}

static void __exit multiread_exit(void)
{
    mutex_destroy(&read_lock);
    device_destroy(myclass, MKDEV(major, 0));
    class_destroy(myclass);
    unregister_chrdev(major, DEVICE_NAME);
}

module_init(multiread_init);
module_exit(multiread_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Multiple readers on same wait queue");
```

**Test with multiple terminals:**

```bash
# Terminal 1, 2, 3: All read and block
cat /dev/multiread
cat /dev/multiread
cat /dev/multiread

# Terminal 4: Write once
echo "message" > /dev/multiread
# Only one cat succeeds, others wait for more data
```
