# Section 29: Packaging Drivers – Assignment

## Task 1: Theory Questions

1. **Makefile targets**: all (build), install (copy to /lib/modules), clean (remove objects), uninstall (delete installed module).

2. **Module parameters**: Declared with module_param(), documented with MODULE_PARM_DESC(). Accessible via /sys/module/name/parameters/.

3. **Udev rules**: Match subsystem/device, create /dev nodes with specified permissions. Triggered on device add/remove.

4. **.deb packages**: Contain control metadata, rules, scripts for installation/removal. Managed by apt package manager.

5. **Distribution metadata**: Version, author, description, license enable proper cataloging and compatibility tracking.

## Task 2: Installable Driver Module

Create drivers with build/install/uninstall targets:

```makefile
obj-m += my_device.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
 make -C $(KERNEL_DIR) M=$(PWD) modules

install: all
 make -C $(KERNEL_DIR) M=$(PWD) modules_install
 depmod -A
 @echo "Module installed. Load with: modprobe my_device"

uninstall:
 rm -f /lib/modules/$(shell uname -r)/kernel/drivers/my_device.ko
 depmod -A
 @echo "Module uninstalled"

clean:
 make -C $(KERNEL_DIR) M=$(PWD) clean

.PHONY: all install uninstall clean
```

**Usage**: `make all`, `sudo make install`, `sudo modprobe my_device`

## Task 3: Udev Rules for Device Creation

```bash
# /etc/udev/rules.d/99-my-driver.rules

# Create /dev/mydevice
KERNEL=="my_device", MODE="0666", NAME="mydevice"

# With group ownership
KERNEL=="my_device", MODE="0660", OWNER="root", GROUP="dialout"
```

**Install**: `sudo cp 99-my-driver.rules /etc/udev/rules.d/`

## Task 4: Versioned Driver Module

```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>

#define DRIVER_VERSION "2.1.0"
#define DRIVER_NAME "pkg_driver"

static int debug = 0;
static int max_devices = 4;

module_param(debug, int, 0644);
module_param(max_devices, int, 0644);

MODULE_PARM_DESC(debug, "Enable debug (0=off, 1=on)");
MODULE_PARM_DESC(max_devices, "Max devices (default: 4)");

static int driver_open(struct inode *inode, struct file *file)
{
    if (debug) {
        printk(KERN_DEBUG "[%s] Opened\n", DRIVER_NAME);
    }
    return 0;
}

static const struct file_operations driver_fops = {
    .owner = THIS_MODULE,
    .open = driver_open,
};

static int __init driver_init(void)
{
    printk(KERN_INFO "[%s] v%s (debug=%d, max=%d)\n",
           DRIVER_NAME, DRIVER_VERSION, debug, max_devices);
    return 0;
}

static void __exit driver_exit(void)
{
    printk(KERN_INFO "[%s] unloaded\n", DRIVER_NAME);
}

module_init(driver_init);
module_exit(driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Packaged driver module");
MODULE_VERSION(DRIVER_VERSION);
```

**Usage**: `sudo modprobe pkg_driver debug=1 max_devices=8`
