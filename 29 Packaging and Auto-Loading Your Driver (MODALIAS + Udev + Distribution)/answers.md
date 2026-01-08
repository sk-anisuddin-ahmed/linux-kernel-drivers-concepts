# Packaging and Auto-Loading Your Driver – Q&A and Assignments

## Assignments

### 1. Quiz

Answer the following questions:

1. **What does `depmod` do?**
   - Rebuilds the kernel module dependency database and regenerates `/lib/modules/[kernel]/modules.alias` mapping MODALIAS strings to module names

2. **What must `MODULE_ALIAS()` match to work?**
   - It must match the `.name` field in the `platform_driver` structure and the device name that appears in the Device Tree

3. **Where does the kernel store MODALIAS info?**
   - `/lib/modules/$(uname -r)/modules.alias` (user-readable mapping) and internally in the device properties exported by the kernel

4. **What is the purpose of a `udev` rule?**
   - To automatically create device files, symlinks, or set permissions when a device is detected; enables predictable `/dev` naming

5. **How do you debug if auto-loading doesn't work?**
   - Check `dmesg` for probe failures, verify MODALIAS with `modinfo gpiobtn.ko`, confirm `modules.alias` with `grep gpiobtn /lib/modules/$(uname -r)/modules.alias`, use `udevadm monitor` to watch udev events

---

### 2. Add Uninstall Script

**Task**: Create `uninstall.sh` to cleanly remove the driver package.

**Requirements**:

- Remove the `.ko` module from `/lib/modules/`
- Run `depmod` to update the module database
- Remove the udev rule
- Reload udev
- Remove device tree overlay (if applicable)
- Print status messages

**Starter Code**:

```bash
#!/bin/bash

# Uninstall module
echo "Removing kernel module..."
sudo rm /lib/modules/$(uname -r)/kernel/drivers/mydrivers/gpiobtn.ko
sudo depmod

# Remove udev rule
echo "Removing udev rule..."
sudo rm /etc/udev/rules.d/99-gpiobtn.rules

# Reload udev
echo "Reloading udev..."
sudo udevadm control --reload
sudo udevadm trigger

# TODO: Remove device tree overlay

echo "Uninstallation complete!"
echo "Run 'lsmod | grep gpiobtn' - should return nothing"
```

**Expected Output After Running**:

```bash
$ sudo ./uninstall.sh
Removing kernel module...
Removing udev rule...
Reloading udev...
Uninstallation complete!
Run 'lsmod | grep gpiobtn' - should return nothing

$ lsmod | grep gpiobtn
# (no output, driver removed successfully)
```

**Bonus**: Add error checking to verify the module was actually removed:

```bash
if [ -f "/lib/modules/$(uname -r)/kernel/drivers/mydrivers/gpiobtn.ko" ]; then
    echo "ERROR: Module still exists!"
    exit 1
fi
```

---

### 3. Detect Auto-Loading

**Task**: Verify that your driver auto-loads when the device tree node is present.

**Steps**:

1. **Remove the driver manually:**

   ```bash
   rmmod gpiobtn
   ```

2. **Reboot the system:**

   ```bash
   sudo reboot
   ```

3. **Ensure your device tree node exists** (either via overlay or compiled into kernel):

   ```bash
   # List device tree nodes
   dtc -I fs /sys/firmware/devicetree/base > dt.txt
   grep -A5 "gpiobtn" dt.txt
   ```

4. **Verify `gpiobtn` is loaded automatically** via `lsmod` or `dmesg`:

   ```bash
   # Check loaded modules
   lsmod | grep gpiobtn
   # Should show: gpiobtn    [size]  0
   
   # Check kernel messages
   dmesg | grep -i gpiobtn
   # Should show: GPIO Button driver probed
   ```

**Success Criteria**:

- ✓ `lsmod` shows `gpiobtn` loaded (without manual `insmod`)
- ✓ `dmesg` shows probe message
- ✓ `/dev/gpiobtn` or `/dev/button*` exists
- ✓ No manual module loading required

**Troubleshooting**:

| Problem | Solution |
|---------|----------|
| Module not loaded | Check `dmesg` for probe errors; verify `MODULE_ALIAS()` matches device name |
| Wrong MODALIAS | Run `modinfo gpiobtn.ko \| grep alias` to verify |
| modules.alias not updated | Run `sudo depmod` manually |
| Device node missing | Check udev rule syntax and reload with `sudo udevadm trigger` |

---

### 4. Create a Complete Deployment Package

**Task**: Build a sharable driver package with proper directory structure.

**Your Package Structure**:

```
gpiobtn-driver/
├── src/
│   ├── gpiobtn.c
│   ├── Makefile
│   └── gpiobtn.h
├── overlays/
│   ├── gpiobtn-overlay.dts
│   └── compile_overlay.sh
├── udev/
│   └── 99-gpiobtn.rules
├── scripts/
│   ├── install.sh
│   ├── uninstall.sh
│   └── test.sh
├── README.md
└── CHANGELOG.md
```

**Requirements**:

1. **README.md** should include:
   - Driver name and version
   - Supported platforms (Raspberry Pi, BeagleBone, etc.)
   - Prerequisites (kernel headers, build tools)
   - Installation instructions
   - Quick start testing
   - Troubleshooting section

2. **Makefile** should support:

   ```bash
   make build       # Compile the module
   make install     # Copy files to destination
   make uninstall   # Remove files
   make clean       # Remove build artifacts
   ```

