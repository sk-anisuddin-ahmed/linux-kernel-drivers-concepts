# 13 Multiple Minor Devices – Handling Many Device Instances

## Overview

You've created single and multiple devices, but how do you efficiently handle **many** device instances (e.g., 1000 serial ports, 256 GPIO pins)?

Creating 1000 device structures manually would be wasteful. Instead, use **minor numbers** to differentiate devices and **dynamic allocation** to scale efficiently.

By the end of this section, you will:

- Understand how major/minor numbers scale
- Use a single `file_operations` handler for multiple devices
- Track device state using minor number indexing
- Implement efficient multi-device drivers

## Major and Minor Numbers Revisited

- **Major number**: Identifies the driver (assigned by kernel)
- **Minor number**: Differentiates devices managed by that driver (0-255 typically, but can extend higher)

A single driver with major=250 can manage:

- `/dev/device0` (major=250, minor=0)
- `/dev/device1` (major=250, minor=1)
- `/dev/device2` (major=250, minor=2)
- ... and so on

The kernel routes all these to your driver, using the **minor number to identify which device**.

## Extracting Minor Number

In your file operations, extract the minor number to identify which device:

```c
static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    struct inode *inode = file_inode(file);
    int minor = MINOR(inode->i_rdev);
    
    // Use 'minor' to identify which device
    printk(KERN_INFO "Reading from device minor=%d\n", minor);
    
    // Access device-specific data
    return read_device_data(minor, buf, len);
}
```

## Scaling with Arrays

Store device state in arrays indexed by minor number:

```c
#define MAX_DEVICES 256

static struct {
    int value;
    int status;
    char buffer[BUFFER_SIZE];
} device_state[MAX_DEVICES];

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    struct inode *inode = file_inode(file);
    int minor = MINOR(inode->i_rdev);
    char msg[50];

    if (minor >= MAX_DEVICES)
        return -ENODEV;

    snprintf(msg, sizeof(msg), "Device %d value: %d\n", 
             minor, device_state[minor].value);
    
    return simple_read_from_buffer(buf, len, offset, msg, strlen(msg));
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    struct inode *inode = file_inode(file);
    int minor = MINOR(inode->i_rdev);
    int value;

    if (minor >= MAX_DEVICES)
        return -ENODEV;

    if (sscanf_s_to_int(buf, len, &value) == 0)
        device_state[minor].value = value;

    return len;
}
```

## Real-World Example: GPIO Driver

Imagine controlling 32 GPIO pins. Each pin gets its own device file:

```
/dev/gpio0  (minor=0)  → GPIO pin 0
/dev/gpio1  (minor=1)  → GPIO pin 1
...
/dev/gpio31 (minor=31) → GPIO pin 31
```

Single driver handles all 32 pins:

```c
#define NUM_GPIOS 32

static struct {
    int level;  // 0 = low, 1 = high
    int direction;  // 0 = input, 1 = output
} gpio_state[NUM_GPIOS];

static ssize_t gpio_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    int pin = MINOR(file_inode(file)->i_rdev);
    char msg[50];

    if (pin >= NUM_GPIOS)
        return -ENODEV;

    snprintf(msg, sizeof(msg), "GPIO%d: %s\n", pin, 
             gpio_state[pin].level ? "HIGH" : "LOW");
    
    return simple_read_from_buffer(buf, len, offset, msg, strlen(msg));
}

static ssize_t gpio_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    int pin = MINOR(file_inode(file)->i_rdev);
    char cmd[10];

    if (pin >= NUM_GPIOS)
        return -ENODEV;

    if (len > sizeof(cmd) - 1)
        len = sizeof(cmd) - 1;

    if (copy_from_user(cmd, buf, len))
        return -EFAULT;

    cmd[len] = '\0';

    if (strncmp(cmd, "high", 4) == 0) {
        gpio_state[pin].level = 1;
        printk(KERN_INFO "GPIO%d set HIGH\n", pin);
    } else if (strncmp(cmd, "low", 3) == 0) {
        gpio_state[pin].level = 0;
        printk(KERN_INFO "GPIO%d set LOW\n", pin);
    }

    return len;
}
```

