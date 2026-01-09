# Section 22: Sysfs Interface – Assignment

## Task 1: Theory Questions

1. **Sysfs vs ioctl**: Sysfs provides text-based attribute files in /sys filesystem; ioctl is command-based. Sysfs preferred for simple state/config.

2. **Attribute Groups**: Collections of related attributes (struct attribute_group) created/removed together. Keeps device files organized.

3. **Show/Store Functions**: show() returns bytes written to buffer; store() returns count or error code. Both must be thread-safe.

4. **Thread Safety**: Use mutexes in show/store to protect shared device data from concurrent access.

5. **DEVICE_ATTR Macros**: Convenience macros generating static structures. _RO() read-only,_WO() write-only, _RW() for both.

## Task 2: LED Control via Sysfs

Implement sysfs interface for LED on/off control with thread safety:

```c
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/gpio.h>

#define GPIO_LED 17

struct led_device {
    int gpio;
    int state;
    struct device *dev;
    struct mutex lock;
};

static struct led_device led = {
    .gpio = GPIO_LED,
    .state = 0,
};

static ssize_t state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    mutex_lock(&led.lock);
    int state = led.state;
    mutex_unlock(&led.lock);
    return sprintf(buf, "%d\n", state);
}

static ssize_t state_store(struct device *dev, struct device_attribute *attr,
                          const char *buf, size_t count)
{
    int state;
    if (sscanf(buf, "%d", &state) != 1 || state < 0 || state > 1) {
        return -EINVAL;
    }
    
    mutex_lock(&led.lock);
    led.state = state;
    gpio_set_value(led.gpio, state);
    mutex_unlock(&led.lock);
    return count;
}

static DEVICE_ATTR_RW(state);

static struct attribute *led_attrs[] = {
    &dev_attr_state.attr,
    NULL
};

static struct attribute_group led_group = {
    .attrs = led_attrs,
};

static int __init led_init(void)
{
    gpio_request(led.gpio, "led");
    gpio_direction_output(led.gpio, 0);
    mutex_init(&led.lock);
    
    struct class *led_class = class_create(THIS_MODULE, "led");
    led.dev = device_create(led_class, NULL, MKDEV(0, 0), NULL, "myled");
    sysfs_create_group(&led.dev->kobj, &led_group);
    
    return 0;
}

static void __exit led_exit(void)
{
    printk(KERN_INFO "LED driver unloaded\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LED sysfs control");
```

**Usage**: `echo 1 > /sys/class/led/myled/state`

## Task 3: Sensor Stats via Sysfs

Implement multi-attribute sysfs interface for sensor with min/max/average:

```c
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>

struct sensor_device {
    int current_temp;
    int min_temp;
    int max_temp;
    int avg_temp;
    int samples;
    struct device *dev;
    struct mutex lock;
};

static struct sensor_device sensor = {
    .current_temp = 25,
    .min_temp = 20,
    .max_temp = 30,
    .avg_temp = 25,
    .samples = 100,
};

static ssize_t current_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", sensor.current_temp);
}

static ssize_t stats_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    mutex_lock(&sensor.lock);
    int len = sprintf(buf, "min=%d max=%d avg=%d samples=%d\n",
                     sensor.min_temp, sensor.max_temp, sensor.avg_temp, sensor.samples);
    mutex_unlock(&sensor.lock);
    return len;
}

static DEVICE_ATTR_RO(current);
static DEVICE_ATTR_RO(stats);

static struct attribute *sensor_attrs[] = {
    &dev_attr_current.attr,
    &dev_attr_stats.attr,
    NULL
};

static struct attribute_group sensor_group = {
    .attrs = sensor_attrs,
};

static int __init sensor_init(void)
{
    mutex_init(&sensor.lock);
    
    struct class *sensor_class = class_create(THIS_MODULE, "sensor");
    sensor.dev = device_create(sensor_class, NULL, MKDEV(0, 0), NULL, "temp0");
    sysfs_create_group(&sensor.dev->kobj, &sensor_group);
    
    printk(KERN_INFO "Sensor driver loaded\n");
    return 0;
}

static void __exit sensor_exit(void)
{
    printk(KERN_INFO "Sensor driver unloaded\n");
}

module_init(sensor_init);
module_exit(sensor_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Temperature sensor with sysfs");
```

**Usage**: `cat /sys/class/sensor/temp0/stats` shows min=20 max=30 avg=25 samples=100
