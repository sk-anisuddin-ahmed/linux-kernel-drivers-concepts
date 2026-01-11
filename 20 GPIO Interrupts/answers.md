# 20 GPIO Interrupts - Assignment Answers and Tasks

## Task 1: Interrupt Theory

1. **What is an IRQ?**

2. **What's the difference between IRQF_TRIGGER_RISING and IRQF_TRIGGER_FALLING?**

3. **Why should interrupt handlers be fast?**

4. **What does gpio_to_irq() do?**

5. **When should you use tasklets instead of doing work in handler?**

## Task 2: Button Interrupt Handler

**Key Requirements:**

- Use gpio_to_irq() to get IRQ number from GPIO
- Register handler with request_irq()
- Specify trigger edge (RISING, FALLING, or BOTH)
- Track button press count in handler

## Task 3: Debounced Button with Workqueue

**Key Requirements:**

- Use workqueue to defer work from interrupt handler
- Implement debounce delay (20-50ms)
- Verify state is stable before processing
