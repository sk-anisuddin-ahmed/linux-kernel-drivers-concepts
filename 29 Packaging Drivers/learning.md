# Section 29: Packaging Drivers – Distribution and Installation

## Module Installation Basics

### Makefile for Distribution

```makefile
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

obj-m := my_driver.o

all:
 make -C $(KERNEL_DIR) M=$(PWD) modules

install:
 make -C $(KERNEL_DIR) M=$(PWD) modules_install
 depmod -A

clean:
 make -C $(KERNEL_DIR) M=$(PWD) clean

uninstall:
 rm /lib/modules/$(shell uname -r)/kernel/drivers/my_driver.ko
 depmod -A
```

### Module Information

```c
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Driver description");
MODULE_VERSION("1.0");

MODULE_PARM_DESC(debug, "Enable debug logging");
module_param(debug, int, 0644);
```

## Udev Rules for Auto-Device Creation

```bash
# /etc/udev/rules.d/99-my-driver.rules

# Character device - create /dev/mydevice
SUBSYSTEM=="misc", NAME="mydevice", MODE="0666"

# Class device
SUBSYSTEM=="myclass", ACTION=="add", \
  RUN+="/usr/bin/myapp --add %k"

# With permissions
KERNEL=="mydevice", MODE="0600", OWNER="root"
```

## Package as .deb (Debian)

### debian/control

```
Package: my-driver
Version: 1.0-1
Architecture: all
Maintainer: Your Name <email@example.com>
Description: My kernel driver
 A sample Linux kernel driver package
```

### debian/rules

```makefile
#!/usr/bin/make -f

override_dh_auto_build:
 make

override_dh_auto_install:
 make DESTDIR=$(CURDIR)/debian/my-driver install

%:
 dh $@
```

### Build Command

```bash
debuild -us -uc
```

## Complete Example: Packaged Driver Module

```c
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DRIVER_VERSION "1.0"
#define DRIVER_NAME "pkg_driver"

static int debug = 0;
module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Enable debug logging (default: 0)");

struct driver_device {
    struct cdev cdev;
    struct device *device;
    dev_t dev_num;
    int opened;
};

static struct driver_device drv_dev;

static int driver_open(struct inode *inode, struct file *file)
{
    if (debug) {
        printk(KERN_DEBUG "[%s] Device opened\n", DRIVER_NAME);
    }
    
    if (drv_dev.opened) {
        return -EBUSY;
    }
    
    drv_dev.opened = 1;
    return 0;
}

static int driver_release(struct inode *inode, struct file *file)
{
    if (debug) {
        printk(KERN_DEBUG "[%s] Device closed\n", DRIVER_NAME);
    }
    
    drv_dev.opened = 0;
    return 0;
}

static ssize_t driver_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    char msg[] = "Hello from packaged driver\n";
    size_t len = strlen(msg);
    
    if (*ppos >= len) {
        return 0;
    }
    
    if (count > len - *ppos) {
        count = len - *ppos;
    }
    
    if (copy_to_user(buf, msg + *ppos, count)) {
        return -EFAULT;
    }
    
    *ppos += count;
    return count;
}

static const struct file_operations driver_fops = {
    .owner = THIS_MODULE,
    .open = driver_open,
    .release = driver_release,
    .read = driver_read,
};

static int __init driver_init(void)
{
    struct class *driver_class;
    int ret;
    
    printk(KERN_INFO "[%s] Loading driver v%s\n", DRIVER_NAME, DRIVER_VERSION);
    
    ret = alloc_chrdev_region(&drv_dev.dev_num, 0, 1, DRIVER_NAME);
    if (ret < 0) {
        printk(KERN_ERR "Failed to allocate device number\n");
        return ret;
    }
    
    cdev_init(&drv_dev.cdev, &driver_fops);
    drv_dev.cdev.owner = THIS_MODULE;
    
    ret = cdev_add(&drv_dev.cdev, drv_dev.dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(drv_dev.dev_num, 1);
        return ret;
    }
    
    driver_class = class_create(THIS_MODULE, DRIVER_NAME);
    if (IS_ERR(driver_class)) {
        cdev_del(&drv_dev.cdev);
        unregister_chrdev_region(drv_dev.dev_num, 1);
        return PTR_ERR(driver_class);
    }
    
    drv_dev.device = device_create(driver_class, NULL, drv_dev.dev_num, NULL, DRIVER_NAME);
    
    if (debug) {
        printk(KERN_DEBUG "Debug mode enabled\n");
    }
    
    printk(KERN_INFO "[%s] Driver loaded (major=%d)\n", DRIVER_NAME, MAJOR(drv_dev.dev_num));
    
    return 0;
}

static void __exit driver_exit(void)
{
    cdev_del(&drv_dev.cdev);
    unregister_chrdev_region(drv_dev.dev_num, 1);
    
    printk(KERN_INFO "[%s] Driver unloaded\n", DRIVER_NAME);
}

module_init(driver_init);
module_exit(driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Example packaged driver module");
MODULE_VERSION(DRIVER_VERSION);
```

## Important Notes

1. **Module parameters** documented with MODULE_PARM_DESC
2. **Version control** via MODULE_VERSION for compatibility tracking
3. **Udev rules** enable automatic /dev node creation
4. **Package managers** simplify distribution and updates
5. **Documentation** essential for users (README, man pages)
6. **License clarity** GPL or other open source license required