3. **install.sh** should:
   - Build the driver
   - Create `/lib/modules/[kernel]/kernel/drivers/mydrivers/` if missing
   - Install the `.ko` file
   - Install udev rule
   - Compile and install device tree overlay
   - Run `depmod` and `udevadm` commands

4. **test.sh** should verify:

   ```bash
   # Check module loaded
   lsmod | grep gpiobtn
   
   # Check device file exists
   ls -l /dev/gpiobtn* /dev/button*
   
   # Test basic operations (read, write, ioctl)
   # (customize based on your driver)
   ```

**Example install.sh**:

```bash
#!/bin/bash

set -e  # Exit on error

KERNEL_VERSION=$(uname -r)
MODULE_PATH="/lib/modules/$KERNEL_VERSION/kernel/drivers/mydrivers"

echo "Installing GPIO Button Driver..."

# Build
echo "[1/5] Building module..."
make clean
make -C src

# Install module
echo "[2/5] Installing module to $MODULE_PATH..."
sudo mkdir -p $MODULE_PATH
sudo cp src/gpiobtn.ko $MODULE_PATH/

# Update module database
echo "[3/5] Running depmod..."
sudo depmod

# Install udev rule
echo "[4/5] Installing udev rule..."
sudo cp udev/99-gpiobtn.rules /etc/udev/rules.d/

# Install device tree overlay
echo "[5/5] Installing device tree overlay..."
cd overlays
bash compile_overlay.sh
sudo cp gpiobtn.dtbo /boot/overlays/
cd ..

echo "Installation complete!"
echo ""
echo "Next steps:"
echo "1. Apply device tree overlay: sudo dtoverlay gpiobtn"
echo "2. Or reboot if overlay is in /boot/config.txt"
echo "3. Test with: ./scripts/test.sh"
```

**Example test.sh**:

```bash
#!/bin/bash

echo "=== GPIO Button Driver Test ==="
echo ""

echo "[1] Checking if module is loaded..."
if lsmod | grep -q gpiobtn; then
    echo "✓ gpiobtn module is loaded"
else
    echo "✗ gpiobtn module NOT loaded"
    exit 1
fi

echo ""
echo "[2] Checking device file..."
if [ -e /dev/gpiobtn ] || [ -L /dev/button0 ]; then
    echo "✓ Device file exists"
    ls -l /dev/gpiobtn* /dev/button* 2>/dev/null
else
    echo "✗ Device file NOT found"
    exit 1
fi

echo ""
echo "[3] Testing basic read..."
if timeout 1 cat /dev/gpiobtn >/dev/null 2>&1; then
    echo "✓ Device read successful (or timed out as expected)"
else
    echo "✗ Device read failed"
    exit 1
fi

echo ""
echo "=== All tests passed! ==="
```

**Distribution via GitHub**:

```bash
git init
git add .
git commit -m "Initial GPIO Button driver package v1.0"
git remote add origin https://github.com/yourusername/gpiobtn-driver.git
git push -u origin main

# Tag for release
git tag -a v1.0 -m "First stable release"
git push origin v1.0
```

---

## Deployment Checklist

Use this checklist for production deployments:

```
Pre-Deployment:
  ☐ Code reviewed and tested
  ☐ MODALIAS matches device name
  ☐ Udev rule syntax verified
  ☐ Device tree overlay compiled successfully
  ☐ All scripts have execute permissions (chmod +x)
  ☐ Documentation complete and accurate

Deployment:
  ☐ Run install.sh successfully
  ☐ Module appears in lsmod
  ☐ Device file exists with correct permissions
  ☐ Udev rule active (check with udevadm)
  ☐ Auto-loading works after reboot
  ☐ No kernel errors in dmesg

Post-Deployment:
  ☐ Run test.sh to verify functionality
  ☐ Document any platform-specific issues
  ☐ Create version tag in git
  ☐ Update CHANGELOG.md with deployment notes
```

---

## Key Takeaways

| Concept | Key Point |
|---------|-----------|
| `MODULE_ALIAS()` | Enables automatic module loading based on device identification |
| `depmod` | MUST be run after installing modules to update dependency database |
| Udev rules | Enable clean device naming and permissions management |
| Device tree | Modern way to describe hardware to the kernel |
| Package structure | Clear organization aids distribution and maintenance |
| Installation script | Automates complex deployment steps |
| Testing script | Verifies successful installation |

---

## Real-World Deployment Scenarios

### Scenario 1: Embedded Linux Distribution (Yocto/OpenEmbedded)

- Add recipe to `.bb` file
- Automatic installation during build
- Module pre-loaded via package manager

### Scenario 2: Raspberry Pi

- Overlay placed in `/boot/overlays/`
- Activated via `/boot/config.txt`: `dtoverlay=gpiobtn`
- Module auto-loads on boot

### Scenario 3: BeagleBone

- Device tree compiled into kernel
- `modprobe` auto-loads when device appears
- Udev rule manages `/dev` naming

---

## Resources

- **Kernel MODALIAS**: `man modprobe`, `man modules.alias`
- **Udev Rules**: `/etc/udev/rules.d/`, `man udev`
- **Device Tree**: `/boot/dts/` examples, Device Tree Specification
- **Platform Drivers**: Kernel documentation in `Documentation/driver-model/`
