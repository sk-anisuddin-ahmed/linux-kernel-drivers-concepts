# Section 26: Debounce Handling – Assignment

## Task 1: Theory Questions

1. **Why Debounce**: Mechanical switches bounce, generating multiple edges within milliseconds. Debounce ignores edges during bounce window.

2. **Level Verification**: Sample GPIO level multiple times during debounce delay to confirm stable state before reporting edge.

3. **Hysteresis Pattern**: Track previous level, report edge only on clean transitions. Prevents rapid oscillation from marginal signals.

4. **Timing Trade-offs**: Longer debounce (100ms) catches all bounces but slower response; shorter (10ms) responsive but may miss slow bounces.

5. **Hardware vs Software**: Hardware debounce uses capacitor/Schmitt trigger; software uses timer/workqueue. Software more flexible.

## Task 2: Debounced Counter with Level Verification

Implement robust button counter using multi-sample stability checking:

```c
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

#define GPIO_BUTTON 27
#define DEBOUNCE_DELAY (HZ/20)  // 50ms
#define STABILITY_SAMPLES 3

struct button_device {
    int gpio;
    int irq;
    int prev_level;
    int press_count;
    struct workqueue_struct *wq;
    struct delayed_work debounce_work;
    unsigned long last_interrupt;
};

static struct button_device btn;

static void check_button_stable(struct work_struct *work)
{
    struct button_device *dev = container_of(work, struct button_device, debounce_work.work);
    int level = gpio_get_value(dev->gpio);
    int stable_count = 0;
    int i;
    
    // Verify stability by sampling
    for (i = 0; i < STABILITY_SAMPLES; i++) {
        if (gpio_get_value(dev->gpio) == level) {
            stable_count++;
        }
        udelay(100);
    }
    
    if (stable_count < STABILITY_SAMPLES) {
        printk(KERN_DEBUG "Bouncing, not stable\n");
        return;
    }
    
    // Detect falling edge
    if (dev->prev_level == 1 && level == 0) {
        dev->press_count++;
        printk(KERN_INFO "Button pressed (count=%d)\n", dev->press_count);
    }
    
    dev->prev_level = level;
}

static irqreturn_t button_handler(int irq, void *dev_id)
{
    struct button_device *dev = (struct button_device *)dev_id;
    unsigned long now = jiffies;
    
    if (time_before(now, dev->last_interrupt + DEBOUNCE_DELAY)) {
        return IRQ_HANDLED;
    }
    
    dev->last_interrupt = now;
    queue_delayed_work(dev->wq, &dev->debounce_work, DEBOUNCE_DELAY);
    
    return IRQ_HANDLED;
}

static int __init button_init(void)
{
    gpio_request(btn.gpio, "button");
    gpio_direction_input(btn.gpio);
    btn.prev_level = gpio_get_value(btn.gpio);
    
    btn.irq = gpio_to_irq(btn.gpio);
    btn.wq = create_singlethread_workqueue("btn_wq");
    
    INIT_DELAYED_WORK(&btn.debounce_work, check_button_stable);
    
    request_irq(btn.irq, button_handler, IRQF_TRIGGER_FALLING, "button", &btn);
    
    printk(KERN_INFO "Debounced button loaded\n");
    return 0;
}

static void __exit button_exit(void)
{
    free_irq(btn.irq, &btn);
    flush_workqueue(btn.wq);
    destroy_workqueue(btn.wq);
    gpio_free(btn.gpio);
    
    printk(KERN_INFO "Total button presses: %d\n", btn.press_count);
}

module_init(button_init);
module_exit(button_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Debounced button counter");
```

## Task 3: Edge Detection with State Machine

Implement robust edge detector tracking multiple state transitions:

```c
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>

#define GPIO_INPUT 27

enum edge_type { EDGE_NONE, EDGE_RISING, EDGE_FALLING };

struct edge_detector {
    int gpio;
    int irq;
    int prev_level;
    int curr_level;
    enum edge_type last_edge;
    int rising_count;
    int falling_count;
    struct workqueue_struct *wq;
    struct delayed_work debounce_work;
    struct mutex lock;
};

static struct edge_detector detector;

static int detect_edge(int prev, int curr)
{
    if (prev == 0 && curr == 1) {
        return EDGE_RISING;
    } else if (prev == 1 && curr == 0) {
        return EDGE_FALLING;
    }
    return EDGE_NONE;
}

static void edge_check_handler(struct work_struct *work)
{
    struct edge_detector *dev = container_of(work, struct edge_detector, debounce_work.work);
    int level = gpio_get_value(dev->gpio);
    enum edge_type edge;
    
    dev->curr_level = level;
    edge = detect_edge(dev->prev_level, dev->curr_level);
    
    if (edge != EDGE_NONE) {
        mutex_lock(&dev->lock);
        
        if (edge == EDGE_RISING) {
            dev->rising_count++;
            printk(KERN_INFO "Rising edge (%d total)\n", dev->rising_count);
        } else {
            dev->falling_count++;
            printk(KERN_INFO "Falling edge (%d total)\n", dev->falling_count);
        }
        
        dev->last_edge = edge;
        dev->prev_level = dev->curr_level;
        
        mutex_unlock(&dev->lock);
    }
}

static irqreturn_t edge_handler(int irq, void *dev_id)
{
    struct edge_detector *dev = (struct edge_detector *)dev_id;
    queue_delayed_work(dev->wq, &dev->debounce_work, HZ/20);
    return IRQ_HANDLED;
}

static int __init edge_init(void)
{
    gpio_request(detector.gpio, "input");
    gpio_direction_input(detector.gpio);
    detector.prev_level = gpio_get_value(detector.gpio);
    
    detector.irq = gpio_to_irq(detector.gpio);
    detector.wq = create_singlethread_workqueue("edge_wq");
    
    mutex_init(&detector.lock);
    INIT_DELAYED_WORK(&detector.debounce_work, edge_check_handler);
    
    request_irq(detector.irq, edge_handler, IRQF_TRIGGER_BOTH, "edge", &detector);
    
    printk(KERN_INFO "Edge detector loaded\n");
    return 0;
}

static void __exit edge_exit(void)
{
    free_irq(detector.irq, &detector);
    flush_workqueue(detector.wq);
    destroy_workqueue(detector.wq);
    gpio_free(detector.gpio);
    
    printk(KERN_INFO "Rising: %d, Falling: %d\n", detector.rising_count, detector.falling_count);
}

module_init(edge_init);
module_exit(edge_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Robust edge detector");
```

**Key Pattern**: State machine tracking prev/curr levels, debounce workqueue for stability check, mutex protecting counters.
