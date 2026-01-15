# Section 21: Deferred Processing – Assignment

## Task 1: Quiz

**1. Why must you not sleep in an ISR?**
ISR runs in atomic context (interrupts disabled). Sleeping would block the entire system. Use workqueues to defer sleeping work.

**2. What happens if you call msleep() in an IRQ handler?**
Kernel panic or hang. The system deadlocks because interrupts are disabled and nothing can wake the process.

**3. What is the benefit of a workqueue over tasklets?**
Workqueues can sleep and use blocking APIs. Tasklets run in soft-IRQ context (can't sleep). Workqueues are simpler for I/O operations.

**4. What does cancel_work_sync() ensure?**
cancel_work_sync() cancels pending work and waits for any running work to complete. Guarantees work is not executing when it returns.

**5. Can you call gpiod_get_value() inside a workqueue?**
Yes. Workqueues run in sleepable context (preemptible). You can safely call blocking GPIO functions.

## Task 2: Toggle LED on Button Press (in Workqueue)

Add an output GPIO (led_gpiod)

In btn_work_handler(), toggle LED state

Use a static flag to keep track of ON/OFF

**Kernel Driver:**
```c
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/gpio/consumer.h>

#define DEBOUNCE_DELAY msecs_to_jiffies(100)

static struct gpio_desc *btn_gpiod, *led_gpiod;
static struct work_struct btn_work;
static int led_state = 0;

static void btn_work_handler(struct work_struct *work)
{
	led_state = !led_state;
	gpiod_set_value(led_gpiod, led_state);
	pr_info("LED toggled to %s\n", led_state ? "ON" : "OFF");
}

static irqreturn_t btn_interrupt(int irq, void *dev_id)
{
	queue_delayed_work(system_wq, (struct delayed_work *)&btn_work, DEBOUNCE_DELAY);
	return IRQ_HANDLED;
}

static int __init led_init(void)
{
	btn_gpiod = gpiod_get(NULL, "btn", GPIOD_IN);
	led_gpiod = gpiod_get(NULL, "led", GPIOD_OUT);
	INIT_WORK(&btn_work, btn_work_handler);
	request_irq(gpiod_to_irq(btn_gpiod), btn_interrupt, IRQF_TRIGGER_FALLING, "btn", NULL);
	return 0;
}

static void __exit led_exit(void)
{
	cancel_work_sync(&btn_work);
	free_irq(gpiod_to_irq(btn_gpiod), NULL);
	gpiod_put(btn_gpiod);
	gpiod_put(led_gpiod);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
```

## Task 3: Implement Bounce Filter (last_press_jiffies)

Track last button press time to ignore bounces within 100ms window. Use time_before() macro to check if sufficient time has elapsed. Only queue work if debounce threshold exceeded.

**Kernel Driver:**
```c
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/gpio/consumer.h>
#include <linux/jiffies.h>

#define DEBOUNCE_THRESHOLD 100
static struct gpio_desc *btn_gpiod, *led_gpiod;
static struct delayed_work btn_work;
static unsigned long last_press_jiffies = 0;

static void btn_work_handler(struct work_struct *work)
{
	static int led_state = 0;
	led_state = !led_state;
	gpiod_set_value(led_gpiod, led_state);
	pr_info("LED %s\n", led_state ? "ON" : "OFF");
}

static irqreturn_t btn_interrupt(int irq, void *dev_id)
{
	if (time_before(last_press_jiffies + msecs_to_jiffies(DEBOUNCE_THRESHOLD), jiffies)) {
		last_press_jiffies = jiffies;
		cancel_delayed_work(&btn_work);
		queue_delayed_work(system_wq, &btn_work, msecs_to_jiffies(DEBOUNCE_THRESHOLD));
	}
	return IRQ_HANDLED;
}

static int __init bounce_init(void)
{
	btn_gpiod = gpiod_get(NULL, "btn", GPIOD_IN);
	led_gpiod = gpiod_get(NULL, "led", GPIOD_OUT);
	INIT_DELAYED_WORK(&btn_work, btn_work_handler);
	request_irq(gpiod_to_irq(btn_gpiod), btn_interrupt, IRQF_TRIGGER_FALLING, "btn", NULL);
	return 0;
}

static void __exit bounce_exit(void)
{
	cancel_delayed_work_sync(&btn_work);
	free_irq(gpiod_to_irq(btn_gpiod), NULL);
	gpiod_put(btn_gpiod);
	gpiod_put(led_gpiod);
}

module_init(bounce_init);
module_exit(bounce_exit);
MODULE_LICENSE("GPL");
```

