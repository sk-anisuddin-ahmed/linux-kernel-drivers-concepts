# 20 GPIO Interrupts - Assignment Answers and Tasks

## Task 1: Interrupt Theory

1. **What is an IRQ?** Interrupt Request number; identifies which interrupt source and handler to invoke.

2. **What's the difference between IRQF_TRIGGER_RISING and IRQF_TRIGGER_FALLING?** RISING: triggers on 0→1 transition. FALLING: triggers on 1→0 transition.

3. **Why should interrupt handlers be fast?** They interrupt normal code execution. Long handlers increase latency and can cause priority inversion.

4. **What does gpio_to_irq() do?** Converts GPIO number to its associated IRQ number for request_irq().

5. **When should you use tasklets instead of doing work in handler?** For long operations (>microseconds) or operations that need to sleep.

## Task 2: Button Interrupt Handler

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

static int button_count = 0;

static irqreturn_t button_handler(int irq, void *dev_id)
{
    int gpio = (int)(long)dev_id;
    int level = gpio_get_value(gpio);
    
    button_count++;
    printk(KERN_INFO "Button pressed! (count=%d, level=%d)\n", button_count, level);
    
    return IRQ_HANDLED;
}

static int button_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    int gpio = of_get_named_gpio(np, "button-gpio", 0);
    int irq, ret;
    
    if (gpio < 0)
        return gpio;
    
    // Setup GPIO
    if (gpio_request(gpio, "button") < 0)
        return -EBUSY;
    
    gpio_direction_input(gpio);
    
    // Get IRQ and register handler
    irq = gpio_to_irq(gpio);
    if (irq < 0)
        return irq;
    
    ret = request_irq(irq, button_handler, 
                     IRQF_TRIGGER_FALLING, "button_irq",
                     (void *)(long)gpio);
    if (ret < 0) {
        gpio_free(gpio);
        return ret;
    }
    
    platform_set_drvdata(pdev, (void *)(long)irq);
    printk(KERN_INFO "Button handler registered (GPIO:%d IRQ:%d)\n", gpio, irq);
    
    return 0;
}

static int button_remove(struct platform_device *pdev)
{
    int irq = (int)(long)platform_get_drvdata(pdev);
    free_irq(irq, NULL);
    return 0;
}
```

## Task 3: Debounced Button with Workqueue

```c
#include <linux/workqueue.h>

static struct work_struct button_work;
static int debounce_gpio;

static void button_work_handler(struct work_struct *work)
{
    int level = gpio_get_value(debounce_gpio);
    
    // Re-check after debounce delay
    msleep(20);
    
    if (level == gpio_get_value(debounce_gpio)) {
        // Level stable, process button
        printk(KERN_INFO "Button stable at level %d\n", level);
    }
}

static irqreturn_t debounced_handler(int irq, void *dev_id)
{
    // Queue work instead of handling immediately
    schedule_work(&button_work);
    return IRQ_HANDLED;
}

static int debounce_probe(struct platform_device *pdev)
{
    int gpio, irq;
    
    gpio = of_get_named_gpio(pdev->dev.of_node, "button-gpio", 0);
    debounce_gpio = gpio;
    
    gpio_request(gpio, "debounce_button");
    gpio_direction_input(gpio);
    
    irq = gpio_to_irq(gpio);
    request_irq(irq, debounced_handler, IRQF_TRIGGER_FALLING,
               "debounce_button", NULL);
    
    INIT_WORK(&button_work, button_work_handler);
    
    return 0;
}
```
