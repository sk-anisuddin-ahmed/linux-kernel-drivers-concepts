# 18 Device Trees - Assignment Answers and Tasks

## Task 1: Theory Questions

1. **What is a device tree?**
   A device tree is a data structure used to describe the hardware layout and configuration of a system in a platform-independent way. It's a text file (typically with .dts extension) that defines devices, their properties, memory regions, interrupts, and how they're connected to the system. The device tree is compiled into a binary format (.dtb) at build time and passed to the kernel at boot. This allows the same kernel binary to support multiple hardware configurations without recompilation.

2. **What are compatible strings?**
   Compatible strings are identifiers in device tree nodes that specify which device driver should handle a particular hardware device. They follow a hierarchical format: "vendor,device-name" (e.g., "ti,am3358-gpio", "arm,pl011"). The kernel uses compatible strings to match devices in the device tree with appropriate device drivers. Multiple compatible strings can be listed in priority order, with the first being the most specific and later ones serving as fallbacks.

3. **How does OF matching work?**
   OF (Open Firmware) matching is the mechanism by which the kernel matches device tree nodes to device drivers. When a device is discovered in the device tree, the kernel compares the device's compatible string(s) against the `of_device_id` table in each driver. The first driver with a matching entry in its table is selected to handle that device. The kernel traverses the compatible strings list and stops at the first match, allowing for flexible fallback behavior if a specific driver is unavailable.

4. **What does MODULE_DEVICE_TABLE(of, table) do?**
   `MODULE_DEVICE_TABLE(of, table)` is a kernel macro that extracts device identification information from the `of_device_id` table and stores it in the module's ELF section. This metadata allows modprobe and other kernel tools to automatically determine which devices a module can handle. When a device is hot-plugged or when the system searches for drivers, it can use this table to automatically load the appropriate driver module without manual intervention.

5. **How do you read properties?**
   Device tree properties are read using the `of_property_read_*()` family of functions provided by the kernel's OF (Open Firmware) API. Common functions include:
   - `of_property_read_u32()` - reads 32-bit unsigned integer properties
   - `of_property_read_string()` - reads string properties
   - `of_property_read_u32_array()` - reads arrays of 32-bit values
   - `of_get_property()` - gets raw property data with length
   - `of_property_count_strings()` - counts string array elements

   These functions take the device node pointer, property name, and output buffer as parameters, returning error codes if the property doesn't exist or is invalid.

## Task 2: Parse Custom Properties

Extend a driver to read multiple properties from device tree

**Key Requirements:**

- Read u32, string, and other property types from device tree nodes
- Use of_property_read_* functions
- Handle missing or invalid properties gracefully

**Solution**
- https://github.com/sk-anisuddin-ahmed/Linux-Kernel-LDD-Program-Examples/tree/main/018_platform_sensor_data_dvctree_of_read

## Task 3: Handle GPIO from Device Tree

Write code that reads GPIO pins from DT and controls them

**Solution**
- https://github.com/sk-anisuddin-ahmed/Linux-Kernel-LDD-Program-Examples/tree/main/019_platform_gpio_dvctree
