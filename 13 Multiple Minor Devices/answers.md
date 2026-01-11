# 13 Multiple Minor Devices - Assignment Answers and Tasks

## Task 1: Concept Questions

1. **How many devices can a single driver with one major number manage?**
   - A single major number can manage up to 256 minor numbers (0-255) by default. Each minor number represents a separate device instance.

2. **Why is it more efficient to use minor numbers instead of creating separate drivers?**
   - A single driver codebase handles all devices, saving memory and maintenance effort. Without minor numbers, each device would require its own driver, driver module, and registration overhead.

3. **How do you extract the minor number in the read/write handlers?**
   - Use the macro `MINOR(inode->i_rdev)` to extract the minor number from the inode. This value is then used to index into device arrays.

4. **What happens if access is attempted on a minor number that doesn't exist?**
   - The driver must validate the minor number and return an error code (like `-ENXIO`) if it's out of range. Otherwise, a segmentation fault can occur.

5. **How can data be efficiently stored for many devices?**
   - Create arrays of device structures, one per minor number. Index into these arrays using the minor number extracted from file operations.

## Task 2: Add Per-Device Locks

Extend the driver to include a **per-device mutex** to prevent concurrent access to individual devices:

**Key Requirements:**

- Create an array of mutexes, one per device
- Lock in open(), unlock in release() (simple exclusive access model)
- OR: Lock in read/write to protect just the data access
- Handle EBUSY if device is already locked (optional)

**Solution**

```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>

#define NUM_DEVICES 4

static dev_t devt;
static struct cdev cdev;
static struct class *cls;
static struct mutex dev_locks[NUM_DEVICES];
static char dev_data[NUM_DEVICES][256];

static ssize_t dev_read(struct file *f, char __user *buf, size_t len, loff_t *pos)
{
 int minor = MINOR(f->f_inode->i_rdev);
 if (minor >= NUM_DEVICES) return -ENXIO;
 
 mutex_lock(&dev_locks[minor]);
 len = strlen(dev_data[minor]);
 copy_to_user(buf, dev_data[minor], len);
 mutex_unlock(&dev_locks[minor]);
 return len;
}

static ssize_t dev_write(struct file *f, const char __user *buf, size_t len, loff_t *pos)
{
 int minor = MINOR(f->f_inode->i_rdev);
 if (minor >= NUM_DEVICES) return -ENXIO;
 if (len > 255) len = 255;
 
 mutex_lock(&dev_locks[minor]);
 copy_from_user(dev_data[minor], buf, len);
 dev_data[minor][len] = '\0';
 mutex_unlock(&dev_locks[minor]);
 return len;
}

static const struct file_operations fops = {
 .read = dev_read,
 .write = dev_write,
};

static int __init dev_init(void)
{
 int i;
 alloc_chrdev_region(&devt, 0, NUM_DEVICES, "multidev");
 cdev_init(&cdev, &fops);
 cdev_add(&cdev, devt, NUM_DEVICES);
 cls = class_create(THIS_MODULE, "multidev");
 
 for (i = 0; i < NUM_DEVICES; i++) {
  device_create(cls, NULL, MKDEV(MAJOR(devt), i), NULL, "dev%d", i);
  mutex_init(&dev_locks[i]);
 }
 return 0;
}

static void __exit dev_exit(void)
{
 int i;
 for (i = 0; i < NUM_DEVICES; i++)
  device_destroy(cls, MKDEV(MAJOR(devt), i));
 class_destroy(cls);
 cdev_del(&cdev);
 unregister_chrdev_region(devt, NUM_DEVICES);
}

MODULE_LICENSE("GPL");
module_init(dev_init);
module_exit(dev_exit);
```

## Task 3: Write Test Application

Create a user-space program that:

1. Opens multiple device files simultaneously (using threads or forks)
2. Writes to each device
3. Reads from each device
4. Demonstrates that each device maintains independent state

**Solution**

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

void *device_thread(void *arg)
{
 int dev_num = *(int *)arg;
 char filename[32], buf[256];
 int fd;
 
 sprintf(filename, "/dev/dev%d", dev_num);
 fd = open(filename, O_RDWR);
 
 sprintf(buf, "Device %d data", dev_num);
 write(fd, buf, strlen(buf));
 
 memset(buf, 0, 256);
 read(fd, buf, 256);
 printf("Device %d read: %s\n", dev_num, buf);
 
 close(fd);
 pthread_exit(NULL);
}

int main()
{
 pthread_t threads[4];
 int device_nums[4] = {0, 1, 2, 3};
 int i;
 
 for (i = 0; i < 4; i++)
  pthread_create(&threads[i], NULL, device_thread, &device_nums[i]);
 
 for (i = 0; i < 4; i++)
  pthread_join(threads[i], NULL);
 
 return 0;
}
```