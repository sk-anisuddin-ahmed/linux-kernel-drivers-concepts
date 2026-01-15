#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/mutex.h>

struct sensor_data
{
    struct mutex lock;
    int current_val;
    int min_val;
    int max_val;
    int sum;
    int count;
};

static struct sensor_data sensor = {
    .current_val = 0,
    .min_val = INT_MAX,
    .max_val = INT_MIN,
    .sum = 0,
    .count = 0,
};

static ssize_t current_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;

    mutex_lock(&sensor.lock);
    ret = snprintf(buf, PAGE_SIZE, "%d\n", sensor.current_val);
    mutex_unlock(&sensor.lock);

    return ret;
}

static ssize_t min_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;

    mutex_lock(&sensor.lock);
    ret = snprintf(buf, PAGE_SIZE, "%d\n", sensor.min_val);
    mutex_unlock(&sensor.lock);

    return ret;
}

static ssize_t max_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;

    mutex_lock(&sensor.lock);
    ret = snprintf(buf, PAGE_SIZE, "%d\n", sensor.max_val);
    mutex_unlock(&sensor.lock);

    return ret;
}

static ssize_t average_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;
    int avg = 0;

    mutex_lock(&sensor.lock);
    if (sensor.count > 0)
        avg = sensor.sum / sensor.count;
    ret = snprintf(buf, PAGE_SIZE, "%d\n", avg);
    mutex_unlock(&sensor.lock);

    return ret;
}

static DEVICE_ATTR_RO(current);
static DEVICE_ATTR_RO(min);
static DEVICE_ATTR_RO(max);
static DEVICE_ATTR_RO(average);

static struct attribute *sensor_attrs[] = {
    &dev_attr_current.attr,
    &dev_attr_min.attr,
    &dev_attr_max.attr,
    &dev_attr_average.attr,
    NULL,
};

static struct attribute_group sensor_group = {
    .attrs = sensor_attrs,
};

// Function to update sensor reading (called from driver/ISR)
void sensor_update(int value)
{
    mutex_lock(&sensor.lock);

    sensor.current_val = value;

    if (value < sensor.min_val)
        sensor.min_val = value;

    if (value > sensor.max_val)
        sensor.max_val = value;

    sensor.sum += value;
    sensor.count++;

    mutex_unlock(&sensor.lock);
}

static int __init sensor_init(void)
{
    int ret;

    mutex_init(&sensor.lock);

    // Create sysfs group
    ret = sysfs_create_group(&THIS_MODULE->mkobj.kobj, &sensor_group);
    if (ret)
    {
        pr_err("Failed to create sysfs group\n");
        return ret;
    }

    pr_info("Sensor driver initialized\n");
    return 0;
}

static void __exit sensor_exit(void)
{
    sysfs_remove_group(&THIS_MODULE->mkobj.kobj, &sensor_group);
    pr_info("Sensor driver removed\n");
}

module_init(sensor_init);
module_exit(sensor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student");
MODULE_DESCRIPTION("Sensor stats via sysfs");
