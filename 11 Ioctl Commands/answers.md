# 11 ioctl Device Commands - Assignment Answers and Tasks

## Task 1: Theory Check

1. **What is ioctl and when would you use it instead of read/write?**

2. **How do the macros _IO, _IOR, _IOW, and _IOWR differ?**

3. **What is the "magic number" in an ioctl command?**

4. **Why must you use copy_from_user() and copy_to_user() in ioctl?**

5. **What should you return on an invalid ioctl command?**

## Task 2: Add More Commands

Extend the temperature driver to include:

1. Command to get the threshold (READ)
2. Command to enable/disable alerts (WRITE boolean flag)
3. Command to get alert status (READ)

**Key Requirements:**

- Define 3 new ioctl commands using appropriate macros
- Implement handlers in the ioctl function
- Use a static variable to track alert enable/disable

## Task 3: Write a Complete ioctl Test Application

Create a user-space C program that:

1. Opens `/dev/tempsensor`
2. Sends commands to set threshold, get temperature, enable alerts
3. Reads the temperature via read() and via ioctl()
4. Displays all results
