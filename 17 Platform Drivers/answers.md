# 17 Platform Drivers - Assignment Answers and Tasks

## Task 1: Theory Questions

1. **What is a platform driver?**
   A driver for devices that are integrated into the System-on-Chip (SoC) or board, not on discoverable buses like PCI/USB. They use a platform_driver structure and lifecycle callbacks (probe/remove) to manage devices registered via platform_device.

2. **How does name matching work?**
   The kernel matches platform_device and platform_driver by comparing their name fields. When a device's name matches a driver's name, the probe() callback is invoked. Multiple devices can share one driver if they have matching names.

3. **What are platform resources?**
   Hardware resources allocated to a platform device, defined in platform_device.resource array. Common resources include IORESOURCE_MEM (memory-mapped registers), IORESOURCE_IRQ (interrupts), and IORESOURCE_IO (I/O ports). Retrieved using platform_get_resource().

   **Example - Defining resources in board code:**

   ```c
   static struct resource sensor_resources[] = {
       [0] = {
           .start = 0x80000000,        // Physical address start
           .end   = 0x80000FFF,        // Physical address end
           .flags = IORESOURCE_MEM,    // Memory-mapped resource
           .name  = "sensor_mem"
       },
       [1] = {
           .start = 32,                // IRQ number
           .end   = 32,
           .flags = IORESOURCE_IRQ,    // Interrupt resource
           .name  = "sensor_irq"
       }
   };

   static struct platform_device sensor_device = {
       .name = "sensor_driver",
       .id = 0,
       .resource = sensor_resources,
       .num_resources = ARRAY_SIZE(sensor_resources)
   };
   ```

   **Example - Accessing resources in driver probe():**

   ```c
   static int sensor_probe(struct platform_device *pdev)
   {
       struct resource *mem_res, *irq_res;

       // Get memory resource
       mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
       if (!mem_res)
           return -ENOENT;

       // Get IRQ resource
       irq_res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
       if (!irq_res)
           return -ENOENT;

       pr_info("Memory: 0x%x - 0x%x\n", mem_res->start, mem_res->end);
       pr_info("IRQ: %d\n", irq_res->start);

       return 0;
   }
   ```

4. **What does ioremap() do?**
   Maps physical memory addresses to kernel virtual addresses for safe access. Required because kernel code cannot directly access physical addresses. Returns a virtual pointer that driver code uses to read/write hardware registers.

5. **When is probe() called?**
   Called automatically by the kernel when a platform_device is registered and its name matches a registered platform_driver's name. Also called during driver registration if matching devices already exist. This is where hardware initialization and character device setup occurs.

## Task 2: Implement a Platform Driver

Create a platform driver for a simulated sensor device that:

1. Reads from a memory-mapped register at base+0x00 (temperature)
2. Writes control commands to base+0x04

**Key Requirements:**

- Use platform_get_resource() to access hardware resources
- Use ioremap() to map physical to virtual addresses
- Handle probe() and remove() lifecycle

## Task 3: Register Device from Board Code

Create board initialization code that registers the platform device
