# 11 ioctl Device Commands - Assignment Answers and Tasks

## Task 1: Theory Check

1. **What is ioctl and when would you use it instead of read/write?**
   - `ioctl` stands for "input/output control" - it's a way to send custom commands to a device driver. Useful when reading configure settings, query device status, or get special information.

2. **How do the macros _IO, _IOR, _IOW, and _IOWR differ?**
   - `_IO`: No data transfer.
   - `_IOR`: Read data from driver.
   - `_IOW`: Write data to driver.
   - `_IOWR`: Both directions.

3. **What is the "magic number" in an ioctl command?**
   - A unique identifier to separate a driver's commands from other drivers. For example, if magic number 'T' is used, all commands start with 'T'. This prevents command conflicts between different drivers.

4. **Why are copy_from_user() and copy_to_user() used in ioctl?**
   - Data from user-space cannot be accessed directly from kernel code (different memory spaces). These functions safely transfer data while performing permission checks. It is the same safety requirement as with read/write.

5. **What should be returned on an invalid ioctl command?**
   - `-EINVAL` (invalid argument) should be returned. This tells user-space that the command was not recognized. It is the standard Linux convention for bad parameters.

## Task 2: Add More Commands

Extend the temperature driver to include:

1. Command to get the threshold (READ)
2. Command to enable/disable alerts (WRITE boolean flag)
3. Command to get alert status (READ)

**Key Requirements:**

- Define 3 new ioctl commands using appropriate macros
- Implement handlers in the ioctl function
- Use a static variable to track alert enable/disable

**Solution**

```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define MAGIC 'T'
#define GET_THRESHOLD     _IOR(MAGIC, 1, int)
#define SET_THRESHOLD     _IOW(MAGIC, 2, int)
#define GET_TEMP          _IOR(MAGIC, 3, int)
#define ENABLE_ALERTS     _IOW(MAGIC, 4, int)
#define GET_ALERT_STATUS  _IOR(MAGIC, 5, int)

static dev_t devt;
static struct cdev cdev;
static struct class *cls;
static int threshold = 30;
static int alerts_enabled = 0;
static int current_temp = 25;

static long dev_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    int val;

    switch (cmd) {
    case GET_THRESHOLD:
        if (copy_to_user((int __user *)arg, &threshold, sizeof(int)))
            return -EFAULT;
        break;
    case SET_THRESHOLD:
        if (copy_from_user(&val, (int __user *)arg, sizeof(int)))
            return -EFAULT;
        threshold = val;
        break;
    case GET_TEMP:
        if (copy_to_user((int __user *)arg, &current_temp, sizeof(int)))
            return -EFAULT;
        break;
    case ENABLE_ALERTS:
        if (copy_from_user(&val, (int __user *)arg, sizeof(int)))
            return -EFAULT;
        alerts_enabled = val;
        break;
    case GET_ALERT_STATUS:
        if (copy_to_user((int __user *)arg, &alerts_enabled, sizeof(int)))
            return -EFAULT;
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = dev_ioctl,
};

static int __init tempsensor_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&devt, 0, 1, "tempsensor");
    if (ret < 0)
        return ret;

    cdev_init(&cdev, &fops);
    cdev.owner = THIS_MODULE;

    ret = cdev_add(&cdev, devt, 1);
    if (ret < 0) {
        unregister_chrdev_region(devt, 1);
        return ret;
    }

    cls = class_create(THIS_MODULE, "tempsensor");
    if (IS_ERR(cls)) {
        cdev_del(&cdev);
        unregister_chrdev_region(devt, 1);
        return PTR_ERR(cls);
    }

    if (IS_ERR(device_create(cls, NULL, devt, NULL, "tempsensor"))) {
        class_destroy(cls);
        cdev_del(&cdev);
        unregister_chrdev_region(devt, 1);
        return -EINVAL;
    }

    pr_info("tempsensor: module loaded\n");
    return 0;
}

static void __exit tempsensor_exit(void)
{
    device_destroy(cls, devt);
    class_destroy(cls);
    cdev_del(&cdev);
    unregister_chrdev_region(devt, 1);
    pr_info("tempsensor: module unloaded\n");
}

MODULE_LICENSE("GPL");
module_init(tempsensor_init);
module_exit(tempsensor_exit);
```

## Task 3: Write a Complete ioctl Test Application

Create a user-space C program that:

1. Opens `/dev/tempsensor`
2. Sends commands to set threshold, get temperature, enable alerts
3. Reads the temperature via read() and via ioctl()
4. Displays all results

**Solution**

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MAGIC 'T'
#define GET_THRESHOLD _IOR(MAGIC, 1, int)
#define SET_THRESHOLD _IOW(MAGIC, 2, int)
#define GET_TEMP _IOR(MAGIC, 3, int)
#define ENABLE_ALERTS _IOW(MAGIC, 4, int)
#define GET_ALERT_STATUS _IOR(MAGIC, 5, int)

int main() {
 int fd = open("/dev/tempsensor", O_RDWR);
 if (fd < 0) { perror("open"); return 1; }
 
 int temp, threshold, alerts;
 
 // Set threshold
 int new_threshold = 35;
 printf("Setting threshold to %d\n", new_threshold);
 ioctl(fd, SET_THRESHOLD, &new_threshold);
 
 // Get threshold
 ioctl(fd, GET_THRESHOLD, &threshold);
 printf("Current threshold: %d\n", threshold);
 
 // Get temperature
 ioctl(fd, GET_TEMP, &temp);
 printf("Current temperature: %d\n", temp);
 
 // Enable alerts
 int enable = 1;
 printf("Enabling alerts\n");
 ioctl(fd, ENABLE_ALERTS, &enable);
 
 // Check alert status
 ioctl(fd, GET_ALERT_STATUS, &alerts);
 printf("Alerts enabled: %s\n", alerts ? "YES" : "NO");
 
 close(fd);
 return 0;
}
```
