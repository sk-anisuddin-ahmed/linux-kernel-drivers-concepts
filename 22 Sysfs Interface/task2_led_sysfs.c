#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/gpio.h>

struct led_device
{
    struct mutex lock;
    int state;
    int gpio_pin;
};

static struct led_device led = {
    .state = 0,
    .gpio_pin = 17, // GPIO 17
};

static ssize_t state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;

    mutex_lock(&led.lock);
    ret = snprintf(buf, PAGE_SIZE, "%d\n", led.state);
    mutex_unlock(&led.lock);

    return ret;
}

static ssize_t state_store(struct device *dev, struct device_attribute *attr,
                           const char *buf, size_t count)
{
    int value;

    if (sscanf(buf, "%d", &value) != 1)
        return -EINVAL;

    if (value != 0 && value != 1)
        return -EINVAL;

    mutex_lock(&led.lock);
    led.state = value;
    gpio_set_value(led.gpio_pin, led.state);
    mutex_unlock(&led.lock);

    return count;
}

static DEVICE_ATTR_RW(state);

static struct attribute *led_attrs[] = {
    &dev_attr_state.attr,
    NULL,
};

static struct attribute_group led_group = {
    .attrs = led_attrs,
};

static int __init led_init(void)
{
    int ret;

    mutex_init(&led.lock);

    // Request GPIO
    if (gpio_request(led.gpio_pin, "led_gpio"))
    {
        pr_err("Failed to request GPIO\n");
        return -1;
    }

    // Set as output
    gpio_direction_output(led.gpio_pin, 0);

    // Create sysfs group
    ret = sysfs_create_group(&THIS_MODULE->mkobj.kobj, &led_group);
    if (ret)
    {
        pr_err("Failed to create sysfs group\n");
        gpio_free(led.gpio_pin);
        return ret;
    }

    pr_info("LED driver initialized\n");
    return 0;
}

static void __exit led_exit(void)
{
    sysfs_remove_group(&THIS_MODULE->mkobj.kobj, &led_group);
    gpio_set_value(led.gpio_pin, 0);
    gpio_free(led.gpio_pin);
    pr_info("LED driver removed\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student");
MODULE_DESCRIPTION("LED control via sysfs");