## Step-by-Step: Scalable Multi-Device Driver

### 1. Create the Driver

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#define DEVICE_NAME "scalable"
#define NUM_DEVICES 16

// Device state array
static struct {
    int counter;
    char buffer[256];
} devices[NUM_DEVICES];

static int major;
static struct class *myclass = NULL;

static int dev_open(struct inode *inode, struct file *file)
{
    int minor = MINOR(inode->i_rdev);
    if (minor >= NUM_DEVICES)
        return -ENODEV;

    printk(KERN_INFO "[scalable%d] Opened\n", minor);
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

    bytes = snprintf(msg, sizeof(msg), 
                     "Device %d: counter=%d, buffer=%s\n",
                     minor, devices[minor].counter, devices[minor].buffer);
    
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

    // Store in device-specific buffer
    strcpy(devices[minor].buffer, input);
    devices[minor].counter++;

    printk(KERN_INFO "[scalable%d] Write: %s (count=%d)\n", 
           minor, input, devices[minor].counter);
    
    return len;
}

static int dev_release(struct inode *inode, struct file *file)
{
    int minor = MINOR(inode->i_rdev);
    printk(KERN_INFO "[scalable%d] Closed (writes: %d)\n", 
           minor, devices[minor].counter);
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

static int __init scalable_init(void)
{
    int i;

    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        printk(KERN_ALERT "Failed to register chrdev\n");
        return major;
    }

    // Create device class
    myclass = class_create(THIS_MODULE, "scalabledevs");
    if (IS_ERR(myclass)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(myclass);
    }

    // Create device instances
    for (i = 0; i < NUM_DEVICES; i++) {
        // Initialize device state
        devices[i].counter = 0;
        strcpy(devices[i].buffer, "");

        // Create device file
        device_create(myclass, NULL, MKDEV(major, i), NULL, 
                     "%s%d", DEVICE_NAME, i);
    }

    printk(KERN_INFO "Scalable driver: %d devices created (major=%d)\n", 
           NUM_DEVICES, major);
    return 0;
}

static void __exit scalable_exit(void)
{
    int i;

    // Destroy all device instances
    for (i = 0; i < NUM_DEVICES; i++)
        device_destroy(myclass, MKDEV(major, i));

    class_destroy(myclass);
    unregister_chrdev(major, DEVICE_NAME);

    printk(KERN_INFO "Scalable driver unloaded\n");
}

module_init(scalable_init);
module_exit(scalable_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Scalable multi-minor device driver");
```

### 2. Makefile

```makefile
obj-m += scalable.o

all:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

### 3. Build and Test

```bash
make
sudo insmod scalable.ko
ls /dev/scalable*
```

Test multiple devices:

```bash
echo "message0" > /dev/scalable0
echo "message1" > /dev/scalable1
cat /dev/scalable0
cat /dev/scalable1
```

## Key Concepts

### MINOR() Macro

```c
int minor = MINOR(inode->i_rdev);
```

Extracts the minor number from the device number stored in the inode.

### Device-Specific Storage

Rather than creating separate structures, index into arrays:

```c
devices[minor].value = something;
devices[minor].state = something_else;
```

### Efficiency

- Single `file_operations` handler for unlimited devices
- All data fits in static arrays (if reasonable size)
- Minimal memory overhead per device (just the state struct)
- Fast lookup: O(1) array access by minor number

## Advantages of Minor Number Scaling

1. **Efficient**: Single driver code handles many devices
2. **Scalable**: Can easily support dozens or hundreds of devices
3. **Simple**: No complex data structures needed
4. **Fast**: Direct array indexing is O(1)
5. **Dynamic**: Can disable/enable devices by checking state

## Important Notes

1. **Validate minor numbers**: Always check `minor < MAX_DEVICES`
2. **Initialize device state**: In init function, zero out device arrays
3. **Track state per device**: Use array indexing consistently
4. **Avoid buffer overflows**: Validate string lengths in per-device buffers
5. **Consider mutexes**: If devices can be accessed concurrently, add per-device locks
6. **Documentation**: Clearly document how many minors your driver supports
