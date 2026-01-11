# 14 Blocking I/O - Assignment Answers and Tasks

## Task 1: Understand Wait Queues

1. **What is a wait queue and why is it needed?**

2. **What does wait_event_interruptible() do?**

3. **What's the difference between wake_up() and wake_up_interruptible()?**

4. **Why must you check the return value of wait_event_interruptible()?**

5. **What would happen if you called wake_up() twice?**

## Task 2: Add Timeout Support

Extend the driver to use `wait_event_interruptible_timeout()` so reads timeout after 5 seconds:

**Key Requirements:**

- Use `wait_event_interruptible_timeout()` instead of `wait_event_interruptible()`
- Return 0 (EOF) on timeout
- Handle both signal and timeout returns properly

## Task 3: Multiple Readers on Same Wait Queue

Create a driver that:

1. Allows multiple processes to sleep on the same wait queue
2. When data arrives, all readers wake up
3. Only the first reader gets the data, others go back to sleep
