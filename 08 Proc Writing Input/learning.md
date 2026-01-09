# Writing to the Kernel Using /proc – Taking Input from User Space

So far, you've sent data from kernel to user space using `/proc`. Now you'll go one step further — you'll allow the user space to send data into the kernel.

## Learning Objectives

By the end of this, you'll:

- Create a writable `/proc` entry that accepts input from user space
- Read and process input data using `copy_from_user()`
- Store and use the received data to change kernel behavior
- Understand the importance of proper buffer management and null-termination

This is how real vendors implement driver configuration interfaces, logging controls, and feature toggles.

## Why Is Writing to the Kernel Important?

### Use Cases

1. **Configuration without reboot**: Change driver settings on-the-fly (e.g., log levels, thresholds)
2. **Command injection**: Send commands to kernel modules (e.g., "start recording", "flush buffers")
3. **Dynamic tuning**: Adjust performance parameters without recompiling

## How Writing Works in `/proc`

When you echo data to a `/proc` file, the kernel's `proc_write()` callback is triggered. Inside:

1. You receive a **buffer from user space** (in kernel memory space)
2. You use **`copy_from_user()`** to safely transfer it
3. You validate, parse, and apply the data

## Key Concepts Before Coding

| Concept | Explanation |
|---------|-------------|
| `proc_create()` | Creates a `/proc` entry with permissions |
| `.proc_write` | Callback function when user writes to the file |
| `copy_from_user()` | Safely copies data from user space to kernel space |
| `char buffer[]` | Kernel-side storage for received data |
| `ssize_t` | Return type for read/write operations (signed size) |
| Null termination | Critical for string safety in the kernel |
| `simple_read_from_buffer()` | Helper to safely read from kernel buffer to user space |

## Step-by-Step: Create a Writable `/proc` Interface

### Our Goal

Create `/proc/writer` that:

- Accepts text input from user space
- Stores it in a kernel buffer
- Prints it to `dmesg`
- Returns it when read back

### Module Code: `proc_write.c`

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define PROC_NAME "writer"
#define MAX_LEN 100

static char message[MAX_LEN];

// Read from /proc/writer
ssize_t writer_read(struct file *file, char __user *buf, 
                    size_t count, loff_t *ppos)
{
    return simple_read_from_buffer(buf, count, ppos, message, strlen(message));
}

// Write to /proc/writer
ssize_t writer_write(struct file *file, const char __user *buf, 
                     size_t count, loff_t *ppos)
{
    // Prevent buffer overflow
    if (count > MAX_LEN - 1)
        count = MAX_LEN - 1;
    
    // Copy from user space to kernel space
    if (copy_from_user(message, buf, count))
        return -EFAULT;
    
    // Ensure null termination
    message[count] = '\0';
    
    // Log to kernel
    printk(KERN_INFO "Kernel received from user: %s\n", message);
    
    return count;
}

// File operations structure
static const struct file_operations writer_fops = {
    .read  = writer_read,
    .write = writer_write,
};

// Module initialization
static int __init writer_init(void)
{
    proc_create(PROC_NAME, 0666, NULL, &writer_fops);
    printk(KERN_INFO "/proc/%s created\n", PROC_NAME);
    return 0;
}

// Module cleanup
static void __exit writer_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "/proc/%s removed\n", PROC_NAME);
}

module_init(writer_init);
module_exit(writer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Writable /proc interface");
```

### Makefile

```makefile
obj-m += proc_write.o

all:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

### Compile and Load

```bash
make
sudo insmod proc_write.ko
```

### Test It

```bash
# Write to the /proc entry
echo "Hello Kernel!" | sudo tee /proc/writer

# Read it back
cat /proc/writer

# Check kernel log
dmesg | tail -3
```

### Expected Output

```
$ echo "Hello Kernel!" | sudo tee /proc/writer
Hello Kernel!

$ cat /proc/writer
Hello Kernel!

$ dmesg | tail -3
[12345.678] Kernel received from user: Hello Kernel!
[12345.679] /proc/writer created
```

## How It Works (Line by Line)

### 1. `writer_write()` Function

```c
if (count > MAX_LEN - 1)
    count = MAX_LEN - 1;
```

**Why?** Prevents buffer overflow by capping the input size.

```c
if (copy_from_user(message, buf, count))
    return -EFAULT;
```

**Why?** `buf` is in user space; we must use `copy_from_user()`. If it fails (e.g., bad pointer), return `-EFAULT` (fault).

```c
message[count] = '\0';
```

**Why?** Ensures the string is null-terminated so `printk` and `strlen` work safely.

### 2. `writer_read()` Function

```c
return simple_read_from_buffer(buf, count, ppos, message, strlen(message));
```

Safely copies from kernel buffer (`message`) back to user space (`buf`). Handles partial reads and file positioning.

### 3. File Permissions

```c
proc_create(PROC_NAME, 0666, NULL, &writer_fops);
```

`0666` = read/write for all users. Use `0644` for read-only to others, `0600` for root-only.

## Complete Workflow

1. **User writes**: `echo "data" > /proc/writer`
2. **Kernel VFS**: Routes to `proc_write()` callback
3. **Kernel checks**: Size validation, safe copy, null termination
4. **Kernel logs**: Prints to dmesg
5. **User reads**: `cat /proc/writer`
6. **Kernel reads**: Routes to `proc_read()` callback
7. **Kernel returns**: Stored message back to user

## Summary Table

| Step | Action | Code |
|------|--------|------|
| Receive from user | Check size, copy, null-terminate | `copy_from_user()` |
| Store in kernel | Use static buffer | `char message[MAX_LEN]` |
| Log to kernel | Use printk | `printk(KERN_INFO "...")` |
| Return to user | Use safe buffer reader | `simple_read_from_buffer()` |
