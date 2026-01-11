# 12 Automatic /dev Creation with udev, class_create, and Dynamic Registration - Q&A

## Assignments

1. **Implement class_create() and device_create() in your character driver**

```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

static dev_t devt;
static struct cdev cdev;
static struct class *cls;

static const struct file_operations fops = {
    .owner = THIS_MODULE,
};

static int __init mydev_init(void)
{
    alloc_chrdev_region(&devt, 0, 1, "mydev");
    cdev_init(&cdev, &fops);
    cdev_add(&cdev, devt, 1);
    cls = class_create(THIS_MODULE, "mydev_class");
    device_create(cls, NULL, devt, NULL, "mydev");
    return 0;
}

static void __exit mydev_exit(void)
{
    device_destroy(cls, devt);
    class_destroy(cls);
    cdev_del(&cdev);
    unregister_chrdev_region(devt, 1);
}

MODULE_LICENSE("GPL");
module_init(mydev_init);
module_exit(mydev_exit);
```

1. **Create a udev rules file for automatic permissions**

Create `/etc/udev/rules.d/99-mydev.rules`:

```
KERNEL=="mydev", MODE="0666", GROUP="users"
```

1. **Test that /dev file is created without manual mknod**

```bash
cat /proc/devices | grep mydev
```

1. **Verify permissions are set correctly by udev rules**

```bash
# Check permissions
ls -l /dev/mydev

# Verify group ownership
stat /dev/mydev
```

1. **Test device cleanup on module unload**

```bash
sudo rmmod mydev
ls -la /dev/mydev
cat /proc/devices | grep mydev
```

## Questions & Answers

1. **What does class_create() do?**
   - It creates a device class in the kernel that udev can recognize. It is like registering a device type with the system so the system knows how to handle it.

2. **How does device_create() differ from mknod?**
   - `mknod` is manual - the /dev file is created by hand. `device_create()` tells udev to automatically create it when the module loads. The difference is automatic vs manual creation.

3. **What is the purpose of udev rules?**
   - They tell udev what to do when a device appears. Permissions can be set, symbolic links created, or scripts run. For example, a device can be made readable/writable by a specific user group.

4. **How are dynamic major numbers (major=0) used?**
   - A value of 0 is passed as the major number to `alloc_chrdev_region()`. The kernel automatically assigns an available number. This is safer than picking a number manually and causing conflicts.

5. **What permissions should be set for device files?**
   - Usually 0666 (everyone can read/write) or 0644 (owner reads/writes, others read only). This depends on the device type - some are sensitive and require 0600 (owner only).
