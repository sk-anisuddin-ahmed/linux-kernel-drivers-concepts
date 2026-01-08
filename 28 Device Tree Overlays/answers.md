# Section 28: Device Tree Overlays – Assignment

## Task 1: Theory Questions

1. **DTO Purpose**: Add device descriptions at runtime without kernel rebuild. Enables module-style device loading.

2. **Fragment Structure**: Multiple fragments target different nodes. Each adds/modifies subtree under target using **overlay**.

3. **Phandle References**: &node_name references existing nodes (i2c1, gpio). Allows connecting to existing buses.

4. **Compilation**: dtc with -@ flag enables overlay generation. Creates .dtbo binary file.

5. **Loading Methods**: bootloader parameter, dtoverlay tool, sysfs interface. Can be loaded/unloaded dynamically.

## Task 2: I2C Sensor Overlay

Create device tree overlay for I2C temperature sensor:

```dts
/dts-v1/;
/plugin/;

/ {
    fragment@0 {
        target = <&i2c1>;
        __overlay__ {
            #address-cells = <1>;
            #size-cells = <0>;
            status = "okay";
            
            temperature_sensor: sensor@48 {
                compatible = "ti,tmp102";
                reg = <0x48>;
                status = "okay";
            };
        };
    };
};
```

**Compilation**: `dtc -@ -I dts -O dtb i2c_sensor.dts -o i2c_sensor.dtbo`

## Task 3: GPIO and Interrupt Overlay

Create overlay defining GPIO button with interrupt support:

```dts
/dts-v1/;
/plugin/;

/ {
    fragment@0 {
        target-path = "/";
        __overlay__ {
            gpio_keys {
                compatible = "gpio-keys";
                status = "okay";
                
                button {
                    label = "Power Button";
                    linux,code = <115>;
                    gpios = <&gpio 27 1>;
                };
            };
        };
    };
};
```

## Task 4: Overlay Driver

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio.h>

static struct of_device_id device_match[] = {
    { .compatible = "vendor,overlay_device" },
    { }
};

MODULE_DEVICE_TABLE(of, device_match);

static int overlay_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    int gpio;
    
    gpio = of_get_named_gpio(np, "control-gpios", 0);
    if (gpio < 0) {
        dev_err(&pdev->dev, "No GPIO\n");
        return gpio;
    }
    
    gpio_request(gpio, "control");
    gpio_direction_output(gpio, 0);
    
    dev_info(&pdev->dev, "Overlay device probed (GPIO %d)\n", gpio);
    return 0;
}

static struct platform_driver overlay_driver = {
    .driver = {
        .name = "overlay_device",
        .of_match_table = device_match,
    },
    .probe = overlay_probe,
};

module_platform_driver(overlay_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Overlay-compatible driver");
```
