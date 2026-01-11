# 10 Enhanced Character Driver - Assignment Answers and Tasks

## Task 1: Concept Questions

1. **What is a ring buffer and what problem does it solve?**

2. **Why do we need mutex locks in character drivers?**

3. **What advantage do module parameters provide?**

4. **How does the ring buffer handle overflow when all slots are filled?**

5. **What's the difference between `mutex_lock()` and `mutex_trylock()`?**

## Task 2: Add Dynamic Buffer Allocation

Modify the driver to:

1. Accept an integer module parameter `max_messages`
2. Use `kmalloc()` to dynamically allocate the message buffer
3. Free it in the exit function with `kfree()`

**Key Requirements:**

- Allocate space for `max_messages` messages, each `MAX_MSG_LEN` bytes
- Handle allocation failure gracefully
- Clean up properly on exit

## Task 3: Write a Multi-Message C Application

Create a user-space C program that:

1. Writes 5 messages to `/dev/mydevice`
2. Reads all 5 messages back in order
3. Displays them on screen
