# 09 Character Device Driver - Assignment Answers and Tasks

## Task 1: Theory Check

1. **What is a major number? What is a minor number?**

2. **What does `register_chrdev()` do?**

3. **What would happen if you don't call `unregister_chrdev()` on exit?**

4. **Why do we use `copy_from_user()` in `write()`?**

5. **Why is `device_buffer[]` important?**

## Task 2: Modify Your Driver

Maintain a counter of how many bytes were written and display it on read:

**Key Requirements:**

1. Track total bytes written across all `write()` calls
2. In `read()`, display: "You wrote <N> bytes last time."

## Task 3: Write a Small C Program

Write a user-space C program to:

- Open `/dev/mychardev`
- Write "Testing from app"
- Read back and print the output
- Close the file
