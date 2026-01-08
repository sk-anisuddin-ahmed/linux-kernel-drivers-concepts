# Section 21: Deferred Processing – Workqueues and Tasklets

## Learning Outcomes

By the end of this section, you will:

- Understand top-half vs bottom-half interrupt processing
- Implement tasklets for lightweight deferred work
- Use workqueues for heavy, sleepable work
- Choose appropriate deferral mechanisms for different scenarios
- Implement debouncing and rate-limiting with deferred processing

## Key Concepts

### Why Deferred Processing?

Interrupt handlers must complete quickly. For long operations:

1. **Top Half (Interrupt Context)**: Minimal work, save state
2. **Bottom Half (Deferred Context)**: Process data, wake users

### Tasklets vs Workqueues

| Feature | Tasklets | Workqueues |
|---------|----------|-----------|
| Context | Softirq (atomic) | Process (kthread) |
| Can Sleep | ❌ No | ✅ Yes |
| Priority | Fixed | Configurable |
| Scope | CPU-specific | System-wide |
| Use Case | Light work | Heavy work |

### Tasklet Implementation

```c
// Declare tasklet handler
static void my_tasklet_func(unsigned long data)
{
    struct my_device *dev = (struct my_device *)data;
    printk(KERN_INFO "Tasklet processing for device %d\n", dev->id);
}

// Declare tasklet
DECLARE_TASKLET(my_tasklet, my_tasklet_func, (unsigned long)&my_dev);

// Schedule in interrupt handler
static irqreturn_t button_irq(int irq, void *dev_id)
{
    tasklet_schedule(&my_tasklet);
    return IRQ_HANDLED;
}

// Enable/disable
tasklet_disable(&my_tasklet);  // Prevents scheduling
tasklet_enable(&my_tasklet);   // Re-enables

// Kill tasklet
tasklet_kill(&my_tasklet);     // Waits for current execution
```

### Workqueue Implementation

```c
// Declare work queue
static struct workqueue_struct *my_wq = NULL;

// Define work handler
static void my_work_handler(struct work_struct *work)
{
    struct my_device *dev = container_of(work, struct my_device, work);
    printk(KERN_INFO "Work queue processing for device %d\n", dev->id);
    
    // Can sleep here!
    msleep(100);
}

// Initialize
INIT_WORK(&dev->work, my_work_handler);

// Or use delayed work
INIT_DELAYED_WORK(&dev->dwork, my_work_handler);
schedule_delayed_work(&dev->dwork, HZ/2);  // 500ms delay

// Queue work
queue_work(my_wq, &dev->work);
```

### Workqueue Creation

```c
// Create workqueue
my_wq = create_workqueue("my_wq");  // Per-CPU
// OR
my_wq = create_singlethread_workqueue("my_wq");  // Single thread

// Create system workqueue (use sparingly)
schedule_work(&dev->work);  // Uses system workqueue

// Cleanup
destroy_workqueue(my_wq);
```

## Complete Example: Button Handler with Debouncing

