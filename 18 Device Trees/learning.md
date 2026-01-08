# 18 Device Trees - DT Binding and Compatible Strings

## Overview

Device Trees (DT) describe hardware in a declarative format. Instead of hardcoding device locations in board files, the bootloader passes a DT blob to the kernel describing:

- Which devices exist
- Their memory addresses and interrupts
- Device-specific properties

Drivers match devices using **compatible strings**.

By the end of this section, you will:

- Read device tree syntax
- Write device tree bindings
- Implement OF (Open Firmware) driver matching
- Parse device tree properties

## Device Tree Format

```
/dts-v1/;

/{
    compatible = "myboard";
    
    soc {
        compatible = "simple-bus";
        
        my_sensor@10000000 {
            compatible = "mycompany,my-sensor";
            reg = <0x10000000 0x1000>;
            interrupts = <32>;
            poll-interval = <1000>;
        };
    };
};
```

## OF Driver Structure

```c
static int my_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    u32 poll_interval;
    
    // Parse DT properties
    of_property_read_u32(np, "poll-interval", &poll_interval);
    
    return 0;
}

static const struct of_device_id my_of_match[] = {
    {.compatible = "mycompany,my-sensor"},
    {},
};
MODULE_DEVICE_TABLE(of, my_of_match);

static struct platform_driver my_driver = {
    .probe = my_probe,
    .driver = {
        .name = "my_sensor",
        .of_match_table = my_of_match,
    },
};

module_platform_driver(my_driver);
```

## Key OF Functions

- `of_property_read_u32(node, prop, &value)` – Read integer
- `of_property_read_string(node, prop, &str)` – Read string
- `of_get_named_gpio(node, name, 0)` – Get GPIO number
- `of_parse_phandle(node, prop, 0)` – Follow reference to another node

## Important Notes

1. Compatible string format: "vendor,device"
2. MODULE_DEVICE_TABLE(of, table) enables autoloading
3. Always check return values when reading properties
4. Use of_node pointer from platform_device.dev.of_node
5. Device trees more maintainable than board files
