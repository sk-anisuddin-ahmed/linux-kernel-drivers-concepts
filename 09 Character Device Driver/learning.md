# 09 First Character Device Driver – Create a /dev/mychardev

## Overview

You will write a **character device driver** (also called a *char driver*) — the kind of driver used for keyboards, serial ports, sensors, GPIOs, etc.

By the end of this section, you will:

- Understand what a character device is
- Create your own device file like `/dev/mychardev`
- Handle `open()`, `read()`, `write()`, and `release()` in your module
- Use user space tools like `cat`, `echo`, and even a simple C app to talk to your device

## What Is a Character Device?

In Linux, devices are treated as **files**. When you open `/dev/ttyUSB0`, you're really calling `open()` on a **character device file**, which is backed by a **driver in the kernel**.

These character devices support:

- **Byte-stream access** (like reading/writing characters from a sensor or serial port)
- User space calls like `read()`, `write()`, `open()`, `close()` that are forwarded into the **driver's functions**

So when you type `echo "abc" > /dev/mydevice`, the kernel calls your module's `write()` function. Neat, right?

## Major and Minor Numbers

Every device is identified by:

- A **major number** → identifies the driver
- A **minor number** → identifies the specific device handled by that driver

Linux uses these numbers to **route the system calls** to the correct code.

You will:

1. Register a new character device driver
2. Create a device file using `mknod` or `udev`
3. Implement `open()`, `read()`, `write()`, and `release()`
4. Interact with it using terminal commands and a C program

## Step-by-Step: Your First Char Device Driver

### 1. Create Project Files

```bash
mkdir ~/char_dev
cd ~/char_dev
nano char_driver.c
```

Paste this code:

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mychardev"
#define BUFFER_SIZE 1024

static int major;
static char device_buffer[BUFFER_SIZE];
static int open_count = 0;

static int dev_open(struct inode *inode, struct file *file)
{
    open_count++;
    printk(KERN_INFO "[%s] Device opened %d times\n", DEVICE_NAME, open_count);
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    return simple_read_from_buffer(buf, len, offset, device_buffer, strlen(device_buffer));
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    if (len > BUFFER_SIZE - 1)
        len = BUFFER_SIZE - 1;

    if (copy_from_user(device_buffer, buf, len))
        return -EFAULT;

    device_buffer[len] = '\0';
    printk(KERN_INFO "[%s] Received: %s\n", DEVICE_NAME, device_buffer);
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
MODULE_DESCRIPTION("Simple character device driver");
```

### 2. Create the Makefile

```bash
nano Makefile
```

```makefile
obj-m += char_driver.o

all:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

### 3. Build, Insert, and Create Device File

```bash
make
sudo insmod char_driver.ko
dmesg | tail
```

Look for:

```
Char driver registered with major number 240
```

Now create the device file manually:

```bash
sudo mknod /dev/mychardev c 240 0
sudo chmod 666 /dev/mychardev
```

(Note: replace `240` with whatever major number you saw in `dmesg`)

### 4. Interact with the Device

Write to the device:

```bash
echo "hello kernel" > /dev/mychardev
```

Read from the device:

```bash
cat /dev/mychardev
```

Check `dmesg`:

```bash
dmesg | tail -10
```

You'll see:

```
[mychardev] Device opened 1 times
[mychardev] Received: hello kernel
[mychardev] Device closed
```

You just created your own **functional character driver**!

### 5. Clean Up

```bash
sudo rmmod char_driver
sudo rm /dev/mychardev
```

## How It Works (Line by Line)

- `register_chrdev()` registers the driver and assigns a major number
- `file_operations` structure contains pointers to our driver functions
- `dev_open()` is called when `open()` is used on `/dev/mychardev`
- `dev_read()` is called when `cat` or `read()` is used
- `dev_write()` is called when `echo` or `write()` is used
- `dev_release()` is called when the file is closed
- `simple_read_from_buffer()` safely copies kernel data to user space
- `copy_from_user()` safely copies user data to kernel space

## Key Concepts

### `register_chrdev()` Function

```c
int register_chrdev(unsigned int major, const char *name, 
                    const struct file_operations *fops)
```

- If `major == 0`, the kernel automatically assigns a free major number
- Returns the assigned major number (positive) or error code (negative)
- Registers your driver to handle system calls for that major number

### `file_operations` Structure

This structure contains function pointers for all file operations:

```c
struct file_operations {
    struct module *owner;
    loff_t (*llseek) (struct file *, loff_t, int);
    ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
    ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
    int (*open) (struct inode *, struct file *);
    int (*release) (struct inode *, struct file *);
    // ... and many more
};
```

Only the functions you implement need to be included; others default to NULL.

### `mknod` Command

```bash
sudo mknod /dev/mychardev c 240 0
```

- `c` = character device
- `240` = major number
- `0` = minor number
- This creates a device file that routes to your driver

### Permission Mode 666

```bash
sudo chmod 666 /dev/mychardev
```

Makes the device readable and writable by anyone. In production, use more restrictive permissions like `644`.

## Advantages of Character Drivers

1. **Byte-stream access**: Ideal for serial devices, keyboards, etc.
2. **Simple interface**: `open()`, `read()`, `write()`, `close()`
3. **System call mapping**: Automatic routing from `/dev` to your kernel functions
4. **No buffering by default**: Data flows directly between user and kernel
5. **Easy testing**: Use standard tools like `echo`, `cat`, `dd`

## Important Notes

1. **Always clean up**: Call `unregister_chrdev()` in your exit function
2. **Validate buffer sizes**: Check length before `copy_from_user()`
3. **Null-terminate strings**: Prevent buffer overruns
4. **Use appropriate permissions**: 666 is too open for most cases
5. **Handle error returns**: Check for negative return values from kernel functions
6. **Provide major number explicitly in newer kernels**: Consider using `cdev` interface instead of `register_chrdev()` for production drivers
