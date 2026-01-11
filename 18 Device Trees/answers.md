# 18 Device Trees - Assignment Answers and Tasks

## Task 1: Theory Questions

1. **What is a device tree?**

2. **What are compatible strings?**

3. **How does OF matching work?**

4. **What does MODULE_DEVICE_TABLE(of, table) do?**

5. **How do you read properties?**

## Task 2: Parse Custom Properties

Extend a driver to read multiple properties from device tree

**Key Requirements:**

- Read u32, string, and other property types from device tree nodes
- Use of_property_read_* functions
- Handle missing or invalid properties gracefully

## Task 3: Handle GPIO from Device Tree

Write code that reads GPIO pins from DT and controls them
