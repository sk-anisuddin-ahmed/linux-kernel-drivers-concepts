# 19 GPIO Device - Assignment Answers and Tasks

## Task 1: GPIO Theory

1. **What does gpio_request() do?** Claims a GPIO pin for exclusive use by the driver, preventing other code from using it.

2. **Difference between gpio_direction_input() and gpio_direction_output()?** Input: pin used to read external signal. Output: driver controls pin level.

3. **What does gpio_get_value() return?** 0 (low) or 1 (high), representing the current GPIO pin level.

4. **How do you export GPIO to user space?** Use /sys/class/gpio/ interface: echo N > /sys/class/gpio/export creates /sys/class/gpio/gpioN/

5. **When should you use ioctl vs sysfs for GPIO?** ioctl: fast, low-latency control. Sysfs: simple, don't need high performance.

## Task 2: GPIO LED Control Driver

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>

#define GPIO_LED_ON _IOW('G', 1, int)
#define GPIO_LED_OFF _IOW('G', 2, int)

static int led_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    int gpio = of_get_named_gpio(np, "led-gpio", 0);
    
    if (gpio < 0)
        return gpio;
    
    if (gpio_request(gpio, "led_control") < 0)
        return -EBUSY;
    
    gpio_direction_output(gpio, 0);
    platform_set_drvdata(pdev, (void *)(long)gpio);
    
    printk(KERN_INFO "LED GPIO: %d\n", gpio);
    return 0;
}

static long led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int gpio = (int)(long)file->private_data;
    
    switch(cmd) {
    case GPIO_LED_ON:
        gpio_set_value(gpio, 1);
        printk(KERN_INFO "LED ON\n");
        break;
    case GPIO_LED_OFF:
        gpio_set_value(gpio, 0);
        printk(KERN_INFO "LED OFF\n");
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static const struct file_operations fops = {
    .unlocked_ioctl = led_ioctl,
};

static int led_open(struct inode *inode, struct file *file)
{
    int gpio = (int)(long)platform_get_drvdata(NULL);
    file->private_data = (void *)(long)gpio;
    return 0;
}

// ... rest of driver with class_create, device_create, etc ...
```

## Task 3: GPIO Interrupt Handling

Create driver that detects GPIO state changes via edge-triggered interrupts:

```c
static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
    int gpio = (int)(long)dev_id;
    int level = gpio_get_value(gpio);
    
    printk(KERN_INFO "GPIO interrupt! Level: %d\n", level);
    return IRQ_HANDLED;
}

static int gpio_interrupt_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    int gpio, irq;
    
    gpio = of_get_named_gpio(np, "interrupt-gpio", 0);
    gpio_request(gpio, "gpio_interrupt");
    gpio_direction_input(gpio);
    
    // Get IRQ from GPIO
    irq = gpio_to_irq(gpio);
    
    // Register handler for rising edge
    request_irq(irq, gpio_irq_handler, IRQF_TRIGGER_RISING,
               "gpio_interrupt", (void *)(long)gpio);
    
    printk(KERN_INFO "GPIO interrupt setup complete (GPIO:%d IRQ:%d)\n", gpio, irq);
    return 0;
}
```
