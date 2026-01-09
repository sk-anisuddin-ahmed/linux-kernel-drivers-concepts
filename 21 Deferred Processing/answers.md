# Section 21: Deferred Processing – Assignment

## Task 1: Theory Questions

1. **Difference between Tasklets and Workqueues**: Tasklets run in softirq context (cannot sleep), while workqueues run in kthread context (can sleep). Use tasklets for light work, workqueues for heavy operations involving I/O or memory allocation.

2. **Why Top-Half / Bottom-Half?**: Interrupt handlers must complete quickly to avoid blocking other interrupts. The top-half (handler) saves minimal state; bottom-half processes data without blocking.

3. **Delayed Work vs Immediate Work**: `schedule_work()` executes ASAP; `schedule_delayed_work()` waits for specified jiffies. Use delayed work for debouncing and rate-limiting.

4. **Workqueue Types**: `create_workqueue()` creates per-CPU worker threads (parallel on multi-core); `create_singlethread_workqueue()` uses single thread (serialized); `system_wq` is shared system queue.

5. **Flushing Work**: `flush_workqueue()` waits for all pending work to complete; necessary before module exit or device removal to prevent use-after-free.

## Task 2: Debounced LED Control

Implement a workqueue-based LED driver that toggles an LED with debounced button press:

```c
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>

#define GPIO_BUTTON 27
#define GPIO_LED 17
#define DEBOUNCE_DELAY (HZ/20)  // 50ms

struct led_device {
    int button_gpio;
    int led_gpio;
    int button_irq;
    int led_state;
    struct workqueue_struct *wq;
    struct delayed_work toggle_work;
    unsigned long last_press;
};

static struct led_device led_dev = {
    .button_gpio = GPIO_BUTTON,
    .led_gpio = GPIO_LED,
    .led_state = 0,
};

static void led_toggle_handler(struct work_struct *work)
{
    struct led_device *dev = container_of(work, struct led_device, toggle_work.work);
    int button_level = gpio_get_value(dev->button_gpio);
    
    if (!button_level && !dev->led_state) {
        gpio_set_value(dev->led_gpio, 1);
        dev->led_state = 1;
        printk(KERN_INFO "LED ON\n");
    } else if (button_level && dev->led_state) {
        gpio_set_value(dev->led_gpio, 0);
        dev->led_state = 0;
        printk(KERN_INFO "LED OFF\n");
    }
}

static irqreturn_t button_handler(int irq, void *dev_id)
{
    struct led_device *dev = (struct led_device *)dev_id;
    unsigned long now = jiffies;
    
    if (time_before(now, dev->last_press + DEBOUNCE_DELAY)) {
        return IRQ_HANDLED;
    }
    
    dev->last_press = now;
    queue_delayed_work(dev->wq, &dev->toggle_work, DEBOUNCE_DELAY);
    
    return IRQ_HANDLED;
}

static int __init led_init(void)
{
    int ret;
    
    gpio_request(led_dev.button_gpio, "button");
    gpio_direction_input(led_dev.button_gpio);
    
    gpio_request(led_dev.led_gpio, "led");
    gpio_direction_output(led_dev.led_gpio, 0);
    
    led_dev.button_irq = gpio_to_irq(led_dev.button_gpio);
    led_dev.wq = create_singlethread_workqueue("led_wq");
    
    INIT_DELAYED_WORK(&led_dev.toggle_work, led_toggle_handler);
    
    request_irq(led_dev.button_irq, button_handler,
               IRQF_TRIGGER_BOTH, "button_irq", &led_dev);
    
    printk(KERN_INFO "LED driver initialized\n");
    return 0;
}

static void __exit led_exit(void)
{
    free_irq(led_dev.button_irq, &led_dev);
    flush_workqueue(led_dev.wq);
    destroy_workqueue(led_dev.wq);
    gpio_set_value(led_dev.led_gpio, 0);
    gpio_free(led_dev.led_gpio);
    gpio_free(led_dev.button_gpio);
    
    printk(KERN_INFO "LED driver unloaded\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LED control with debounced workqueue");
```

**Expected Output**:

```
LED driver initialized
Button pressed:
LED ON
Button released:
LED OFF
```

## Task 3: High-Load Sensor with Delayed Processing

Implement a sensor driver that buffers readings in workqueue to avoid interrupt context memory allocation:

```c
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>

#define SAMPLE_COUNT 256

struct sensor_device {
    struct workqueue_struct *wq;
    struct delayed_work process_work;
    int sensor_samples[SAMPLE_COUNT];
    int sample_index;
    int total_readings;
};

static struct sensor_device sensor_dev = {
    .sample_index = 0,
    .total_readings = 0,
};

static void sensor_process_handler(struct work_struct *work)
{
    struct sensor_device *dev = container_of(work, struct sensor_device, process_work.work);
    int i, sum = 0;
    
    if (dev->sample_index == 0) {
        printk(KERN_INFO "No samples to process\n");
        return;
    }
    
    for (i = 0; i < dev->sample_index; i++) {
        sum += dev->sensor_samples[i];
    }
    
    int average = sum / dev->sample_index;
    printk(KERN_INFO "Processed %d samples, average: %d\n", dev->sample_index, average);
    
    dev->sample_index = 0;
    dev->total_readings += SAMPLE_COUNT;
}

static irqreturn_t sensor_irq_handler(int irq, void *dev_id)
{
    struct sensor_device *dev = (struct sensor_device *)dev_id;
    static int sensor_value = 500;
    
    if (dev->sample_index < SAMPLE_COUNT) {
        dev->sensor_samples[dev->sample_index++] = sensor_value;
        sensor_value = (sensor_value + 100) % 1000;
        
        if (dev->sample_index >= SAMPLE_COUNT) {
            queue_delayed_work(dev->wq, &dev->process_work, HZ/10);
        }
    }
    
    return IRQ_HANDLED;
}

static int __init sensor_init(void)
{
    dev->wq = create_workqueue("sensor_wq");
    INIT_DELAYED_WORK(&sensor_dev.process_work, sensor_process_handler);
    
    printk(KERN_INFO "Sensor workqueue driver loaded\n");
    return 0;
}

static void __exit sensor_exit(void)
{
    cancel_delayed_work_sync(&sensor_dev.process_work);
    destroy_workqueue(sensor_dev.wq);
    
    printk(KERN_INFO "Sensor driver unloaded (total readings: %d)\n", sensor_dev.total_readings);
}

module_init(sensor_init);
module_exit(sensor_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("High-load sensor with workqueue processing");
```

**Key Pattern**: Interrupt handler fills buffer quickly; workqueue processes data without blocking other interrupts.
