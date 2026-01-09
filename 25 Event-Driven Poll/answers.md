# 25 ioctl Device Commands - Assignment Answers and Tasks

## Task 1: Theory Check

1. **What is ioctl and when would you use it instead of read/write?**
   - ioctl is a system call for sending commands to a device driver. Use it instead of read/write when you need to send control signals or configuration commands that don't involve bulk data transfer, like setting a GPIO level, resetting a device, or querying status.

2. **How do the macros _IO, _IOR, _IOW, and _IOWR differ?**
   - `_IO`: Command with no data transfer
   - `_IOR`: Read from device (kernel → user space)
   - `_IOW`: Write to device (user space → kernel)
   - `_IOWR`: Bidirectional (both directions)

3. **What is the "magic number" in an ioctl command?**
   - A unique identifier (8-bit value, typically a single letter) that groups related commands and prevents conflicts between different drivers. Example: 'T' for temperature, 'S' for serial.

4. **Why must you use copy_from_user() and copy_to_user() in ioctl?**
   - Kernel code cannot directly access user-space pointers (different memory contexts). These functions safely copy data between user and kernel space with proper error handling.

5. **What should you return on an invalid ioctl command?**
   - Return `-EINVAL` to signal that the command is invalid or not supported by this driver.

## Task 2: Add More Commands

Extend the temperature driver to include:

1. Command to get the threshold (READ)
2. Command to enable/disable alerts (WRITE boolean flag)
3. Command to get alert status (READ)

**Key Requirements:**

- Define 3 new ioctl commands using appropriate macros
- Implement handlers in the ioctl function
- Use a static variable to track alert enable/disable

**Sample Solution:**

```c
// temp_sensor.h (updated)
#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <linux/ioctl.h>

#define DEVICE_MAGIC 'T'

#define IOCTL_SET_THRESHOLD   _IOW(DEVICE_MAGIC, 1, int)
#define IOCTL_GET_TEMP        _IOR(DEVICE_MAGIC, 2, int)
#define IOCTL_RESET           _IO(DEVICE_MAGIC, 3)
#define IOCTL_GET_THRESHOLD   _IOR(DEVICE_MAGIC, 4, int)    // NEW
#define IOCTL_SET_ALERTS      _IOW(DEVICE_MAGIC, 5, int)    // NEW
#define IOCTL_GET_ALERTS      _IOR(DEVICE_MAGIC, 6, int)    // NEW

#endif
```

```c
// temp_driver.c (updated sections)

static int current_temperature = 25;
static int threshold = 30;
static int alerts_enabled = 1;  // NEW: track alert state

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
        break;

    case IOCTL_RESET:
        current_temperature = 25;
        threshold = 30;
        alerts_enabled = 1;
        printk(KERN_INFO "Device reset to defaults\n");
        break;

    case IOCTL_GET_THRESHOLD:  // NEW
        if (copy_to_user((int __user *)arg, &threshold, sizeof(int)))
            return -EFAULT;
        printk(KERN_INFO "Returning threshold: %d°C\n", threshold);
        break;

    case IOCTL_SET_ALERTS:  // NEW
        if (copy_from_user(&value, (int __user *)arg, sizeof(int)))
            return -EFAULT;
        alerts_enabled = (value != 0) ? 1 : 0;
        printk(KERN_INFO "Alerts %s\n", alerts_enabled ? "ENABLED" : "DISABLED");
        break;

    case IOCTL_GET_ALERTS:  // NEW
        if (copy_to_user((int __user *)arg, &alerts_enabled, sizeof(int)))
            return -EFAULT;
        printk(KERN_INFO "Alert status: %s\n", alerts_enabled ? "ENABLED" : "DISABLED");
        break;

    default:
        printk(KERN_WARNING "Unknown ioctl: 0x%X\n", cmd);
        return -EINVAL;
    }

    return 0;
}
```

## Task 3: Write a Complete ioctl Test Application

Create a user-space C program that:

1. Opens `/dev/tempsensor`
2. Sends commands to set threshold, get temperature, enable alerts
3. Reads the temperature via read() and via ioctl()
4. Displays all results

**Sample Solution:**

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include "temp_sensor.h"

int main() {
    int fd;
    int temp, threshold, alerts;
    char buffer[100];

    // Open device
    fd = open("/dev/tempsensor", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/tempsensor");
        return 1;
    }

    printf("=== Temperature Sensor ioctl Test ===\n\n");

    // 1. Set threshold to 28
    printf("Setting threshold to 28°C...\n");
    threshold = 28;
    if (ioctl(fd, IOCTL_SET_THRESHOLD, &threshold) < 0) {
        perror("ioctl SET_THRESHOLD failed");
        close(fd);
        return 1;
    }
    printf("✓ Threshold set\n\n");

    // 2. Get temperature via ioctl
    printf("Getting temperature via ioctl...\n");
    if (ioctl(fd, IOCTL_GET_TEMP, &temp) < 0) {
        perror("ioctl GET_TEMP failed");
        close(fd);
        return 1;
    }
    printf("✓ Current temperature: %d°C\n\n", temp);

    // 3. Get temperature via read()
    printf("Getting temperature via read()...\n");
    if (read(fd, buffer, sizeof(buffer)) < 0) {
        perror("read failed");
        close(fd);
        return 1;
    }
    printf("✓ Read output: %s\n", buffer);

    // 4. Get threshold via ioctl
    printf("Getting threshold...\n");
    if (ioctl(fd, IOCTL_GET_THRESHOLD, &threshold) < 0) {
        perror("ioctl GET_THRESHOLD failed");
        close(fd);
        return 1;
    }
    printf("✓ Threshold: %d°C\n\n", threshold);

    // 5. Check alert status
    printf("Checking alert status...\n");
    if (ioctl(fd, IOCTL_GET_ALERTS, &alerts) < 0) {
        perror("ioctl GET_ALERTS failed");
        close(fd);
        return 1;
    }
    printf("✓ Alerts: %s\n\n", alerts ? "ENABLED" : "DISABLED");

    // 6. Disable alerts
    printf("Disabling alerts...\n");
    alerts = 0;
    if (ioctl(fd, IOCTL_SET_ALERTS, &alerts) < 0) {
        perror("ioctl SET_ALERTS failed");
        close(fd);
        return 1;
    }
    printf("✓ Alerts disabled\n\n");

    // 7. Verify alerts are disabled
    printf("Verifying alerts status...\n");
    if (ioctl(fd, IOCTL_GET_ALERTS, &alerts) < 0) {
        perror("ioctl GET_ALERTS failed");
        close(fd);
        return 1;
    }
    printf("✓ Alerts: %s\n\n", alerts ? "ENABLED" : "DISABLED");

    close(fd);
    printf("=== Test Completed Successfully ===\n");

    return 0;
}
```

**Compile and run:**

```bash
gcc -o ioctl_test ioctl_test.c
./ioctl_test
```

**Expected output:**

```
=== Temperature Sensor ioctl Test ===

Setting threshold to 28°C...
✓ Threshold set

Getting temperature via ioctl...
✓ Current temperature: 25°C

Getting temperature via read()...
✓ Read output: Temperature: 25°C

Getting threshold...
✓ Threshold: 28°C

Checking alert status...
✓ Alerts: ENABLED

Disabling alerts...
✓ Alerts disabled

Verifying alerts status...
✓ Alerts: DISABLED

=== Test Completed Successfully ===
```

This demonstrates the full power of ioctl for device control without bulk data transfer.
