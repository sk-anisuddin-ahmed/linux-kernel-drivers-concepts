# Section 22: Sysfs Interface – Exposing Device Attributes

## Learning Outcomes

- Create device attributes in `/sys/class/` and `/sys/devices/`
- Implement sysfs attribute read/write operations
- Use attribute groups for organized device exposure
- Implement show/store functions with proper locking
- Expose both device state and control parameters via sysfs

## Key Concepts

### Why Sysfs?

Sysfs provides **in-kernel user-space interface** without ioctl:

- `/sys/class/myclass/mydevice/` for user-friendly attributes
- `/sys/devices/...` for hardware tree structure
- Text-based (easy to script with shell)
- No special tools needed (just `cat` / `echo`)

### Attribute Macros

```c
// Read-only attribute (mode 0444)
static DEVICE_ATTR_RO(status);

// Write-only attribute (mode 0222)
static DEVICE_ATTR_WO(command);

// Read-write attribute (mode 0644)
static DEVICE_ATTR_RW(config);

// Manual mode definition
static DEVICE_ATTR(temperature, 0644, temp_show, temp_store);
```

### Show and Store Functions

```c
// Show function (read)
static ssize_t temperature_show(struct device *dev,
                               struct device_attribute *attr,
                               char *buf)
{
    struct my_device *dev_data = dev_get_drvdata(dev);
    return sprintf(buf, "%d\n", dev_data->temperature);
}

// Store function (write)
static ssize_t temperature_store(struct device *dev,
                                struct device_attribute *attr,
                                const char *buf, size_t count)
{
    struct my_device *dev_data = dev_get_drvdata(dev);
    int temp;
    
    if (sscanf(buf, "%d", &temp) != 1) {
        return -EINVAL;
    }
    
    dev_data->temperature = temp;
    return count;
}

static DEVICE_ATTR(temperature, 0644, temperature_show, temperature_store);
```

### Attribute Groups

```c
static struct attribute *my_attrs[] = {
    &dev_attr_temperature.attr,
    &dev_attr_status.attr,
    &dev_attr_config.attr,
    NULL
};

static struct attribute_group my_group = {
    .attrs = my_attrs,
    .name = "config",  // Optional subdirectory name
};

// In init:
sysfs_create_group(&device->kobj, &my_group);

// In cleanup:
sysfs_remove_group(&device->kobj, &my_group);
```

## Complete Example: Temperature Sensor with Sysfs

