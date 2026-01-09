# Packaging and Auto-Loading Your Driver (MODALIAS + Udev + Distribution)

By the end of this, you'll:

- Understand the **MODALIAS mechanism** that triggers driver auto-load
- Use **`MODULE_ALIAS()`** to register your driver for hotplug
- Write **udev rules** to create persistent `/dev/gpiobtnX` devices
- Organize your work into a **sharable, reusable driver package**

This is how real vendors distribute their GPIO, SPI, I2C, or sensor drivers — clean, auto-loading, and user-friendly.

## What Is MODALIAS and How Does It Work?

The kernel maintains a file: `/lib/modules/$(uname -r)/modules.alias`

This maps device identifiers (like from Device Tree) to kernel modules.

When the kernel encounters a **platform device**, it:

1. Exports a **MODALIAS string**
2. `udev` or `modprobe` checks `modules.alias`
3. Loads the matching `.ko` driver **automatically**

To support this in your driver, you use: `MODULE_ALIAS("platform:gpiobtn");`

This line **registers your module** with the alias `platform:gpiobtn`

So when the device tree creates a node named `gpiobtn`, your module is loaded.

## Step-by-Step: Enable Auto-Loading with MODALIAS

### 1. Add This to Your Driver Code

```c
MODULE_ALIAS("platform:gpiobtn");
```

This should match the `.name` field in your `platform_driver`:

```c
.driver = {
    .name = "gpiobtn",
    ...
}
```

### 2. Rebuild and Install Your Module

```bash
make
sudo cp gpiobtn.ko /lib/modules/$(uname -r)/kernel/drivers/mydrivers/
sudo depmod
```

`depmod` regenerates `modules.alias`

### 3. Verify Alias

```bash
modinfo gpiobtn.ko
```

You'll see:

```
alias:          platform:gpiobtn
```

Now if your device is created via Device Tree with:

```
gpiobtn@0 {
    compatible = "myvendor,gpiobtn";
    ...
};
```

…Linux will probe it, emit a MODALIAS event (`platform:gpiobtn`), and `modprobe` will auto-load your driver.

## Writing a udev Rule for Clean /dev Naming

By default, the system creates: `/dev/gpiobtn`

To ensure predictable naming (like `/dev/button0`, `/dev/button1`), add a **udev rule**.

### 1. Create Udev Rule

**Create a file:** `sudo nano /etc/udev/rules.d/99-gpiobtn.rules`

**Add:**

```
SUBSYSTEM=="misc", KERNEL=="gpiobtn*", SYMLINK+="button%n"
```

**This creates a symlink:** `/dev/button0` → `/dev/gpiobtn0`

You can also filter by vendor, device name, or Device Tree properties if needed.

### 2. Reload Udev

```bash
sudo udevadm control --reload
sudo udevadm trigger
```

**Now test:** `ls -l /dev/button*`

**Expected:** `lrwxrwxrwx 1 root root ... /dev/button0 → /dev/gpiobtn0`

## Packaging Your Driver

Here's a **recommended structure** for distributing your driver:

```
gpiobtn_driver/
├── Makefile
├── gpiobtn.c
├── gpiobtn-overlay.dts
├── gpiobtn.dtbo
├── udev/
│   └── 99-gpiobtn.rules
├── install.sh
├── README.md
```

### README.md Should Include

- Compatible boards (Raspberry Pi, BeagleBone, etc.)
- GPIO pin numbers
- How to build: `make`
- **How to install:** `sudo ./install.sh`
- **How to test:** `cat /dev/gpiobtn`, `poll`, `ioctl`, etc.

## Summary Table

| Concept | You Learned |
|---------|------------|
| `MODULE_ALIAS()` | Registers MODALIAS for auto-load |
| `depmod` | Updates modules.alias |
| `modinfo` | Displays module information including aliases |
| `udev rules` | Auto-symlink or rename device nodes |
| `/dev/button0` | Created via udev symlink |
| Driver packaging | Share and deploy with clean layout |

## Real-World Example: Complete Deployment

### Step 1: Your Driver (gpiobtn.c)

```c
#include <linux/module.h>
#include <linux/platform_device.h>

static int gpiobtn_probe(struct platform_device *pdev)
{
    printk(KERN_INFO "GPIO Button driver probed\n");
    return 0;
}

static int gpiobtn_remove(struct platform_device *pdev)
{
    printk(KERN_INFO "GPIO Button driver removed\n");
    return 0;
}

static struct platform_driver gpiobtn_driver = {
    .probe = gpiobtn_probe,
    .remove = gpiobtn_remove,
    .driver = {
        .name = "gpiobtn",
        .owner = THIS_MODULE,
    }
};

module_platform_driver(gpiobtn_driver);

MODULE_ALIAS("platform:gpiobtn");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("GPIO Button Platform Driver");
```

### Step 2: Device Tree Overlay (gpiobtn-overlay.dts)

```
/dts-v1/;
/plugin/;

/ {
    fragment@0 {
        target-path = "/";
        __overlay__ {
            gpiobtn@0 {
                compatible = "myvendor,gpiobtn";
                status = "okay";
            };
        };
    };
};
```

### Step 3: Udev Rule (udev/99-gpiobtn.rules)

```
SUBSYSTEM=="misc", KERNEL=="gpiobtn*", SYMLINK+="button%n"
```

### Step 4: Install Script (install.sh)

```bash
#!/bin/bash

# Build
make

# Install module
sudo cp gpiobtn.ko /lib/modules/$(uname -r)/kernel/drivers/mydrivers/
sudo depmod

# Install udev rule
sudo cp udev/99-gpiobtn.rules /etc/udev/rules.d/

# Reload udev
sudo udevadm control --reload
sudo udevadm trigger

# Compile and apply device tree overlay
dtc -@ -I dts -O dtb -o gpiobtn.dtbo gpiobtn-overlay.dts
sudo cp gpiobtn.dtbo /boot/overlays/

echo "Installation complete!"
echo "Please reboot or load the overlay manually:"
echo "sudo dtoverlay gpiobtn"
```

### Step 5: Testing Auto-Load

```bash
# After installation and reboot/overlay load
lsmod | grep gpiobtn
# Should show: gpiobtn    12345  0

# Check device file
ls -l /dev/button*
# Should show symlink to /dev/gpiobtn

# Test the driver
cat /dev/gpiobtn
# Or use poll/select depending on driver implementation
```

## You've now created a self-contained, plug-and-play Linux driver package suitable for

- Raspberry Pi
- BeagleBone
- Yocto-based builds
- Embedded distribution

## Key Deployment Concepts

| Component | Purpose | Location |
|-----------|---------|----------|
| `.ko` module | Kernel driver binary | `/lib/modules/[kernel]/kernel/drivers/` |
| `modules.alias` | MODALIAS mappings | `/lib/modules/[kernel]/` (generated by depmod) |
| `udev rules` | Device node creation | `/etc/udev/rules.d/` |
| Device tree overlay | Hardware description | `/boot/overlays/` (Raspberry Pi) |
| Installation script | Automated setup | `install.sh` in package |

---

## Next Steps for Distribution

- **Version control**: Use git for driver code and overlays
- **Build system**: Create a Makefile for end-users
- **Documentation**: Include troubleshooting guides
- **Testing scripts**: Automate device availability checks
- **Repository**: Upload to GitHub for community drivers
