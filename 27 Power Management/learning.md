# Section 27: Power Management – Suspend/Resume

## Power States

| State | Description | CPU | Memory |
|-------|-------------|-----|--------|
| S0 | Running | Active | Active |
| S1 | Sleep | Stopped | Powered |
| S3 | Suspend-to-RAM | Off | Powered |
| S4 | Hibernate | Off | Off |
| S5 | Off | Off | Off |

## Suspend/Resume Handlers

```c
static int driver_suspend(struct device *dev)
{
    struct my_device *my_dev = dev_get_drvdata(dev);
    
    // Save device state
    my_dev->saved_state = read_device_register();
    
    // Stop ongoing operations
    disable_irq(my_dev->irq);
    cancel_delayed_work_sync(&my_dev->work);
    
    // Power down if needed
    gpio_set_value(my_dev->power_gpio, 0);
    
    printk(KERN_INFO "Device suspended\n");
    return 0;
}

static int driver_resume(struct device *dev)
{
    struct my_device *my_dev = dev_get_drvdata(dev);
    
    // Power up
    gpio_set_value(my_dev->power_gpio, 1);
    msleep(100);  // Wait for power stabilization
    
    // Restore device state
    write_device_register(my_dev->saved_state);
    
    // Resume operations
    enable_irq(my_dev->irq);
    schedule_work(&my_dev->work);
    
    printk(KERN_INFO "Device resumed\n");
    return 0;
}

static const struct dev_pm_ops driver_pm_ops = {
    .suspend = driver_suspend,
    .resume = driver_resume,
};

static struct platform_driver my_driver = {
    .driver = {
        .name = "my_device",
        .pm = &driver_pm_ops,
    },
    .probe = my_probe,
    .remove = my_remove,
};
```

## Complete Example: Power-Managed Device

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

#define GPIO_POWER 25
#define GPIO_IRQ 27

struct pm_device {
    struct device *dev;
    int irq;
    int power_gpio;
    int sensor_reading;
    int enabled;
    struct delayed_work sensor_work;
    struct workqueue_struct *wq;
};

static struct pm_device pm_dev;

static void sensor_read_work(struct work_struct *work)
{
    if (!pm_dev.enabled) return;
    
    pm_dev.sensor_reading = (pm_dev.sensor_reading + 1) % 1024;
    printk(KERN_DEBUG "Sensor reading: %d\n", pm_dev.sensor_reading);
    
    if (pm_dev.enabled) {
        queue_delayed_work(pm_dev.wq, &pm_dev.sensor_work, HZ);
    }
}

static int device_suspend(struct device *dev)
{
    printk(KERN_INFO "Suspending device\n");
    
    pm_dev.enabled = 0;
    disable_irq(pm_dev.irq);
    cancel_delayed_work_sync(&pm_dev.sensor_work);
    
    gpio_set_value(pm_dev.power_gpio, 0);
    printk(KERN_INFO "Device powered down\n");
    
    return 0;
}

static int device_resume(struct device *dev)
{
    printk(KERN_INFO "Resuming device\n");
    
    gpio_set_value(pm_dev.power_gpio, 1);
    msleep(50);
    
    pm_dev.enabled = 1;
    enable_irq(pm_dev.irq);
    queue_delayed_work(pm_dev.wq, &pm_dev.sensor_work, HZ);
    
    printk(KERN_INFO "Device powered up\n");
    
    return 0;
}

static const struct dev_pm_ops pm_ops = {
    .suspend = device_suspend,
    .resume = device_resume,
};

static int device_probe(struct platform_device *pdev)
{
    gpio_request(GPIO_POWER, "power");
    gpio_direction_output(GPIO_POWER, 1);
    pm_dev.power_gpio = GPIO_POWER;
    
    pm_dev.irq = gpio_to_irq(GPIO_IRQ);
    pm_dev.wq = create_singlethread_workqueue("pm_wq");
    pm_dev.enabled = 1;
    
    INIT_DELAYED_WORK(&pm_dev.sensor_work, sensor_read_work);
    queue_delayed_work(pm_dev.wq, &pm_dev.sensor_work, HZ);
    
    printk(KERN_INFO "PM device probed\n");
    return 0;
}

static int device_remove(struct platform_device *pdev)
{
    pm_dev.enabled = 0;
    cancel_delayed_work_sync(&pm_dev.sensor_work);
    destroy_workqueue(pm_dev.wq);
    gpio_free(pm_dev.power_gpio);
    
    printk(KERN_INFO "PM device removed\n");
    return 0;
}

static struct platform_driver pm_driver = {
    .driver = {
        .name = "pm_device",
        .pm = &pm_ops,
    },
    .probe = device_probe,
    .remove = device_remove,
};

module_platform_driver(pm_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Power-managed device driver");
```

## Important Notes

1. **State Preservation** essential for resume correctness
2. **Disable interrupts** during suspend to prevent wake-ups
3. **Power sequence timing** critical (wait after power-on)
4. **Cancel async work** before suspend to prevent race conditions
5. **Test wake-up paths** to ensure device responds properly
