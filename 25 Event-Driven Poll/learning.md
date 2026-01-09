# 25 Advanced Concepts – Device Commands and Custom Control Interface

## Overview

While `read()` and `write()` are great for sending data, sometimes you need to send **commands** to a device—things like:

- Set temperature sensor threshold
- Enable/disable a GPIO pin
- Reset device state
- Configure device options

For this, Linux provides **ioctl()** (input/output control), which lets you define custom commands for your device.

By the end of this section, you will:

- Understand ioctl command definitions
- Implement `unlocked_ioctl()` in your character driver
- Create command-specific handlers
- Write user-space code to send ioctl commands

## What Is ioctl()?

**ioctl** is a system call that sends arbitrary commands to a device driver:

```c
int ioctl(int fd, unsigned long request, ...);
```

Example:

```c
fd = open("/dev/mydevice", O_RDWR);
ioctl(fd, MY_COMMAND, &data);
close(fd);
```

The driver receives the command number and can take action. Unlike `read()`/`write()`, ioctl supports request-specific functionality.

## Defining ioctl Commands

Commands are 32-bit integers with a specific structure:

```
Bits 0-7:   Command number (0-255)
Bits 8-13:  Command size (argument size in bytes)
Bits 14:    Read bit (1 = data flows from device to user)
Bits 15:    Write bit (1 = data flows from user to device)
Bits 16-30: Magic number (device identifier, e.g., 'T' for Temperature)
Bits 31:    Direction bit (combined read/write)
```

Use **macros** to define commands easily:

```c
#include <linux/ioctl.h>

#define DEVICE_MAGIC 'T'  // Choose a unique letter

// Command to set threshold (write-only)
#define IOCTL_SET_THRESHOLD _IOW(DEVICE_MAGIC, 1, int)

// Command to get temperature (read-only)
#define IOCTL_GET_TEMP _IOR(DEVICE_MAGIC, 2, int)

// Command to reset device (no arguments)
#define IOCTL_RESET _IO(DEVICE_MAGIC, 3)
```

Macro meanings:

- `_IO(magic, nr)` — Command with no argument
- `_IOR(magic, nr, type)` — Read from device
- `_IOW(magic, nr, type)` — Write to device
- `_IOWR(magic, nr, type)` — Read and write

## Implementing ioctl in Your Driver

Add `unlocked_ioctl()` to your `file_operations` structure:

```c
static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int value;

    switch(cmd) {
    case IOCTL_SET_THRESHOLD:
        if (copy_from_user(&value, (int __user *)arg, sizeof(int)))
            return -EFAULT;
        printk(KERN_INFO "Setting threshold to %d\n", value);
        threshold = value;
        break;

    case IOCTL_GET_TEMP:
        value = current_temperature;  // Read from sensor
        if (copy_to_user((int __user *)arg, &value, sizeof(int)))
            return -EFAULT;
        break;

    case IOCTL_RESET:
        printk(KERN_INFO "Device reset\n");
        current_temperature = 0;
        break;

    default:
        return -EINVAL;  // Invalid command
    }

    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .unlocked_ioctl = dev_ioctl,
    .release = dev_release,
};
```

## Key Concepts

### ioctl Macro Parameters

- **magic**: Unique identifier for your device class (8 bits, often a single letter like 'T', 'L', 'S')
- **nr**: Command number (8 bits, 0-255, allows up to 256 commands)
- **type**: Argument type for size calculation (`int`, `char`, custom struct, etc.)

### Direction Flags

- `_IO`: No data transfer (command only)
- `_IOR`: Read from device (kernel → user)
- `_IOW`: Write to device (user → kernel)
- `_IOWR`: Bidirectional (user ↔ kernel)

### Error Handling

Common ioctl return values:

- `0` — Success
- `-EINVAL` — Invalid command or argument
- `-EFAULT` — Bad user-space pointer
- `-EBUSY` — Device busy
- `-EACCES` — Permission denied

