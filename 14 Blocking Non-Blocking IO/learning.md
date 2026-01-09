# 14 Blocking I/O – Wait Queues and Sleep

## Overview

So far, all your drivers use **non-blocking I/O**: `read()` returns immediately with available data, or returns 0/error.

But what if there's **no data available**? Real drivers (UART, sensors, etc.) often need to **block the calling process** until data arrives.

Linux uses **wait queues** to implement this. A process sleeps until a condition is met (e.g., "data arrived").

By the end of this section, you will:

- Understand wait queues and sleep mechanisms
- Implement blocking read in your driver
- Use `wake_up()` to signal waiting processes
- Handle signals interrupting sleep

## Wait Queues Concept

A **wait queue** is a kernel data structure that holds processes sleeping for a particular event:

```c
#include <linux/wait.h>

// Declare a wait queue
static DECLARE_WAIT_QUEUE_HEAD(data_queue);

// Producer: Generate data and wake up waiters
void produce_data(void) {
    // ... update shared data ...
    wake_up(&data_queue);  // Wake all waiters
}

// Consumer: Wait for data
ssize_t dev_read(...) {
    // If no data, sleep on the queue
    wait_event_interruptible(data_queue, has_data());
    
    // ... read the data ...
    return bytes_read;
}
```

## Key Wait Queue Functions

### wait_event_interruptible()

```c
int ret = wait_event_interruptible(wait_queue, condition);
```

- Sleeps until `condition` becomes true
- Wakes if a signal arrives (e.g., Ctrl+C)
- Returns 0 if condition met, -ERESTARTSYS if signaled
- Use `-ERESTARTSYS` to restart the syscall

### wake_up()

```c
wake_up(&wait_queue);
```

- Wakes ALL processes sleeping on the queue
- Safe to call from interrupt handlers
- Processes check their condition and proceed if met

### wake_up_interruptible()

```c
wake_up_interruptible(&wait_queue);
```

- Wakes only interruptible sleepers (ignores TASK_UNINTERRUPTIBLE)

## Step-by-Step: Blocking I/O Driver

### 1. Create the Driver

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/sched.h>

#define DEVICE_NAME "blocking"
#define BUFFER_SIZE 256

static int major;
static struct class *myclass = NULL;
static struct device *mydevice = NULL;

// Shared data
static char device_buffer[BUFFER_SIZE];
static int data_available = 0;

// Wait queue for readers
static DECLARE_WAIT_QUEUE_HEAD(read_queue);

static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[%s] Device opened\n", DEVICE_NAME);
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    int ret;

    // Check if data is available, if not sleep
    ret = wait_event_interruptible(read_queue, data_available);
    if (ret == -ERESTARTSYS) {
        printk(KERN_INFO "[%s] Read interrupted by signal\n", DEVICE_NAME);
        return -ERESTARTSYS;
    }

    // Data is now available
    if (len > BUFFER_SIZE)
        len = BUFFER_SIZE;

    if (copy_to_user(buf, device_buffer, len))
        return -EFAULT;

    // Reset flag for next read
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

    // Set flag and wake up readers
    data_available = 1;
    wake_up_interruptible(&read_queue);

    printk(KERN_INFO "[%s] Data written and readers woken (%zu bytes)\n", 
           DEVICE_NAME, len);
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

static int __init blocking_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        return major;
    }

    myclass = class_create(THIS_MODULE, "blockingdevs");
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

    data_available = 0;
    printk(KERN_INFO "Blocking I/O driver loaded\n");
    return 0;
}

static void __exit blocking_exit(void)
{
    device_destroy(myclass, MKDEV(major, 0));
    class_destroy(myclass);
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "Blocking I/O driver unloaded\n");
}

module_init(blocking_init);
module_exit(blocking_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Blocking I/O with wait queues");
```

### 2. Test Script

```bash
#!/bin/bash

# Terminal 1: Read (blocks until data available)
# cat /dev/blocking

# Terminal 2: Write (wakes reader)
# echo "Hello!" > /dev/blocking
```

## Wait Queue Variants

| Function | Behavior |
|----------|----------|
| `wait_event(q, cond)` | Sleep, uninterruptible |
| `wait_event_interruptible(q, cond)` | Sleep, can be signaled |
| `wait_event_timeout(q, cond, timeout)` | Sleep with timeout (milliseconds) |
| `wait_event_interruptible_timeout(q, cond, timeout)` | Both signal and timeout |

## Key Concepts

### DECLARE_WAIT_QUEUE_HEAD

```c
static DECLARE_WAIT_QUEUE_HEAD(my_queue);
```

Declares and initializes a wait queue at compile time.

### Data Condition

Always check a condition variable:

```c
while (!condition) {
    wait_event_interruptible(queue, condition);
}
```

### Signal Handling

Return `-ERESTARTSYS` when signaled to allow proper cleanup:

```c
if (wait_event_interruptible(queue, condition) == -ERESTARTSYS)
    return -ERESTARTSYS;
```

## Important Notes

1. **Always check return value**: `wait_event_interruptible()` returns -ERESTARTSYS if signaled
2. **Condition must be checkable**: The condition is re-evaluated after waking
3. **Multiple waiters**: `wake_up()` wakes all, they all check the condition
4. **No race conditions**: Wait queue handles kernel scheduling safely
5. **Use proper types**: Conditions should be `int` or `bool`, not pointers
6. **Avoid busy-waiting**: Never use `while(condition) cpu_relax()` with sleeps
