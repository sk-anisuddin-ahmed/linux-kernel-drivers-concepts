# 13 Multiple Minor Devices - Assignment Answers and Tasks

## Task 1: Concept Questions

1. **How many devices can a single driver with one major number manage?**

2. **Why is it more efficient to use minor numbers instead of creating separate drivers?**

3. **How do you extract the minor number in the read/write handlers?**

4. **What happens if you access a minor number that doesn't exist?**

5. **How can you efficiently store data for many devices?**

## Task 2: Add Per-Device Locks

Extend the driver to include a **per-device mutex** to prevent concurrent access to individual devices:

**Key Requirements:**

- Create an array of mutexes, one per device
- Lock in open(), unlock in release() (simple exclusive access model)
- OR: Lock in read/write to protect just the data access
- Handle EBUSY if device is already locked (optional)

## Task 3: Write Test Application

Create a user-space program that:

1. Opens multiple device files simultaneously (using threads or forks)
2. Writes to each device
3. Reads from each device
4. Demonstrates that each device maintains independent state
