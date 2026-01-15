# 20 GPIO Interrupts - Assignment Answers and Tasks

## Task 1: Concept Quiz

**1. Why is request_irq() preferred over polling?**
request_irq() lets the hardware interrupt the CPU immediately when an event occurs. Polling wastes CPU cycles constantly checking. Interrupts are event-driven and energy-efficient.

**2. What does IRQF_TRIGGER_FALLING do?**
IRQF_TRIGGER_FALLING fires the interrupt handler when GPIO transitions from HIGH to LOW (falling edge). Used for button presses (active low).

**3. What is the return type of an IRQ handler?**
irqreturn_t: either IRQ_HANDLED (interrupt was for this handler) or IRQ_NONE (interrupt was for another device).

**4. What happens if you don't call free_irq() in remove()?**
The IRQ handler remains registered and fires when the device triggers, but the driver is gone. This causes kernel crash or undefined behavior.

**5. Can you sleep in an IRQ handler?**
No. IRQ handlers run in atomic context (interrupts disabled). Use workqueues to defer sleeping work.

## Task 2: Rising Edge Support

Modify the driver to:

Accept a DT property "irq-trigger" (string: "rising", "falling", "both")

Parse it and choose IRQF_TRIGGER_RISING, etc., dynamically

**Device Tree:**
```
button {
	compatible = "my,button";
	gpios = <&gpio0 27 GPIO_ACTIVE_LOW>;
	irq-trigger = "falling";
};
```

**Kernel Driver:**
```c
static int btn_probe(struct platform_device *pdev)
{
	const char *trigger_str;
	unsigned long trigger = IRQF_TRIGGER_FALLING;

	of_property_read_string(pdev->dev.of_node, "irq-trigger", &trigger_str);

	if (strcmp(trigger_str, "rising") == 0)
		trigger = IRQF_TRIGGER_RISING;
	else if (strcmp(trigger_str, "both") == 0)
		trigger = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;

	gpio_request(GPIO_BTN, "button");
	gpio_direction_input(GPIO_BTN);
	btn_irq = gpio_to_irq(GPIO_BTN);
	request_irq(btn_irq, btn_interrupt, trigger, "btn_irq", NULL);

	return 0;
}
```

## Task 3: Count Button Presses

Add a press_count variable

Increment it in the ISR

Print total count on each press

**Kernel Driver with Sysfs Exposure:**
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
	pr_info("Button press %d\n", press_count);
	return IRQ_HANDLED;
}

static ssize_t press_count_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", press_count);
}

static ssize_t press_count_store(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t count)
{
	sscanf(buf, "%d", &press_count);
	return count;
}

static DEVICE_ATTR(press_count, 0644, press_count_show, press_count_store);

static int __init btn_init(void)
{
	gpio_request(GPIO_BTN, "button");
	gpio_direction_input(GPIO_BTN);
	btn_irq = gpio_to_irq(GPIO_BTN);
	request_irq(btn_irq, btn_interrupt, IRQF_TRIGGER_FALLING, "btn_irq", NULL);
	device_create_file(dev, &dev_attr_press_count);
	return 0;
}

static void __exit btn_exit(void)
{
	free_irq(btn_irq, NULL);
	gpio_free(GPIO_BTN);
	device_remove_file(dev, &dev_attr_press_count);
}

module_init(btn_init);
module_exit(btn_exit);
MODULE_LICENSE("GPL");
```

**Usage:**
```bash
cat /sys/devices/.../press_count
echo 0 > /sys/devices/.../press_count
```
