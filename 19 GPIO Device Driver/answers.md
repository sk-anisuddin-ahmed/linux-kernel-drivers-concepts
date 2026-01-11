# 19 GPIO Device - Assignment Answers and Tasks

## Task 1: GPIO Theory

1. **What does gpio_request() do?**

2. **Difference between gpio_direction_input() and gpio_direction_output()?**

3. **What does gpio_get_value() return?**

4. **How do you export GPIO to user space?**

5. **When should you use ioctl vs sysfs for GPIO?**

## Task 2: GPIO LED Control Driver

**Key Requirements:**

- Request GPIO pins
- Control pin direction (input/output)
- Set/read GPIO values using gpio_set_value() and gpio_get_value()

## Task 3: GPIO Interrupt Handling

Create driver that detects GPIO state changes via edge-triggered interrupts
