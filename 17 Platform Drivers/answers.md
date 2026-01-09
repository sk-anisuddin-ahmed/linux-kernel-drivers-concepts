# 17 Platform Drivers - Assignment Answers and Tasks

## Task 1: Theory Questions

1. **What is a platform driver?** A driver that binds to devices registered via platform device registration, typically used for SoC-integrated hardware.

2. **How does name matching work?** Device and driver names must match exactly. When a device with name "my_device" is registered, the kernel finds a driver with driver.name = "my_device" and calls probe().

3. **What are platform resources?** Structs describing memory, I/O ports, IRQs, and DMA channels needed by the device. Retrieved using platform_get_resource().

4. **What does ioremap() do?** Maps physical memory addresses (used by hardware) to virtual kernel addresses so the driver can read/write device registers.

5. **When is probe() called?** When a platform device is registered and a matching platform driver is loaded (or vice versa).

## Task 2: Implement a Platform Driver

Create a platform driver for a simulated sensor device that:

1. Reads from a memory-mapped register at base+0x00 (temperature)
2. Writes control commands to base+0x04

**Sample Solution:**

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/device.h>

#define SENSOR_TEMP_REG 0x00
#define SENSOR_CTRL_REG 0x04

static struct device *sensor_dev = NULL;

static int sensor_probe(struct platform_device *pdev)
{
    struct resource *res;
    void __iomem *base;
    u32 temp;

    printk(KERN_INFO "[sensor] Probing %s (id=%d)\n", pdev->name, pdev->id);

    // Get memory resource
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        printk(KERN_ERR "No memory resource\n");
        return -ENOENT;
    }

    printk(KERN_INFO "Memory: 0x%X - 0x%X\n", res->start, res->end);

    // Map to kernel virtual address
    base = ioremap(res->start, resource_size(res));
    if (!base) {
        return -ENOMEM;
    }

    // Read temperature register
    temp = readl(base + SENSOR_TEMP_REG);
    printk(KERN_INFO "Temperature: %u C\n", temp);

    // Store in device private data
    platform_set_drvdata(pdev, base);
    
    sensor_dev = &pdev->dev;

    iounmap(base);
    return 0;
}

static int sensor_remove(struct platform_device *pdev)
{
    void __iomem *base = platform_get_drvdata(pdev);
    
    printk(KERN_INFO "[sensor] Removing %s\n", pdev->name);
    
    if (base)
        iounmap(base);
    
    return 0;
}

static struct platform_driver sensor_driver = {
    .probe = sensor_probe,
    .remove = sensor_remove,
    .driver = {
        .name = "my_sensor",
        .owner = THIS_MODULE,
    },
};

module_platform_driver(sensor_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Platform sensor driver");
```

## Task 3: Register Device from Board Code

Create board initialization code that registers the platform device:

```c
// In your board setup code or as a separate module:

static struct resource sensor_resources[] = {
    {
        .start = 0x10000000,
        .end = 0x10000FFF,
        .flags = IORESOURCE_MEM,
        .name = "sensor_mem",
    },
    {
        .start = 10,
        .end = 10,
        .flags = IORESOURCE_IRQ,
        .name = "sensor_irq",
    },
};

static struct platform_device sensor_device = {
    .name = "my_sensor",
    .id = 0,
    .num_resources = ARRAY_SIZE(sensor_resources),
    .resource = sensor_resources,
};

// In board init:
platform_device_register(&sensor_device);

// Later, when driver loads:
// Kernel calls sensor_probe() automatically
```

**When driver loads:**

```bash
dmesg
# [sensor] Probing my_sensor (id=0)
# Memory: 0x10000000 - 0x10000FFF
# Temperature: 25 C
# [sensor] Removing my_sensor
```