```c
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>

#define GPIO_BUTTON 27
#define DEBOUNCE_DELAY (HZ/10)  // 100ms

struct button_device {
    int gpio;
    int irq;
    struct workqueue_struct *wq;
    struct delayed_work debounce_work;
    unsigned long last_press;
    int press_count;
};

static struct button_device btn_dev = {
    .gpio = GPIO_BUTTON,
    .press_count = 0,
};

// Debounced work handler
static void button_debounce_handler(struct work_struct *work)
{
    struct button_device *dev = container_of(work, struct button_device, debounce_work.work);
    int level = gpio_get_value(dev->gpio);
    
    printk(KERN_INFO "Button debounced - GPIO level: %d\n", level);
    
    if (!level) {
        dev->press_count++;
        printk(KERN_INFO "Button press #%d\n", dev->press_count);
    }
}

// Fast interrupt handler
static irqreturn_t button_irq_handler(int irq, void *dev_id)
{
    struct button_device *dev = (struct button_device *)dev_id;
    unsigned long now = jiffies;
    
    // Check minimum time since last interrupt
    if (time_before(now, dev->last_press + DEBOUNCE_DELAY)) {
        return IRQ_HANDLED;  // Ignore bounce
    }
    
    dev->last_press = now;
    
    // Schedule deferred processing
    queue_delayed_work(dev->wq, &dev->debounce_work, DEBOUNCE_DELAY);
    
    return IRQ_HANDLED;
}

// Module init
static int __init button_init(void)
{
    int ret;
    
    // Request GPIO
    ret = gpio_request(btn_dev.gpio, "button");
    if (ret < 0) {
        printk(KERN_ERR "Failed to request GPIO\n");
        return ret;
    }
    
    gpio_direction_input(btn_dev.gpio);
    
    // Get IRQ from GPIO
    btn_dev.irq = gpio_to_irq(btn_dev.gpio);
    
    // Create workqueue
    btn_dev.wq = create_singlethread_workqueue("button_wq");
    if (!btn_dev.wq) {
        printk(KERN_ERR "Failed to create workqueue\n");
        gpio_free(btn_dev.gpio);
        return -ENOMEM;
    }
    
    // Initialize delayed work
    INIT_DELAYED_WORK(&btn_dev.debounce_work, button_debounce_handler);
    
    // Request IRQ (falling edge)
    ret = request_irq(btn_dev.irq, button_irq_handler,
                     IRQF_TRIGGER_FALLING, "button_irq", &btn_dev);
    if (ret < 0) {
        printk(KERN_ERR "Failed to request IRQ\n");
        destroy_workqueue(btn_dev.wq);
        gpio_free(btn_dev.gpio);
        return ret;
    }
    
    printk(KERN_INFO "Button driver loaded (GPIO %d, IRQ %d)\n", btn_dev.gpio, btn_dev.irq);
    return 0;
}

// Module cleanup
static void __exit button_exit(void)
{
    free_irq(btn_dev.irq, &btn_dev);
    
    // Flush all pending work
    flush_workqueue(btn_dev.wq);
    destroy_workqueue(btn_dev.wq);
    
    gpio_free(btn_dev.gpio);
    
    printk(KERN_INFO "Button driver unloaded (total presses: %d)\n", btn_dev.press_count);
}

module_init(button_init);
module_exit(button_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Kernel Student");
MODULE_DESCRIPTION("Button handler with workqueue debouncing");
```

## Makefile

```makefile
obj-m += button_defer.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
 make -C $(KDIR) M=$(PWD) modules

clean:
 make -C $(KDIR) M=$(PWD) clean

test: all
 sudo insmod button_defer.ko
 @echo "Module loaded. Watching kernel log..."
 @echo "Press your button now..."
 dmesg | tail -10
 sudo rmmod button_defer
 dmesg | tail -5
```

## Build and Test Instructions

```bash
# Build module
make

# Install
sudo insmod button_defer.ko

# Monitor kernel messages
dmesg -f | grep -i button &

# Test by pressing button on GPIO 27

# Check logs
cat /proc/interrupts | grep button

# Uninstall
sudo rmmod button_defer

# View final statistics
dmesg | tail -20
```

## How It Works

1. **IRQ Fires**: Button generates interrupt (falling edge)
2. **Fast Handler**: Checks debounce time, schedules work if valid
3. **Work Queue**: Deferred handler waits 100ms to check stable level
4. **Confirmation**: If still low after debounce, increment counter
5. **User Notification**: Can wake up waiting processes

This pattern prevents bounce noise from generating spurious events.

## Key Concepts Summary

| Concept | Purpose |
|---------|---------|
| Tasklet | Fast, lightweight, non-sleeping deferred work |
| Workqueue | Flexible, sleepable deferred work with full process context |
| Delayed Work | Schedule work to run after delay (tasklet or workqueue) |
| Debounce | Ignore rapid successive events from bouncing hardware |
| Top-Half | Minimize time in interrupt context |
| Bottom-Half | Execute heavy processing outside interrupt context |

## Important Notes

1. **Tasklets are per-CPU**: Two tasklets with same code run simultaneously on different CPUs
2. **Workqueues are synchronized**: Default behavior serializes work on same workqueue
3. **Work vs Delayed Work**: Use delayed_work for rate-limiting and debouncing
4. **Cleanup is Critical**: Always flush/destroy workqueues before module exit
5. **Don't block in interrupt**: If you need to sleep, use workqueue, not tasklet
6. **Memory allocation**: Prefer pre-allocated structures to avoid allocation in handlers
