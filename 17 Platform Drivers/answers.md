# 17 Platform Drivers - Assignment Answers and Tasks

## Task 1: Theory Questions

1. **What is a platform driver?**

2. **How does name matching work?**

3. **What are platform resources?**

4. **What does ioremap() do?**

5. **When is probe() called?**

## Task 2: Implement a Platform Driver

Create a platform driver for a simulated sensor device that:

1. Reads from a memory-mapped register at base+0x00 (temperature)
2. Writes control commands to base+0x04

**Key Requirements:**

- Use platform_get_resource() to access hardware resources
- Use ioremap() to map physical to virtual addresses
- Handle probe() and remove() lifecycle

## Task 3: Register Device from Board Code

Create board initialization code that registers the platform device
