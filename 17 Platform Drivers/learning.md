# 17 Platform Drivers – Device Binding and Data Transfer

## Overview

Real embedded systems have **hardware integrated into the SoC** (System-on-Chip):

- UARTs, SPI/I2C controllers, GPIO blocks, Timer units

These are described in **board data** or **device trees** that specify which hardware exists, resources (memory, interrupts), and parameters.

**Platform drivers** use this information to bind to devices automatically.

By the end of this section, you will:

- Understand platform device/driver model
- Parse platform data and resources
- Create platform drivers
- Understand driver binding

## Platform Device Structure

```c
struct platform_device {
    const char *name;
    int id;
    struct resource *resource;
    void *platform_data;
};
```

Board code registers devices:

```c
static struct resource my_resources[] = {
    {.start = 0x10000000, .end = 0x10000FFF, .flags = IORESOURCE_MEM},
    {.start = 32, .end = 32, .flags = IORESOURCE_IRQ},
};

static struct platform_device my_device = {
    .name = "my_driver",
    .id = 0,
    .num_resources = ARRAY_SIZE(my_resources),
    .resource = my_resources,
};

platform_device_register(&my_device);
```

## Platform Driver Structure

```c
static int my_probe(struct platform_device *pdev)
{
    struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    int irq = platform_get_irq(pdev, 0);
    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver my_driver = {
    .probe = my_probe,
    .remove = my_remove,
    .driver = {.name = "my_driver", .owner = THIS_MODULE},
};

module_platform_driver(my_driver);
```

## Key Functions

- `platform_get_resource(pdev, IORESOURCE_MEM, index)` – Get memory resource
- `platform_get_irq(pdev, index)` – Get IRQ resource
- `ioremap(addr, size)` – Map physical memory to virtual
- `readl(addr)`, `writel(val, addr)` – Read/write memory-mapped I/O

## Important Notes

1. Device and driver names must match exactly
2. Always verify resources exist before using
3. Free all resources in remove()
4. Use ioremap() for memory-mapped I/O
5. Error handling in probe() prevents device loading
