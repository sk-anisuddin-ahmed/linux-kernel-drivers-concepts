# 15 Poll and Select - Assignment Answers and Tasks

## Task 1: Theory Questions

1. **What is the purpose of poll() and select()?**
   They allow a process to monitor multiple file descriptors simultaneously and wait for events (readable, writable, error) on any of them. This enables efficient event-driven applications that handle multiple devices without blocking on a single one.

2. **What does poll_wait() do in the driver?**
   It registers the current process on a wait queue so the kernel can wake it when an event occurs. The driver doesn't actually sleep; `poll_wait()` just tells the kernel "if an event happens on this queue, wake up the waiting process."

3. **What's the difference between POLLIN and POLLRDNORM?**
   `POLLIN` is a generic mask indicating data is available to read, while `POLLRDNORM` is specific for normal (non-urgent) readable data. Use both together: `mask |= POLLIN | POLLRDNORM` for standard read operations.

4. **When does the kernel re-call a driver's poll() function?**
   After `wake_up_interruptible()` wakes the sleeping process, the kernel re-calls `poll()` on all registered devices to check their final status. This ensures the poll() return mask is accurate before returning to user-space.

5. **Can poll() sleep in the driver?**
   No, the driver's `poll()` function never sleeps; it only registers wait queues with `poll_wait()` and returns a mask immediately. The kernel handles the sleeping, not the driver.

## Task 2: Monitor Multiple Devices with Poll

Create two `/dev` files and a user program that uses poll() to wait for either one:

**Kernel Module (dual_poll.c):**

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

#define NUM_DEVICES 2
#define DRIVER_NAME "dual_poll"

static dev_t dev_num;
static struct cdev cdev;
static struct class *dev_class;
static wait_queue_head_t wq_array[NUM_DEVICES];
static int event_flag[NUM_DEVICES];

static unsigned int dual_poll(struct file *filp, struct poll_table_struct *wait)
{
    int minor = iminor(filp->f_inode);
    unsigned int mask = 0;

    poll_wait(filp, &wq_array[minor], wait);

    if (event_flag[minor]) {
        mask = POLLIN | POLLRDNORM;
    }

    return mask;
}

static ssize_t dual_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    int minor = iminor(filp->f_inode);
    char msg[50];
    int len;

    if (!event_flag[minor]) {
        return 0;
    }

    len = snprintf(msg, sizeof(msg), "Event from device %d\n", minor);
    if (copy_to_user(buf, msg, len))
        return -EFAULT;

    event_flag[minor] = 0;
    return len;
}

static ssize_t dual_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    int minor = iminor(filp->f_inode);

    event_flag[minor] = 1;
    wake_up_interruptible(&wq_array[minor]);
    pr_info("Device %d: event triggered\n", minor);

    return count;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = dual_read,
    .write = dual_write,
    .poll = dual_poll,
};

static int __init dual_poll_init(void)
{
    int i, ret;
    struct device *device;

    ret = alloc_chrdev_region(&dev_num, 0, NUM_DEVICES, DRIVER_NAME);
    if (ret < 0)
        return ret;

    cdev_init(&cdev, &fops);
    cdev.owner = THIS_MODULE;
    ret = cdev_add(&cdev, dev_num, NUM_DEVICES);
    if (ret < 0) {
        unregister_chrdev_region(dev_num, NUM_DEVICES);
        return ret;
    }

    dev_class = class_create(THIS_MODULE, DRIVER_NAME);
    if (IS_ERR(dev_class)) {
        cdev_del(&cdev);
        unregister_chrdev_region(dev_num, NUM_DEVICES);
        return PTR_ERR(dev_class);
    }

    for (i = 0; i < NUM_DEVICES; i++) {
        init_waitqueue_head(&wq_array[i]);
        event_flag[i] = 0;
        device = device_create(dev_class, NULL, MKDEV(MAJOR(dev_num), i),
                               NULL, "%s%d", DRIVER_NAME, i);
        if (IS_ERR(device)) {
            class_destroy(dev_class);
            cdev_del(&cdev);
            unregister_chrdev_region(dev_num, NUM_DEVICES);
            return PTR_ERR(device);
        }
    }

    pr_info("dual_poll: module loaded\n");
    return 0;
}

static void __exit dual_poll_exit(void)
{
    int i;

    for (i = 0; i < NUM_DEVICES; i++) {
        device_destroy(dev_class, MKDEV(MAJOR(dev_num), i));
    }

    class_destroy(dev_class);
    cdev_del(&cdev);
    unregister_chrdev_region(dev_num, NUM_DEVICES);
    pr_info("dual_poll: module unloaded\n");
}

module_init(dual_poll_init);
module_exit(dual_poll_exit);
```

## Task 3: User-Space Poll Application

Write a C program that uses poll() to wait for data on multiple devices.

**User-Space Poll Program (poll_test.c):**

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>

int main()
{
    int fd0, fd1;
    struct pollfd fds[2];
    char buf[50];
    int ret;

    fd0 = open("/dev/dual_poll0", O_RDWR);
    fd1 = open("/dev/dual_poll1", O_RDWR);

    if (fd0 < 0 || fd1 < 0) {
        perror("open");
        return -1;
    }

    fds[0].fd = fd0;
    fds[0].events = POLLIN;
    fds[1].fd = fd1;
    fds[1].events = POLLIN;

    printf("Waiting for events on both devices (write to /dev/dual_poll* to trigger)...\n");

    while (1) {
        ret = poll(fds, 2, 5000);

        if (ret < 0) {
            perror("poll");
            break;
        } else if (ret == 0) {
            printf("Timeout: no events\n");
        } else {
            if (fds[0].revents & POLLIN) {
                read(fd0, buf, sizeof(buf));
                printf("Device 0: %s", buf);
                fds[0].revents = 0;
            }
            if (fds[1].revents & POLLIN) {
                read(fd1, buf, sizeof(buf));
                printf("Device 1: %s", buf);
                fds[1].revents = 0;
            }
        }
    }

    close(fd0);
    close(fd1);
    return 0;
}
```

**Testing:**

```bash
# Terminal 1: Load module and run user program
sudo insmod dual_poll.ko
gcc -o poll_test poll_test.c
./poll_test

# Terminal 2: Trigger events
echo "test" > /dev/dual_poll0  # Triggers device 0
echo "test" > /dev/dual_poll1  # Triggers device 1
```

