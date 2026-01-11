# 09 Character Device Driver - Assignment Answers and Tasks

## Task 1: Theory Check

1. **What is a major number? What is a minor number?**
   - **Major number**: Identifies the device driver itself. The kernel uses it to route file operations to the correct driver.
   - **Minor number**: Identifies a specific device instance managed by that driver. For example, `/dev/ttyS0` and `/dev/ttyS1` might have the same major number but different minor numbers.

2. **What does `register_chrdev()` do?**
   - It registers a character device driver with the kernel. It takes a major number (0 = auto-assign), a device name, and a `file_operations` structure. The kernel returns the assigned major number or a negative error code.

3. **What would happen if unregister_chrdev() is not called on exit?**
   - The driver remains registered in the kernel. Subsequent attempts to load the same driver would fail. The kernel would still route calls to the now-unloaded driver code, causing crashes or undefined behavior.

4. **Why is `copy_from_user()` used in `write()`?**
   - User-space pointers cannot be directly dereferenced from kernel code. `copy_from_user()` safely transfers data from user space to kernel space, with proper memory protection and error handling.

5. **Why is `device_buffer[]` important?**
   - It stores data written to the device. Without it, the data would be lost after the `write()` function returns. It also allows the `read()` function to retrieve previously written data.


## Task 2: Modify Your Driver

Maintain a counter of how many bytes were written and display it on read:

**Key Requirements:**

1. Track total bytes written across all `write()` calls
2. In `read()`, display: "You wrote <N> bytes last time."

**Solution**

```c
#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/uaccess.h>
#include<linux/errno.h>
#include<linux/types.h>
#include<linux/cdev.h>
#include<linux/kdev_t.h>
#include<linux/device.h>
#include<linux/string.h>

//Char Device Register
#define Dev_Name "rw_dev"
#define Class_Name "rw_class"
static dev_t devt;
static struct cdev rw_cdev;
static struct class *rw_class;
static struct device *rw_device;

static int rcnt, wcnt;
static char kbuf[4096];

#define print(fmt, ...) pr_info(fmt "\n", ##__VA_ARGS__)

static ssize_t rw_read (struct file *file, char __user *buf, size_t len, loff_t *pos)
{
        ssize_t min_len = 0;
        size_t avail = sizeof(kbuf) - (size_t)*pos;
        int ret;
        if (avail == 0)
                return 0;
        min_len = ((ssize_t)len > (ssize_t)avail)? (ssize_t)avail: (ssize_t)len;
        ret = copy_to_user(buf, kbuf, min_len);
        if (ret < 0)
                return -1;
        rcnt += (int)min_len;
        *pos += (loff_t)min_len;
        print("Total read bytes: %d", rcnt);
        return min_len;
}

static ssize_t rw_write (struct file *file,const char __user *buf, size_t len, loff_t *pos)
{
        ssize_t min_len = (len > sizeof(kbuf))? sizeof(kbuf) : len;
        int ret = copy_from_user(kbuf, buf, min_len);
        if (ret < 0)
                return -1;
        wcnt += (int)min_len;
        print("Total written bytes: %d", wcnt);
        return min_len;
}

static int rw_open (struct inode *inode, struct file *file)
{
        print("device opened");
        return 0;
}

static int rw_close (struct inode *inode, struct file *file)
{
        print("device closed");
        return 0;
}

static const struct file_operations rw_fops = {
        .owner = THIS_MODULE,
        .read = rw_read,
        .write = rw_write,
        .open = rw_open,
        .release = rw_close
};

static int __init dev_init(void)
{
        int ret = alloc_chrdev_region(&devt, 0, 1, Dev_Name);
        cdev_init(&rw_cdev ,&rw_fops);
        ret &= cdev_add(&rw_cdev, devt, 1);
        rw_class = class_create(THIS_MODULE, Class_Name);
        rw_device = device_create(rw_class, NULL, devt, NULL, Dev_Name);
        print("module loaded\n");
        return 0;
}

static void __exit dev_exit(void)
{
        device_destroy(rw_class, devt);
        class_destroy(rw_class);
        cdev_del(&rw_cdev);
        unregister_chrdev_region(devt, 1);
        print("module unloaded\n");
}

MODULE_LICENSE("GPL");

module_init(dev_init);
module_exit(dev_exit);
```

## Task 3: Write a Small C Program

Write a user-space C program to:

- Open `/dev/mychardev`
- Write "Testing from app"
- Read back and print the output
- Close the file
**Solution**

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
    int fd;
    char write_buf[] = "Testing from app";
    char read_buf[256];
    ssize_t bytes_written, bytes_read;

    // Open the device
    fd = open("/dev/rw_dev", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/rw_dev");
        return -1;
    }
    printf("Device opened successfully\n");

    // Write data to device
    bytes_written = write(fd, write_buf, strlen(write_buf));
    if (bytes_written < 0) {
        perror("Write failed");
        close(fd);
        return -1;
    }
    printf("Written %ld bytes: %s\n", bytes_written, write_buf);

    // Read data back from device
    bytes_read = read(fd, read_buf, sizeof(read_buf) - 1);
    if (bytes_read < 0) {
        perror("Read failed");
        close(fd);
        return -1;
    }
    read_buf[bytes_read] = '\0';  // Null-terminate the string
    printf("Read %ld bytes: %s\n", bytes_read, read_buf);

    // Close the device
    if (close(fd) < 0) {
        perror("Failed to close device");
        return -1;
    }
    printf("Device closed\n");

    return 0;
}
```