## Step-by-Step: Temperature Sensor Driver with ioctl

### 1. Create the Header File

```c
// temp_sensor.h
#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <linux/ioctl.h>

#define DEVICE_MAGIC 'T'

#define IOCTL_SET_THRESHOLD _IOW(DEVICE_MAGIC, 1, int)
#define IOCTL_GET_TEMP      _IOR(DEVICE_MAGIC, 2, int)
#define IOCTL_RESET         _IO(DEVICE_MAGIC, 3)

#endif
```

### 2. Create the Driver

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include "temp_sensor.h"

#define DEVICE_NAME "tempsensor"

static int major;
static int current_temperature = 25;  // Simulated sensor
static int threshold = 30;

static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[%s] Device opened\n", DEVICE_NAME);
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    char temp_str[50];
    int bytes;

    bytes = snprintf(temp_str, sizeof(temp_str), "Temperature: %d°C\n", current_temperature);
    return simple_read_from_buffer(buf, len, offset, temp_str, bytes);
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    char input[10];

    if (len > sizeof(input) - 1)
        len = sizeof(input) - 1;

    if (copy_from_user(input, buf, len))
        return -EFAULT;

    input[len] = '\0';
    sscanf(input, "%d", &current_temperature);

    printk(KERN_INFO "Temperature updated to %d°C\n", current_temperature);
    return len;
}

static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int value;

    switch(cmd) {
    case IOCTL_SET_THRESHOLD:
        if (copy_from_user(&value, (int __user *)arg, sizeof(int)))
            return -EFAULT;
        threshold = value;
        printk(KERN_INFO "Threshold set to %d°C\n", threshold);
        break;

    case IOCTL_GET_TEMP:
        if (copy_to_user((int __user *)arg, &current_temperature, sizeof(int)))
            return -EFAULT;
        printk(KERN_INFO "Returning temperature: %d°C\n", current_temperature);
        break;

    case IOCTL_RESET:
        current_temperature = 25;
        threshold = 30;
        printk(KERN_INFO "Device reset to defaults\n");
        break;

    default:
        printk(KERN_WARNING "Unknown ioctl command: 0x%X\n", cmd);
        return -EINVAL;
    }

    return 0;
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
    .unlocked_ioctl = dev_ioctl,
    .release = dev_release,
};

static int __init temp_driver_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        printk(KERN_ALERT "Failed to register driver\n");
        return major;
    }

    printk(KERN_INFO "Temperature sensor driver loaded (major: %d)\n", major);
    return 0;
}

static void __exit temp_driver_exit(void)
{
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "Temperature sensor driver unloaded\n");
}

module_init(temp_driver_init);
module_exit(temp_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Temperature sensor driver with ioctl");
```

### 3. Build and Test

```bash
make
sudo insmod temp_driver.ko
sudo mknod /dev/tempsensor c <major> 0
sudo chmod 666 /dev/tempsensor
```

**From command line:**

```bash
# Read temperature
cat /dev/tempsensor

# Update temperature via write
echo "35" > /dev/tempsensor

# Use ioctl via C program (see answers.md)
```

## Advantages of ioctl

1. **Flexibility**: Define unlimited custom commands
2. **Efficiency**: Faster than multiple read/write calls
3. **Structured data**: Pass complex structs between user and kernel
4. **Device control**: Send control signals without data transfer
5. **Safety**: Built-in validation of command structure

## Important Notes

1. **Choose unique magic number**: Avoid conflicts with other drivers (check `/usr/include/linux/`)
2. **Validate all user pointers**: Always use `copy_from_user()` and `copy_to_user()`
3. **Check return values**: Both macros and kernel functions can fail
4. **Document commands**: Keep command definitions accessible to user-space tools
5. **Use proper locking**: Protect shared data with mutexes in ioctl
6. **Test edge cases**: Invalid commands, NULL pointers, size mismatches
