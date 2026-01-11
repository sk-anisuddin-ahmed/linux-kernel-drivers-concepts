# Section 21: Deferred Processing – Assignment

## Task 1: Theory Questions

1. **Difference between Tasklets and Workqueues**:

2. **Why Top-Half / Bottom-Half?**:

3. **Delayed Work vs Immediate Work**:

4. **Workqueue Types**:

5. **Flushing Work**:

## Task 2: Debounced LED Control

Implement a workqueue-based LED driver that toggles an LED with debounced button press:

**Key Requirements:**

- Use delayed_work or work_struct
- Debounce with DEBOUNCE_DELAY constant  
- Toggle LED state on button press
- Flush workqueue on module exit

## Task 3: High-Load Sensor with Delayed Processing

Implement a sensor driver that buffers readings in workqueue to avoid interrupt context memory allocation