```c
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>

struct sensor_device {
    struct device *dev;
    int temperature;
    int min_threshold;
    int max_threshold;
    unsigned long last_read;
    struct mutex lock;
    int alarm_triggered;
};

static struct sensor_device sensor = {
    .temperature = 25,
    .min_threshold = 10,
    .max_threshold = 40,
    .alarm_triggered = 0,
};

// Sysfs attributes

static ssize_t temperature_show(struct device *dev,
                               struct device_attribute *attr,
                               char *buf)
{
    mutex_lock(&sensor.lock);
    
    // Simulate sensor read every 1 second
    if (time_after(jiffies, sensor.last_read + HZ)) {
        sensor.temperature += ((random32() % 7) - 3);  // ±3°C change
        sensor.last_read = jiffies;
    }
    
    int temp = sensor.temperature;
    mutex_unlock(&sensor.lock);
    
    return sprintf(buf, "%d\n", temp);
}

static ssize_t min_threshold_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
    return sprintf(buf, "%d\n", sensor.min_threshold);
}

static ssize_t min_threshold_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf, size_t count)
{
    int threshold;
    
    if (sscanf(buf, "%d", &threshold) != 1) {
        return -EINVAL;
    }
    
    if (threshold < 0 || threshold > 100) {
        return -ERANGE;
    }
    
    mutex_lock(&sensor.lock);
    sensor.min_threshold = threshold;
    mutex_unlock(&sensor.lock);
    
    return count;
}

static ssize_t max_threshold_show(struct device *dev,
                                 struct device_attribute *attr,
                                 char *buf)
{
    return sprintf(buf, "%d\n", sensor.max_threshold);
}

static ssize_t max_threshold_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf, size_t count)
{
    int threshold;
    
    if (sscanf(buf, "%d", &threshold) != 1) {
        return -EINVAL;
    }
    
    if (threshold < 0 || threshold > 100) {
        return -ERANGE;
    }
    
    mutex_lock(&sensor.lock);
    sensor.max_threshold = threshold;
    mutex_unlock(&sensor.lock);
    
    return count;
}

static ssize_t status_show(struct device *dev,
                          struct device_attribute *attr,
                          char *buf)
{
    mutex_lock(&sensor.lock);
    
    const char *status = "normal";
    if (sensor.temperature < sensor.min_threshold) {
        status = "cold";
    } else if (sensor.temperature > sensor.max_threshold) {
        status = "overtemp";
    }
    
    int alarm = sensor.alarm_triggered;
    mutex_unlock(&sensor.lock);
    
    return sprintf(buf, "status=%s alarm=%d\n", status, alarm);
}

// Attribute definitions
static DEVICE_ATTR_RO(temperature);
static DEVICE_ATTR_RW(min_threshold);
static DEVICE_ATTR_RW(max_threshold);
static DEVICE_ATTR_RO(status);

static struct attribute *sensor_attrs[] = {
    &dev_attr_temperature.attr,
    &dev_attr_min_threshold.attr,
    &dev_attr_max_threshold.attr,
    &dev_attr_status.attr,
    NULL
};

static struct attribute_group sensor_group = {
    .attrs = sensor_attrs,
};

// Module init/exit

static struct class *sensor_class;
static struct device *sensor_device;

static int __init sensor_sysfs_init(void)
{
    int ret;
    
    mutex_init(&sensor.lock);
    
    // Create class
    sensor_class = class_create(THIS_MODULE, "sensor");
    if (IS_ERR(sensor_class)) {
        printk(KERN_ERR "Failed to create class\n");
        return PTR_ERR(sensor_class);
    }
    
    // Create device
    sensor_device = device_create(sensor_class, NULL, MKDEV(0, 0), NULL, "tempmon");
    if (IS_ERR(sensor_device)) {
        printk(KERN_ERR "Failed to create device\n");
        class_destroy(sensor_class);
        return PTR_ERR(sensor_device);
    }
    
    // Create sysfs attributes
    ret = sysfs_create_group(&sensor_device->kobj, &sensor_group);
    if (ret < 0) {
        printk(KERN_ERR "Failed to create sysfs group\n");
        device_destroy(sensor_class, MKDEV(0, 0));
        class_destroy(sensor_class);
        return ret;
    }
    
    dev_set_drvdata(sensor_device, &sensor);
    
    printk(KERN_INFO "Sensor sysfs driver loaded\n");
    printk(KERN_INFO "Read: cat /sys/class/sensor/tempmon/temperature\n");
    printk(KERN_INFO "Write: echo 35 > /sys/class/sensor/tempmon/max_threshold\n");
    
    return 0;
}

static void __exit sensor_sysfs_exit(void)
{
    sysfs_remove_group(&sensor_device->kobj, &sensor_group);
    device_destroy(sensor_class, MKDEV(0, 0));
    class_destroy(sensor_class);
    
    printk(KERN_INFO "Sensor sysfs driver unloaded\n");
}

module_init(sensor_sysfs_init);
module_exit(sensor_sysfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Student");
MODULE_DESCRIPTION("Temperature sensor with sysfs interface");
```

## Makefile

```makefile
obj-m += sensor_sysfs.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
 make -C $(KDIR) M=$(PWD) modules

clean:
 make -C $(KDIR) M=$(PWD) clean

test: all
 sudo insmod sensor_sysfs.ko
 @echo "Reading temperature..."
 cat /sys/class/sensor/tempmon/temperature
 @echo "Setting max threshold to 35..."
 echo 35 | sudo tee /sys/class/sensor/tempmon/max_threshold > /dev/null
 cat /sys/class/sensor/tempmon/status
 sudo rmmod sensor_sysfs
```

## Key Concepts

| Concept | Purpose |
|---------|---------|
| sysfs | Filesystem representation of kernel objects |
| Attribute | Single sysfs file (show/store operation) |
| Attribute Group | Collection of related attributes |
| dev_get_drvdata | Retrieve driver data from device |
| mutex_lock | Protect concurrent sysfs access |

## Important Notes

1. **Thread Safety**: Use mutexes in show/store functions for shared data
2. **Return Values**: show() returns bytes written; store() returns count or error
3. **Format Conventions**: Use sprintf/sscanf for text format conversion
4. **No Blocking**: Avoid long operations; use workqueues if needed
5. **Cleanup Order**: Remove sysfs attributes before destroying device/class
6. **Performance**: Sysfs is for configuration/monitoring, not high-speed I/O
