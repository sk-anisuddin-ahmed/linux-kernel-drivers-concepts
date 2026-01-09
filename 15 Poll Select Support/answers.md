# 15 Poll and Select - Assignment Answers and Tasks

## Task 1: Theory Questions

1. **What is the purpose of poll() and select()?**
   - They allow a single process to monitor multiple file descriptors and block until one or more becomes ready for I/O. Applications can respond to whichever device has an event first.

2. **What does poll_wait() do in the driver?**
   - It registers the driver's wait queue with the kernel's polling mechanism so that when an event occurs on that queue, the kernel knows to wake the poll and re-check which devices are ready.

3. **What's the difference between POLLIN and POLLRDNORM?**
   - POLLIN indicates normal data available. POLLRDNORM is its normalized equivalent. Typically both are returned together to indicate readable data.

4. **When does the kernel re-call a driver's poll() function?**
   - After an event occurs on one of the monitored wait queues. The kernel re-polls all devices to build a final set of ready file descriptors.

5. **Can poll() sleep in the driver?**
   - No, poll() should never sleep or block. It should check conditions and return immediately. The kernel handles sleeping and waiting.

## Task 2: Monitor Multiple Devices with Poll

Create two `/dev` files and a user program that uses poll() to wait for either one:

**Sample Solution:**

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/poll.h>

#define DEVICE_NAME "dualpoll"
#define NUM_DEVICES 2
#define BUFFER_SIZE 256

static int major;
static struct class *myclass = NULL;
static struct device *mydevices[NUM_DEVICES];

static struct {
    char buffer[BUFFER_SIZE];
    int data_available;
    struct wait_queue_head read_queue;
} devices[NUM_DEVICES];

static unsigned int dev_poll(struct file *file, struct poll_table_struct *wait)
{
    int minor = MINOR(file_inode(file)->i_rdev);
    unsigned int mask = 0;

    if (minor >= NUM_DEVICES)
        return POLLNVAL;

    poll_wait(file, &devices[minor].read_queue, wait);

    if (devices[minor].data_available)
        mask |= POLLIN | POLLRDNORM;

    mask |= POLLOUT | POLLWRNORM;

    return mask;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    int minor = MINOR(file_inode(file)->i_rdev);

    if (minor >= NUM_DEVICES)
        return -ENODEV;

    wait_event_interruptible(devices[minor].read_queue, devices[minor].data_available);

    if (len > BUFFER_SIZE)
        len = BUFFER_SIZE;

    copy_to_user(buf, devices[minor].buffer, len);
    devices[minor].data_available = 0;

    printk(KERN_INFO "[dualpoll%d] Read\n", minor);
    return len;
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    int minor = MINOR(file_inode(file)->i_rdev);

    if (minor >= NUM_DEVICES)
        return -ENODEV;

    if (len > BUFFER_SIZE - 1)
        len = BUFFER_SIZE - 1;

    copy_from_user(devices[minor].buffer, buf, len);
    devices[minor].buffer[len] = '\0';

    devices[minor].data_available = 1;
    wake_up_interruptible(&devices[minor].read_queue);

    printk(KERN_INFO "[dualpoll%d] Data ready\n", minor);
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

static int __init dualpoll_init(void)
{
    int i;

    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0)
        return major;

    myclass = class_create(THIS_MODULE, "dualpolldevices");
    if (IS_ERR(myclass)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(myclass);
    }

    for (i = 0; i < NUM_DEVICES; i++) {
        init_waitqueue_head(&devices[i].read_queue);
        devices[i].data_available = 0;
        mydevices[i] = device_create(myclass, NULL, MKDEV(major, i), NULL,
                                    "%s%d", DEVICE_NAME, i);
    }

    printk(KERN_INFO "Dual poll driver loaded\n");
    return 0;
}

static void __exit dualpoll_exit(void)
{
    int i;

    for (i = 0; i < NUM_DEVICES; i++)
        device_destroy(myclass, MKDEV(major, i));

    class_destroy(myclass);
    unregister_chrdev(major, DEVICE_NAME);
}

module_init(dualpoll_init);
module_exit(dualpoll_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Multiple device poll support");
```

## Task 3: User-Space Poll Application

Write a C program that uses poll() to wait for data on multiple devices:

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>

int main()
{
    int fd0, fd1;
    struct pollfd fds[2];
    char buffer[256];
    int ret;

    // Open both devices
    fd0 = open("/dev/dualpoll0", O_RDWR);
    fd1 = open("/dev/dualpoll1", O_RDWR);

    if (fd0 < 0 || fd1 < 0) {
        perror("open");
        return 1;
    }

    // Setup poll file descriptors
    fds[0].fd = fd0;
    fds[0].events = POLLIN;

    fds[1].fd = fd1;
    fds[1].events = POLLIN;

    printf("Waiting for events on both devices...\n");
    printf("(Write to /dev/dualpoll0 or /dev/dualpoll1 from another terminal)\n\n");

    while (1) {
        // Poll with 10 second timeout
        ret = poll(fds, 2, 10000);

        if (ret < 0) {
            perror("poll");
            break;
        } else if (ret == 0) {
            printf("Poll timeout\n");
            continue;
        }

        // Check which device is readable
        if (fds[0].revents & POLLIN) {
            printf("Device 0 has data:\n");
            read(fd0, buffer, sizeof(buffer) - 1);
            printf("  %s\n", buffer);
            fds[0].revents = 0;
        }

        if (fds[1].revents & POLLIN) {
            printf("Device 1 has data:\n");
            read(fd1, buffer, sizeof(buffer) - 1);
            printf("  %s\n", buffer);
            fds[1].revents = 0;
        }
    }

    close(fd0);
    close(fd1);
    return 0;
}
```

**Test it:**

```bash
# Terminal 1: Run poll app
gcc -o poll_test poll_test.c
./poll_test

# Terminal 2 and 3: Write to different devices
echo "data from 0" > /dev/dualpoll0
echo "data from 1" > /dev/dualpoll1

# Poll app shows which device had data first
```
