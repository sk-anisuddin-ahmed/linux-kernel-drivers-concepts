# Section 28: Device Tree Overlays – Dynamic Device Trees

## Device Tree Overlay Basics

DT Overlays (.dtbo) add device descriptions at runtime without recompiling kernel DT.

### Overlay Structure

```dts
/dts-v1/;
/plugin/;

/ {
    fragment@0 {
        target = <&i2c1>;
        __overlay__ {
            #address-cells = <1>;
            #size-cells = <0>;
            
            my_sensor: sensor@68 {
                compatible = "vendor,sensor";
                reg = <0x68>;
            };
        };
    };
};
```

### Compilation

```bash
dtc -@ -I dts -O dtb sensor-overlay.dts -o sensor-overlay.dtbo
sudo dtoverlay sensor-overlay
```

## Complete Example: GPIO Button Overlay

```dts
/dts-v1/;
/plugin/;

/ {
    compatible = "raspberry,pi";
    
    fragment@0 {
        target-path = "/";
        __overlay__ {
            gpio_keys {
                compatible = "gpio-keys";
                status = "okay";
                
                key_btn {
                    label = "Button";
                    gpios = <&gpio 27 1>;  // GPIO 27, active-low
                    linux,code = <BTN_0>;
                };
            };
        };
    };
};
```

## C Code for Overlay Support

```c
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/gpio.h>

static struct of_device_id my_match[] = {
    { .compatible = "vendor,my-device" },
    { }
};

MODULE_DEVICE_TABLE(of, my_match);

static int my_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    int gpio;
    
    gpio = of_get_named_gpio(np, "control-gpio", 0);
    if (gpio >= 0) {
        gpio_request(gpio, "control");
        printk(KERN_INFO "Overlay device probed with GPIO %d\n", gpio);
    }
    
    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    printk(KERN_INFO "Overlay device removed\n");
    return 0;
}

static struct platform_driver my_driver = {
    .driver = {
        .name = "my_overlay_device",
        .of_match_table = my_match,
    },
    .probe = my_probe,
    .remove = my_remove,
};

module_platform_driver(my_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Device tree overlay support");
```

## Overlay Loading Methods

### Method 1: bootloader Parameter

```bash
dtoverlay=sensor-overlay
```

### Method 2: Runtime dtoverlay Tool

```bash
sudo dtoverlay sensor-overlay
sudo dtoverlay -r sensor-overlay  # Remove overlay
```

### Method 3: Sysfs Interface

```bash
echo sensor-overlay > /sys/kernel/config/device-tree/overlays/new_overlay
```

## Important Notes

1. **Fragment targets** must match existing nodes or use target-path
2. **Phandles** reference existing nodes (&i2c1, &gpio)
3. **Compatibility** strings must match driver compatible list
4. **Address cells** must match parent node format
5. **Testing overlays** critical before production use
