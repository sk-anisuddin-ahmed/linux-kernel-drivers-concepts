# Section 26: Debounce Handling and Edge Detection

## Debounce Strategies

### Software Debounce Timer

```c
static void debounce_timer_callback(struct timer_list *t)
{
    struct device *dev = from_timer(dev, t, debounce_timer);
    int level = gpio_get_value(dev->gpio);
    
    if (level != dev->expected_level) {
        dev->state = level;
        wake_up(&dev->wait_queue);
    }
}

static irqreturn_t gpio_irq(int irq, void *dev_id)
{
    struct device *dev = dev_id;
    
    mod_timer(&dev->debounce_timer, jiffies + msecs_to_jiffies(20));
    
    return IRQ_HANDLED;
}
```

### Debounce with Workqueue

```c
static void debounce_work_handler(struct work_struct *work)
{
    struct device *dev = container_of(work, struct device, debounce_work);
    int level = gpio_get_value(dev->gpio);
    int stable = 1;
    
    // Check multiple times
    for (int i = 0; i < 5; i++) {
        msleep(5);
        if (gpio_get_value(dev->gpio) != level) {
            stable = 0;
            break;
        }
    }
    
    if (stable) {
        dev->state = level;
        wake_up(&dev->wait_queue);
    }
}
```

## Edge Detection

```c
#define EDGE_RISING   1
#define EDGE_FALLING  2
#define EDGE_BOTH     3

static int detect_edge(int prev_level, int curr_level, int edge_type)
{
    switch (edge_type) {
        case EDGE_RISING:
            return (prev_level == 0) && (curr_level == 1);
        case EDGE_FALLING:
            return (prev_level == 1) && (curr_level == 0);
        case EDGE_BOTH:
            return prev_level != curr_level;
        default:
            return 0;
    }
}
```

## Complete Example: Robust Button Handler

```c
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
#include <linux/timer.h>

#define GPIO_BUTTON 27
#define DEBOUNCE_MS 20
#define SAMPLE_COUNT 5

struct button_device {
    int gpio;
    int irq;
    int prev_level;
    int curr_level;
    struct workqueue_struct *wq;
    struct delayed_work debounce_work;
    unsigned long last_press;
    int press_count;
};

static struct button_device btn_dev = {
    .gpio = GPIO_BUTTON,
    .prev_level = 1,
    .curr_level = 1,
};

static void button_debounce(struct work_struct *work)
{
    struct button_device *dev = container_of(work, struct button_device, debounce_work.work);
    int stable_level = gpio_get_value(dev->gpio);
    int stable = 1;
    int i;
    
    // Verify level stability
    for (i = 0; i < SAMPLE_COUNT; i++) {
        msleep(DEBOUNCE_MS / SAMPLE_COUNT);
        
        if (gpio_get_value(dev->gpio) != stable_level) {
            stable = 0;
            break;
        }
    }
    
    if (!stable) {
        printk(KERN_DEBUG "Bounce detected\n");
        return;
    }
    
    dev->curr_level = stable_level;
    
    // Detect falling edge (button pressed)
    if ((dev->prev_level == 1) && (dev->curr_level == 0)) {
        dev->press_count++;
        printk(KERN_INFO "Button press #%d\n", dev->press_count);
    }
    
    dev->prev_level = dev->curr_level;
}

static irqreturn_t button_irq_handler(int irq, void *dev_id)
{
    struct button_device *dev = (struct button_device *)dev_id;
    unsigned long now = jiffies;
    
    // Ignore rapid interrupts (within debounce window)
    if (time_before(now, dev->last_press + msecs_to_jiffies(DEBOUNCE_MS))) {
        return IRQ_HANDLED;
    }
    
    dev->last_press = now;
    
    // Schedule deferred debounce check
    queue_delayed_work(dev->wq, &dev->debounce_work, msecs_to_jiffies(DEBOUNCE_MS));
    
    return IRQ_HANDLED;
}

static int __init button_init(void)
{
    int ret;
    
    gpio_request(btn_dev.gpio, "button");
    gpio_direction_input(btn_dev.gpio);
    
    btn_dev.irq = gpio_to_irq(btn_dev.gpio);
    btn_dev.wq = create_singlethread_workqueue("button_wq");
    
    INIT_DELAYED_WORK(&btn_dev.debounce_work, button_debounce);
    
    ret = request_irq(btn_dev.irq, button_irq_handler,
                     IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
                     "button_irq", &btn_dev);
    
    printk(KERN_INFO "Button debounce driver loaded (DEBOUNCE_MS=%d)\n", DEBOUNCE_MS);
    return ret;
}

static void __exit button_exit(void)
{
    free_irq(btn_dev.irq, &btn_dev);
    cancel_delayed_work_sync(&btn_dev.debounce_work);
    destroy_workqueue(btn_dev.wq);
    gpio_free(btn_dev.gpio);
    
    printk(KERN_INFO "Button debounce driver unloaded (total presses: %d)\n", btn_dev.press_count);
}

module_init(button_init);
module_exit(button_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Robust button debounce handler");
```

## Key Concepts

| Technique | Pros | Cons |
|-----------|------|------|
| Timer-based | Simple | Tied to jiffies granularity |
| Workqueue | Flexible timing | More overhead |
| Level checking | Accurate | Needs multiple samples |
| Hysteresis | Immune to noise | Requires calibration |

## Important Notes

1. **Debounce delay** typically 10-50ms for mechanical switches
2. **Edge detection** requires state tracking across IRQ calls
3. **Verify stability** by sampling multiple times during debounce window
4. **Hysteresis** prevents re-triggering on marginal level changes
5. **IRQ throttling** prevents excessive interrupt processing
6. **Test patterns** with slow toggles to verify debounce algorithm
