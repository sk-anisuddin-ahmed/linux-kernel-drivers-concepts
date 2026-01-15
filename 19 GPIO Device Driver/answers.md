# 19 GPIO Device - Assignment Answers and Tasks

## Task 1: GPIO Theory

1. **What does gpio_request() do?**
gpio_request() claims ownership of a GPIO pin from the kernel. It prevents other drivers from using the same pin and marks it as in-use. Must be called before using the pin.

2. **Difference between gpio_direction_input() and gpio_direction_output()?**
gpio_direction_input() configures pin as input (reads external signals). gpio_direction_output() configures as output (drives signals) and requires an initial value (HIGH or LOW).

3. **What does gpio_get_value() return?**
gpio_get_value() returns 0 (LOW/ground) or non-zero (HIGH/Vcc) representing the current logic level of an input GPIO pin.

4. **How do you export GPIO to user space?**
Write the GPIO number to /sys/class/gpio/export. This creates /sys/class/gpio/gpioN/ with value and direction files for user-space access without a dedicated driver.

5. **When should you use ioctl vs sysfs for GPIO?**
Use ioctl() for high-frequency operations and complex control from applications. Use sysfs for simple shell scripts and manual testing. ioctl is faster; sysfs is simpler.

## Task 2: GPIO LED Control Driver

**Key Requirements:**

- Request GPIO pins
- Control pin direction (input/output)
- Set/read GPIO values using gpio_set_value() and gpio_get_value()

**Kernel Driver:**
```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>

#define GPIO_LED 17
#define GPIO_BTN 27

static int led_open(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t led_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	char val;
	if (get_user(val, buf))
		return -EFAULT;
	gpio_set_value(GPIO_LED, val == '1' ? 1 : 0);
	return 1;
}

static ssize_t led_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	char val = '0' + gpio_get_value(GPIO_BTN);
	if (put_user(val, buf))
		return -EFAULT;
	return 1;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.read = led_read,
	.write = led_write,
};

static int __init led_init(void)
{
	gpio_request(GPIO_LED, "led");
	gpio_direction_output(GPIO_LED, 0);
	gpio_request(GPIO_BTN, "btn");
	gpio_direction_input(GPIO_BTN);
	return 0;
}

static void __exit led_exit(void)
{
	gpio_free(GPIO_LED);
	gpio_free(GPIO_BTN);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
```

## Task 3: GPIO Interrupt Handling

Create driver that detects GPIO state changes via edge-triggered interrupts

**Kernel Driver:**
```c
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#define GPIO_BTN 27
static int btn_irq;
static int press_count = 0;

static irqreturn_t btn_interrupt(int irq, void *dev_id)
{
	press_count++;
	pr_info("Button pressed %d times\n", press_count);
	return IRQ_HANDLED;
}

static int __init btn_init(void)
{
	gpio_request(GPIO_BTN, "button");
	gpio_direction_input(GPIO_BTN);
	btn_irq = gpio_to_irq(GPIO_BTN);
	request_irq(btn_irq, btn_interrupt, IRQF_TRIGGER_FALLING, "btn_irq", NULL);
	pr_info("Button driver loaded (IRQ %d)\n", btn_irq);
	return 0;
}

static void __exit btn_exit(void)
{
	free_irq(btn_irq, NULL);
	gpio_free(GPIO_BTN);
	pr_info("Button driver unloaded\n");
}

module_init(btn_init);
module_exit(btn_exit);
MODULE_LICENSE("GPL");
```
