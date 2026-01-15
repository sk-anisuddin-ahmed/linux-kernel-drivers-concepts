# Section 27: Power Management � Assignment

## Task 1: Quiz

**1. What does dev_pm_ops do?**
dev_pm_ops is a structure containing function pointers for power management callbacks (suspend, resume, freeze, thaw, etc). The kernel calls these during system sleep transitions to let drivers save/restore device state.

**2. What happens if you forget to enable_irq() on resume?**
Interrupts remain disabled. The device won't generate interrupts and userspace can't detect button presses. The system appears frozen from the driver's perspective.

**3. Can you call gpiod_get() inside resume()?**
No. GPIO handles must be obtained during probe(). During resume(), only use already-obtained handles. Calling gpiod_get() may fail or cause sleeping in non-sleepable context.

**4. Should you allocate memory in suspend()?**
No. Suspend runs with limited memory available and interrupts may be disabled. Pre-allocate any needed memory during probe() and reuse it.

**5. What's the safest place to restore LED state?**
In resume_noirq() if using only register writes, or in resume() if you need interrupts/workqueues. Never in suspend_noirq() as that's too early.

## Task 2: Add Wakeup Capability (Bonus)

In DT, set: wakeup-source;

In your driver: device_init_wakeup(&pdev->dev, true);

Test if button can wake the system from suspend.

**Device Tree:**
```
gpiobtn {
	compatible = "my,gpiobtn";
	gpios = <&gpio0 17 GPIO_ACTIVE_LOW>;
	wakeup-source;
};
```

**Kernel Driver:**
```c
#include <linux/pm_wakeup.h>

static int btn_probe(struct platform_device *pdev)
{
	device_init_wakeup(&pdev->dev, true);
	enable_irq_wake(irq);
	pr_info("Wakeup enabled\n");
	return 0;
}

static int btn_remove(struct platform_device *pdev)
{
	disable_irq_wake(irq);
	device_init_wakeup(&pdev->dev, false);
	return 0;
}
```

## Task 3: LED Blink Resume Indicator

In resume(), blink LED 3 times using gpiod_set_value() and msleep()

Only do this if you're using workqueues or pm_wq, as resume() runs in a sleepable context.

**Kernel Driver:**
```c
#include <linux/pm.h>
#include <linux/delay.h>

static struct gpio_desc *led_gpiod;

static int btn_resume(struct device *dev)
{
	int i;

	pr_info("Resume: Blinking LED\n");

	for (i = 0; i < 3; i++) {
		gpiod_set_value(led_gpiod, 1);
		msleep(200);
		gpiod_set_value(led_gpiod, 0);
		msleep(200);
	}

	enable_irq(irq);
	return 0;
}

static int btn_suspend(struct device *dev)
{
	disable_irq(irq);
	return 0;
}

static const struct dev_pm_ops btn_pm_ops = {
	.suspend = btn_suspend,
	.resume = btn_resume,
};

static struct platform_driver btn_driver = {
	.probe = btn_probe,
	.remove = btn_remove,
	.driver = {
		.name = "gpiobtn",
		.pm = &btn_pm_ops,
	},
};
```
