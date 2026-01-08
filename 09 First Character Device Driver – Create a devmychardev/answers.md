# 09 Character Device Driver - Assignment Answers and Tasks

## Task 1: Theory Check

1. **What is a major number? What is a minor number?**
   - **Major number**: Identifies the driver itself. The kernel uses it to route file operations to the correct driver.
   - **Minor number**: Identifies a specific device instance managed by that driver. For example, `/dev/ttyS0` and `/dev/ttyS1` might have the same major number but different minor numbers.

2. **What does `register_chrdev()` do?**
   - It registers a character device driver with the kernel. It takes a major number (0 = auto-assign), a device name, and a `file_operations` structure. The kernel returns the assigned major number or a negative error code.

3. **What would happen if you don't call `unregister_chrdev()` on exit?**
   - The driver remains registered in the kernel. Subsequent attempts to load the same driver would fail. The kernel would still route calls to the now-unloaded driver code, causing crashes or undefined behavior.

4. **Why do we use `copy_from_user()` in `write()`?**
   - User-space pointers cannot be directly dereferenced from kernel code. `copy_from_user()` safely transfers data from user space to kernel space, with proper memory protection and error handling.

5. **Why is `device_buffer[]` important?**
   - It stores the data written by the user. Without it, the data would be lost after the `write()` function returns. It also allows the `read()` function to retrieve previously written data.

## Task 2: Modify Your Driver

Maintain a counter of how many bytes were written and display it on read:

**Key Requirements:**

1. Track total bytes written across all `write()` calls
2. In `read()`, display: "You wrote <N> bytes last time."

**Sample Solution:**

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mychardev"
#define BUFFER_SIZE 1024

static int major;
static char device_buffer[BUFFER_SIZE];
static int last_write_count = 0;  // Track bytes written

static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[%s] Device opened\n", DEVICE_NAME);
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    static char info[200];
    
    snprintf(info, sizeof(info), 
             "You wrote %d bytes last time.\nBuffer content: %s\n", 
             last_write_count, device_buffer);
    
    return simple_read_from_buffer(buf, len, offset, info, strlen(info));
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    if (len > BUFFER_SIZE - 1)
        len = BUFFER_SIZE - 1;

    if (copy_from_user(device_buffer, buf, len))
        return -EFAULT;

    device_buffer[len] = '\0';
    last_write_count = len;  // Store byte count
    
    printk(KERN_INFO "[%s] Received %zd bytes: %s\n", DEVICE_NAME, len, device_buffer);
    return len;
}

static int dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[%s] Device closed\n", DEVICE_NAME);
    return 0;
}

static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = dev_open,
    .read    = dev_read,
    .write   = dev_write,
    .release = dev_release,
};

static int __init char_driver_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        printk(KERN_ALERT "Failed to register character device\n");
        return major;
    }

    printk(KERN_INFO "Char driver registered with major number %d\n", major);
    return 0;
}

static void __exit char_driver_exit(void)
{
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "Char driver unregistered\n");
}

module_init(char_driver_init);
module_exit(char_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Character device with byte counter");
```

**Test it:**

```bash
echo "testing" > /dev/mychardev
cat /dev/mychardev
# Output: You wrote 8 bytes last time.
#         Buffer content: testing
```

## Task 3: Write a Small C Program

Write a user-space C program to:

- Open `/dev/mychardev`
- Write "Testing from app"
- Read back and print the output
- Close the file

**Sample Solution:**

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
    int fd;
    char write_buffer[] = "Testing from app";
    char read_buffer[200];
    ssize_t bytes_read;
    
    // Open the device
    fd = open("/dev/mychardev", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/mychardev");
        return 1;
    }
    
    printf("Device opened successfully\n");
    
    // Write to the device
    if (write(fd, write_buffer, strlen(write_buffer)) < 0) {
        perror("Write failed");
        close(fd);
        return 1;
    }
    
    printf("Wrote: %s\n", write_buffer);
    
    // Read from the device
    bytes_read = read(fd, read_buffer, sizeof(read_buffer) - 1);
    if (bytes_read < 0) {
        perror("Read failed");
        close(fd);
        return 1;
    }
    
    read_buffer[bytes_read] = '\0';
    printf("Read from device:\n%s\n", read_buffer);
    
    // Close the device
    close(fd);
    printf("Device closed\n");
    
    return 0;
}
```

**Compile and run:**

```bash
gcc -o app_test app_test.c
./app_test
```

**Expected output:**

```
Device opened successfully
Wrote: Testing from app
Read from device:
You wrote 16 bytes last time.
Buffer content: Testing from app

Device closed
```

This demonstrates the complete cycle of opening, writing, reading, and closing a character device from user space.
