# 13 Multiple Minor Devices - Assignment Answers and Tasks

## Task 1: Concept Questions

1. **How many devices can a single driver with one major number manage?**
   - Theoretically unlimited, but practically 0-255 minor numbers are common. Modern kernels support larger minor ranges, but 256 is the typical limit per driver. You could have multiple major numbers if needed.

2. **Why is it more efficient to use minor numbers instead of creating separate drivers?**
   - A single driver with one `file_operations` structure can handle all devices, eliminating code duplication and reducing memory overhead. Creating separate drivers for each device would waste memory and complicate management.

3. **How do you extract the minor number in the read/write handlers?**
   - Use `MINOR(inode->i_rdev)` where `inode = file_inode(file)`. This extracts the minor number from the device number stored in the inode.

4. **What happens if you access a minor number that doesn't exist?**
   - If your driver hasn't initialized state for that minor, you may read garbage or cause a crash. It's essential to validate `minor < MAX_DEVICES` and return `-ENODEV` for invalid minors.

5. **How can you efficiently store data for many devices?**
   - Use arrays indexed by minor number. For example: `static struct { int value; char buffer[256]; } devices[MAX_DEVICES];` allows O(1) lookups.

## Task 2: Add Per-Device Locks

Extend the driver to include a **per-device mutex** to prevent concurrent access to individual devices:

**Key Requirements:**

- Create an array of mutexes, one per device
- Lock in open(), unlock in release() (simple exclusive access model)
- OR: Lock in read/write to protect just the data access
- Handle EBUSY if device is already locked (optional)

**Sample Solution:**

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/mutex.h>

#define DEVICE_NAME "locked"
#define NUM_DEVICES 16

// Device state with per-device lock
static struct {
    struct mutex lock;
    int counter;
    char buffer[256];
    int is_open;
} devices[NUM_DEVICES];

static int major;
static struct class *myclass = NULL;

static int dev_open(struct inode *inode, struct file *file)
{
    int minor = MINOR(inode->i_rdev);

    if (minor >= NUM_DEVICES)
        return -ENODEV;

    // Attempt to acquire lock for exclusive access
    if (mutex_lock_interruptible(&devices[minor].lock))
        return -ERESTARTSYS;

    if (devices[minor].is_open) {
        mutex_unlock(&devices[minor].lock);
        return -EBUSY;  // Device already open
    }

    devices[minor].is_open = 1;
    mutex_unlock(&devices[minor].lock);

    printk(KERN_INFO "[locked%d] Opened\n", minor);
    file->private_data = (void *)(long)minor;
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    int minor = (int)(long)file->private_data;
    char msg[100];
    int bytes;

    if (minor >= NUM_DEVICES)
        return -ENODEV;

    // Acquire lock for this device
    if (mutex_lock_interruptible(&devices[minor].lock))
        return -ERESTARTSYS;

    bytes = snprintf(msg, sizeof(msg), 
                     "Device %d: counter=%d\n", 
                     minor, devices[minor].counter);

    mutex_unlock(&devices[minor].lock);

    return simple_read_from_buffer(buf, len, offset, msg, bytes);
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    int minor = (int)(long)file->private_data;
    char input[256];

    if (minor >= NUM_DEVICES)
        return -ENODEV;

    if (len > sizeof(input) - 1)
        len = sizeof(input) - 1;

    if (copy_from_user(input, buf, len))
        return -EFAULT;

    input[len] = '\0';

    // Acquire lock for this device
    if (mutex_lock_interruptible(&devices[minor].lock))
        return -ERESTARTSYS;

    strcpy(devices[minor].buffer, input);
    devices[minor].counter++;

    mutex_unlock(&devices[minor].lock);

    printk(KERN_INFO "[locked%d] Write: %s\n", minor, input);
    return len;
}

