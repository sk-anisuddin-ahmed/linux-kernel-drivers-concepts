# 12 Automatic /dev Creation with udev, class_create, and Dynamic Registration

## Learning Objectives

- Use class_create() to register device class
- Use device_create() to automatically create /dev files
- Understand udev rules for device management
- Eliminate manual mknod commands
- Implement proper device cleanup

## Dynamic Device Registration

Instead of manually creating /dev files with mknod, let the kernel do it:

```c
#include <linux/device.h>

static struct class *device_class;
static struct device *device_node;

static int __init char_dev_init(void)
{
    int major;
    
    // Register character device
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if(major < 0) {
        printk(KERN_ERR "Failed to register\n");
        return major;
    }
    
    // Create device class
    device_class = class_create(THIS_MODULE, DEVICE_NAME);
    if(IS_ERR(device_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(device_class);
    }
    
    // Create device file
    device_node = device_create(device_class, NULL, MKDEV(major, 0), 
                                NULL, DEVICE_NAME);
    if(IS_ERR(device_node)) {
        class_destroy(device_class);
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(device_node);
    }
    
    printk(KERN_INFO "Device created successfully\n");
    return 0;
}

static void __exit char_dev_exit(void)
{
    device_destroy(device_class, MKDEV(major, 0));
    class_destroy(device_class);
    unregister_chrdev(major, DEVICE_NAME);
}
```

## Understanding udev

udev (userspace /dev) automatically:

- **Discovers devices** as they appear
- **Creates /dev files** based on rules
- **Sets permissions** and ownership
- **Triggers scripts** on device events

## udev Rules File

Create `/etc/udev/rules.d/99-mydevice.rules`:

```bash
# Match our device and create /dev/mychardev
SUBSYSTEM=="misc", NAME="mychardev", MODE="0666"

# Or match by major/minor number
DEVPATH=="/devices/virtual/misc/mychardev", MODE="0666", GROUP="plugdev"
```

## Major Numbers and Device Classes

For modern kernels:

- Use `major = register_chrdev(0, ...)` to get dynamic major number
- The `0` means "pick any available major number"
- Kernel assigns and returns the number

## Full Dynamic Example

```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>

#define DEVICE_NAME "dynamic_dev"

static int major;
static struct class *dev_class;
static struct device *dev_device;

static struct file_operations fops = {
    .open = device_open,
    .read = device_read,
    .write = device_write,
    .release = device_release,
};

static int __init init_module(void)
{
    // Get dynamic major number
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if(major < 0) {
        pr_err("Registration failed\n");
        return major;
    }
    
    // Create class
    dev_class = class_create(THIS_MODULE, DEVICE_NAME);
    if(IS_ERR(dev_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(dev_class);
    }
    
    // Create device
    dev_device = device_create(dev_class, NULL, MKDEV(major, 0), 
                               NULL, "%s", DEVICE_NAME);
    if(IS_ERR(dev_device)) {
        class_destroy(dev_class);
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(dev_device);
    }
    
    pr_info("Device %s created with major %d\n", DEVICE_NAME, major);
    return 0;
}

static void __exit cleanup_module(void)
{
    device_destroy(dev_class, MKDEV(major, 0));
    class_destroy(dev_class);
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("Device destroyed\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Dynamic device creation example");

module_init(init_module);
module_exit(cleanup_module);
```

## Testing Dynamic Device Creation

```bash
# Compile and load
make
sudo insmod dynamic_dev.ko

# Check if device was created automatically
ls -la /dev/dynamic_dev

# Test the device
cat /dev/dynamic_dev
echo "test" > /dev/dynamic_dev

# Unload (device automatically removed)
sudo rmmod dynamic_dev
```

## Key Benefits

- **No manual mknod** required
- **Permissions handled by udev** rules
- **Automatic cleanup** on module unload
- **Proper device discovery**
- **Works with modern systems**
