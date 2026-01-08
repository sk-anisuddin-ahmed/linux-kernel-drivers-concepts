# 18 Device Trees - Assignment Answers and Tasks

## Task 1: Theory Questions

1. **What is a device tree?** A declarative format (DT blob) that describes hardware topology, resources, and properties. Bootloader passes it to kernel.

2. **What are compatible strings?** Vendor,device format identifiers that drivers use to match devices in the tree (e.g., "arm,pl110").

3. **How does OF matching work?** When a DT node is instantiated, its "compatible" property is checked against all driver.of_match_table entries. On match, probe() is called.

4. **What does MODULE_DEVICE_TABLE(of, table) do?** Extracts the OF match table so module tools can auto-load the driver when compatible hardware is detected.

5. **How do you read properties?** Use of_property_read_* functions like of_property_read_u32() passing the device node, property name, and output pointer.

## Task 2: Parse Custom Properties

Extend a driver to read multiple properties from device tree:

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>

static int sensor_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    u32 poll_interval, max_temp;
    const char *sensor_name;
    int ret;

    printk(KERN_INFO "Probing sensor from DT\n");

    // Read poll-interval (u32)
    ret = of_property_read_u32(np, "poll-interval", &poll_interval);
    if (ret == 0) {
        printk(KERN_INFO "Poll interval: %u ms\n", poll_interval);
    }

    // Read max-temperature (u32)
    ret = of_property_read_u32(np, "max-temperature", &max_temp);
    if (ret == 0) {
        printk(KERN_INFO "Max temperature: %u C\n", max_temp);
    }

    // Read sensor-name (string)
    ret = of_property_read_string(np, "sensor-name", &sensor_name);
    if (ret == 0) {
        printk(KERN_INFO "Sensor name: %s\n", sensor_name);
    }

    return 0;
}

static const struct of_device_id sensor_of_match[] = {
    {.compatible = "mycompany,temp-sensor"},
    {.compatible = "mycompany,humidity-sensor"},
    {},
};
MODULE_DEVICE_TABLE(of, sensor_of_match);

static struct platform_driver sensor_driver = {
    .probe = sensor_probe,
    .driver = {
        .name = "dt_sensor",
        .of_match_table = sensor_of_match,
    },
};

module_platform_driver(sensor_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("DT-based sensor driver");
```

**Device tree:**

```
my_sensor@10000000 {
    compatible = "mycompany,temp-sensor";
    reg = <0x10000000 0x1000>;
    poll-interval = <1000>;
    max-temperature = <50>;
    sensor-name = "Kitchen Temperature";
};
```

## Task 3: Handle GPIO from Device Tree

Write code that reads GPIO pins from DT and controls them:

```c
#include <linux/gpio.h>
#include <linux/of_gpio.h>

static int gpio_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    int gpio_pin, ret;

    // Get GPIO pin from "control-gpio" property
    gpio_pin = of_get_named_gpio(np, "control-gpio", 0);
    if (gpio_pin < 0) {
        printk(KERN_ERR "No control-gpio in DT\n");
        return gpio_pin;
    }

    printk(KERN_INFO "Got GPIO pin: %d\n", gpio_pin);

    // Request GPIO
    ret = gpio_request(gpio_pin, "sensor_control");
    if (ret < 0) {
        printk(KERN_ERR "Failed to request GPIO\n");
        return ret;
    }

    // Set as output
    gpio_direction_output(gpio_pin, 1);

    // Store for later use
    platform_set_drvdata(pdev, (void *)(long)gpio_pin);

    printk(KERN_INFO "GPIO setup complete\n");
    return 0;
}

static int gpio_remove(struct platform_device *pdev)
{
    int gpio_pin = (int)(long)platform_get_drvdata(pdev);
    gpio_free(gpio_pin);
    return 0;
}

// ... rest of driver ...
```

**Device tree:**

```
my_device@10000000 {
    compatible = "mycompany,gpio-device";
    reg = <0x10000000 0x1000>;
    control-gpio = <&gpio_controller 5 0>;
};
```