static int dev_release(struct inode *inode, struct file *file)
{
    int minor = MINOR(inode->i_rdev);

    if (minor >= NUM_DEVICES)
        return -ENODEV;

    mutex_lock(&devices[minor].lock);
    devices[minor].is_open = 0;
    mutex_unlock(&devices[minor].lock);

    printk(KERN_INFO "[locked%d] Closed\n", minor);
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

static int __init locked_init(void)
{
    int i;

    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        return major;
    }

    myclass = class_create(THIS_MODULE, "lockeddevs");
    if (IS_ERR(myclass)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(myclass);
    }

    // Initialize all devices with their mutexes
    for (i = 0; i < NUM_DEVICES; i++) {
        mutex_init(&devices[i].lock);
        devices[i].counter = 0;
        strcpy(devices[i].buffer, "");
        devices[i].is_open = 0;

        device_create(myclass, NULL, MKDEV(major, i), NULL,
                     "%s%d", DEVICE_NAME, i);
    }

    printk(KERN_INFO "Locked driver initialized\n");
    return 0;
}

static void __exit locked_exit(void)
{
    int i;

    for (i = 0; i < NUM_DEVICES; i++) {
        device_destroy(myclass, MKDEV(major, i));
        mutex_destroy(&devices[i].lock);
    }

    class_destroy(myclass);
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "Locked driver unloaded\n");
}

module_init(locked_init);
module_exit(locked_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Scalable driver with per-device locks");
```

## Task 3: Write Test Application

Create a user-space program that:

1. Opens multiple device files simultaneously (using threads or forks)
2. Writes to each device
3. Reads from each device
4. Demonstrates that each device maintains independent state

**Sample Solution:**

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

#define NUM_DEVICES 4
#define NUM_WRITES 5

typedef struct {
    int device_num;
    int fd;
} thread_args_t;

void* device_worker(void *arg)
{
    thread_args_t *args = (thread_args_t *)arg;
    int device = args->device_num;
    char device_path[64];
    char write_data[100];
    char read_data[256];
    int i;

    snprintf(device_path, sizeof(device_path), "/dev/locked%d", device);

    // Open device
    int fd = open(device_path, O_RDWR);
    if (fd < 0) {
        perror("open");
        return NULL;
    }

    printf("Thread %d: Opened %s\n", device, device_path);

    // Write multiple times
    for (i = 0; i < NUM_WRITES; i++) {
        snprintf(write_data, sizeof(write_data), "Thread %d write %d", device, i);
        if (write(fd, write_data, strlen(write_data)) < 0) {
            perror("write");
            close(fd);
            return NULL;
        }
        printf("Thread %d: Wrote '%s'\n", device, write_data);
        usleep(100000);  // 100ms delay
    }

    // Read back
    if (read(fd, read_data, sizeof(read_data)) < 0) {
        perror("read");
        close(fd);
        return NULL;
    }

    printf("Thread %d: Read: %s\n\n", device, read_data);

    close(fd);
    return NULL;
}

int main()
{
    pthread_t threads[NUM_DEVICES];
    thread_args_t args[NUM_DEVICES];
    int i;

    printf("=== Multi-Device Test ===\n\n");

    // Create threads, each accessing a different device
    for (i = 0; i < NUM_DEVICES; i++) {
        args[i].device_num = i;
        pthread_create(&threads[i], NULL, device_worker, &args[i]);
    }

    // Wait for all threads
    for (i = 0; i < NUM_DEVICES; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("=== Test Completed ===\n");

    return 0;
}
```

**Compile and run:**

```bash
gcc -pthread -o test_multi test_multi.c
sudo ./test_multi
```

**Expected output:**

```
=== Multi-Device Test ===

Thread 0: Opened /dev/locked0
Thread 1: Opened /dev/locked1
Thread 2: Opened /dev/locked2
Thread 3: Opened /dev/locked3
Thread 0: Wrote 'Thread 0 write 0'
Thread 1: Wrote 'Thread 1 write 0'
Thread 2: Wrote 'Thread 2 write 0'
Thread 3: Wrote 'Thread 3 write 0'
...
Thread 0: Read: Device 0: counter=5
Thread 1: Read: Device 1: counter=5
Thread 2: Read: Device 2: counter=5
Thread 3: Read: Device 3: counter=5

=== Test Completed ===
```

This demonstrates independent device state and thread-safe access.
