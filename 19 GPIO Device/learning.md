# 19 GPIO Devices with Device Tree

GPIO devices allow kernel code to control individual pins for LEDs, buttons, sensors. This section combines DT knowledge with GPIO handling.

## Key Concepts

- GPIO numbers: 0-1023+ depending on controller
- GPIO direction: input or output
- GPIO value: high (1) or low (0)
- OF functions: of_get_named_gpio(), gpio_request(), gpio_get_value(), gpio_set_value()

## Implementation Pattern

```c
static int gpio_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    int gpio = of_get_named_gpio(np, "control-gpio", 0);
    
    gpio_request(gpio, "my_device");
    gpio_direction_output(gpio, 0);
    
    platform_set_drvdata(pdev, (void *)(long)gpio);
    return 0;
}

static int gpio_remove(struct platform_device *pdev)
{
    int gpio = (int)(long)platform_get_drvdata(pdev);
    gpio_free(gpio);
    return 0;
}
```

## ioctl for GPIO Control

```c
static long gpio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int gpio = (int)(long)file->private_data;
    int value;
    
    switch(cmd) {
    case GPIO_SET_HIGH:
        gpio_set_value(gpio, 1);
        break;
    case GPIO_SET_LOW:
        gpio_set_value(gpio, 0);
        break;
    case GPIO_GET_VALUE:
        value = gpio_get_value(gpio);
        copy_to_user((int*)arg, &value, sizeof(int));
        break;
    }
    return 0;
}
```

## Important Notes

1. Always request GPIO before using
2. Specify direction (input/output) clearly
3. Check return values from gpio_request()
4. Free GPIO in cleanup
5. Use gpio_set_value() atomically for consistency
