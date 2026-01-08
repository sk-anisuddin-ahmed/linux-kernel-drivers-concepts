# Section 27: Power Management – Assignment

## Task 1: Theory Questions

1. **PM States**: S0 active, S1-S2 light sleep (CPU off, memory on), S3 suspend-to-RAM (memory only), S4 hibernate (disk), S5 off.

2. **Suspend Handler Responsibilities**: Save state, disable interrupts, cancel work, power down device. Must be fast and reliable.

3. **Resume Handler Responsibilities**: Power up, restore state, enable interrupts, restart work. Must handle initial power-up delays.

4. **Async Work and PM**: Must cancel_delayed_work_sync() before suspend. Don't queue work during suspend (driver_pm_ops prevents it).

5. **Wakeup Sources**: Devices can be designated as wake sources to resume system from sleep on specific events (button, network).

## Task 2: Temperature Sensor with Suspend/Resume

Implement temperature sensor supporting power management:

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/gpio.h>

#define GPIO_POWER 25

struct temp_sensor {
    int power_gpio;
    int temperature;
    int is_enabled;
    struct delayed_work read_work;
    struct workqueue_struct *wq;
};

static struct temp_sensor sensor;

static void read_temperature(struct work_struct *work)
{
    if (!sensor.is_enabled) return;
    
    sensor.temperature = 20 + (random32() % 20);
    printk(KERN_INFO "Temperature: %dC\n", sensor.temperature);
    
    if (sensor.is_enabled) {
        queue_delayed_work(sensor.wq, &sensor.read_work, 2*HZ);
    }
}

static int sensor_suspend(struct device *dev)
{
    printk(KERN_INFO "Sensor suspend\n");
    
    sensor.is_enabled = 0;
    cancel_delayed_work_sync(&sensor.read_work);
    
    gpio_set_value(sensor.power_gpio, 0);
    
    return 0;
}

static int sensor_resume(struct device *dev)
{
    printk(KERN_INFO "Sensor resume\n");
    
    gpio_set_value(sensor.power_gpio, 1);
    msleep(100);
    
    sensor.is_enabled = 1;
    queue_delayed_work(sensor.wq, &sensor.read_work, HZ);
    
    return 0;
}

static const struct dev_pm_ops sensor_pm_ops = {
    .suspend = sensor_suspend,
    .resume = sensor_resume,
};

static int sensor_probe(struct platform_device *pdev)
{
    gpio_request(GPIO_POWER, "sensor_power");
    gpio_direction_output(GPIO_POWER, 1);
    sensor.power_gpio = GPIO_POWER;
    
    sensor.wq = create_singlethread_workqueue("sensor_wq");
    sensor.is_enabled = 1;
    
    INIT_DELAYED_WORK(&sensor.read_work, read_temperature);
    queue_delayed_work(sensor.wq, &sensor.read_work, HZ);
    
    printk(KERN_INFO "Sensor probed\n");
    return 0;
}

static int sensor_remove(struct platform_device *pdev)
{
    sensor.is_enabled = 0;
    cancel_delayed_work_sync(&sensor.read_work);
    destroy_workqueue(sensor.wq);
    gpio_free(sensor.power_gpio);
    
    return 0;
}

static struct platform_driver sensor_driver = {
    .driver = {
        .name = "temp_sensor",
        .pm = &sensor_pm_ops,
    },
    .probe = sensor_probe,
    .remove = sensor_remove,
};

module_platform_driver(sensor_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Temperature sensor with PM");
```

## Task 3: Multi-Device PM Coordination

Implement coordinator managing power for multiple dependent devices:

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/gpio.h>
#include <linux/mutex.h>

#define NUM_DEVICES 3

struct device_pm {
    int power_gpio;
    int refcount;
    int is_powered;
};

static struct device_pm devices[NUM_DEVICES] = {
    { .power_gpio = 25 },
    { .power_gpio = 26 },
    { .power_gpio = 27 },
};

static struct mutex pm_lock = __MUTEX_INITIALIZER(pm_lock);

static int pm_device_on(int dev_id)
{
    mutex_lock(&pm_lock);
    
    if (devices[dev_id].refcount == 0) {
        gpio_set_value(devices[dev_id].power_gpio, 1);
        devices[dev_id].is_powered = 1;
        printk(KERN_INFO "Device %d powered on\n", dev_id);
    }
    
    devices[dev_id].refcount++;
    
    mutex_unlock(&pm_lock);
    
    return 0;
}

static int pm_device_off(int dev_id)
{
    mutex_lock(&pm_lock);
    
    if (devices[dev_id].refcount > 0) {
        devices[dev_id].refcount--;
        
        if (devices[dev_id].refcount == 0) {
            gpio_set_value(devices[dev_id].power_gpio, 0);
            devices[dev_id].is_powered = 0;
            printk(KERN_INFO "Device %d powered off\n", dev_id);
        }
    }
    
    mutex_unlock(&pm_lock);
    
    return 0;
}

static int coordinator_suspend(struct device *dev)
{
    int i;
    
    printk(KERN_INFO "PM coordinator suspend\n");
    
    for (i = 0; i < NUM_DEVICES; i++) {
        while (devices[i].refcount > 0) {
            pm_device_off(i);
        }
    }
    
    return 0;
}

static int coordinator_resume(struct device *dev)
{
    int i;
    
    printk(KERN_INFO "PM coordinator resume\n");
    
    for (i = 0; i < NUM_DEVICES; i++) {
        pm_device_on(i);
    }
    
    return 0;
}

static const struct dev_pm_ops coordinator_pm_ops = {
    .suspend = coordinator_suspend,
    .resume = coordinator_resume,
};

static int coordinator_probe(struct platform_device *pdev)
{
    int i;
    
    for (i = 0; i < NUM_DEVICES; i++) {
        gpio_request(devices[i].power_gpio, "device_power");
        gpio_direction_output(devices[i].power_gpio, 0);
        pm_device_on(i);
    }
    
    printk(KERN_INFO "PM coordinator probed\n");
    return 0;
}

static struct platform_driver coordinator_driver = {
    .driver = {
        .name = "pm_coordinator",
        .pm = &coordinator_pm_ops,
    },
    .probe = coordinator_probe,
};

module_platform_driver(coordinator_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Multi-device PM coordinator");
```

**Key Pattern**: Refcounting for shared power domains, mutex protecting state, coordinated suspend/resume.
