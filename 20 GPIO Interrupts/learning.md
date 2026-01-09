# 20 GPIO Interrupts – Hardware Interrupt Handling

Interrupts allow hardware to signal the CPU that an event occurred (button press, data ready, etc). GPIO pins can trigger interrupts on state changes.

## Key Concepts

- Interrupt number (IRQ): CPU identifies handler by IRQ number
- Trigger type: IRQF_TRIGGER_RISING (0→1), FALLING (1→0), BOTH
- Handler: function called when interrupt occurs
- Shared interrupts: Multiple drivers on same IRQ line

## Registering an Interrupt

```c
irqreturn_t my_irq_handler(int irq, void *dev_id)
{
    printk(KERN_INFO "Interrupt %d occurred!\n", irq);
    return IRQ_HANDLED;
}

static int setup_interrupt(int gpio)
{
    int irq = gpio_to_irq(gpio);
    
    request_irq(irq, my_irq_handler, IRQF_TRIGGER_RISING,
               "my_interrupt", NULL);
    
    return irq;
}

static void cleanup_interrupt(int irq)
{
    free_irq(irq, NULL);
}
```

## IRQ Flags

- IRQF_TRIGGER_RISING: Rising edge (low→high)
- IRQF_TRIGGER_FALLING: Falling edge (high→low)
- IRQF_TRIGGER_HIGH: High level
- IRQF_TRIGGER_LOW: Low level
- IRQF_SHARED: Can be shared with other drivers

## Important Notes

1. Interrupt handlers should be fast (no sleep, no blocking)
2. Use tasklets or workqueues for heavy processing
3. Disable interrupts in probe() if needed
4. Always free_irq() in remove()
5. dev_id parameter helps identify shared interrupts
