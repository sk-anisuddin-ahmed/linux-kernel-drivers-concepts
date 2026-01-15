# Section 29: Packaging and Auto-Loading � Assignment

## Task 1: Quiz

**1. What does depmod do?**
depmod analyzes all installed kernel modules and builds the modules.dep file, which maps module dependencies. Run after installing new modules to update the dependency database.

**2. What must MODULE_ALIAS() match to work?**
MODULE_ALIAS() must match the MODALIAS string that udev sends. For platform devices, it's "platform:device_name". For USB, it's "usb:vXXXXpXXXX". Must match exactly.

**3. Where does the kernel store MODALIAS info?**
MODALIAS is stored in /sys/devices/*/modalias. When udev detects a device, it reads this file and attempts to load matching modules using modprobe.

**4. What is the purpose of a udev rule?**
Udev rules specify how to handle device hotplug events (e.g., create /dev files, run scripts, change permissions). Rules trigger modprobe to load drivers matching the device's MODALIAS.

**5. How do you debug if auto-loading doesn't work?**
Check: (1) MODULE_ALIAS matches device MODALIAS, (2) depmod was run, (3) udev rules exist and match, (4) dmesg for errors, (5) udevadm test to simulate udev processing.

## Task 2: Add Uninstall Script

Log Session uninstall.sh:

**uninstall.sh:**
```bash
#!/bin/bash
rm /lib/modules/$(uname -r)/kernel/drivers/mydrivers/gpiobtn.ko
depmod
```

## Task 3: Detect Auto-Loading

Remove the driver: rmmod gpiobtn

Reboot

Ensure your device tree node exists

Verify gpiobtn is loaded automatically via lsmod or dmesg

**Verification steps:**
```bash
lsmod | grep gpiobtn
dmesg | grep gpiobtn
ls -la /dev/gpiobtn
modinfo gpiobtn | grep alias
```