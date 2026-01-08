# 08 Writing to the Kernel Using /proc – Taking Input from User Space

## Overview

So far, you've sent **data from kernel to user space** using `/proc`. Now you'll go one step further — you'll allow the **user space to send data into the kernel**.

By the end of this section, you'll be able to:

- Create a writable `/proc` entry
- Read input from user space using `echo` or user programs
- Process that input inside your kernel module
- Use the data to change kernel behavior or print dynamic responses

This is how real kernel drivers receive input from userspace, making your modules **interactive and controllable**.

## Why Is Writing to the Kernel Important?

The kernel is like a **secure brain** that normally hides from users. But sometimes we want to give it hints or control commands from user space.

For example:

- You may want to **turn on/off debug logging** in a driver
- You may want to **send a message** to the kernel from a user-space app
- Or trigger a **device action** — like a soft reset, test, or calibration

Instead of recompiling the module each time, we can **pass input into the kernel dynamically** — and `/proc` makes this possible.

## How Writing Works in `/proc`

A `/proc` file behaves like a regular file. You can:

- `cat /proc/xyz` to read from it
- `echo something > /proc/xyz` to write to it

Inside the kernel, you must:

- Define a `.write` function that runs when a user writes to the file
- Register that handler using the `proc_ops` struct
- Allocate a buffer to store the input
- Process or print the data

Think of it as building your own **command input box** for your kernel module.

## Key Concepts Before Coding

| Concept | Explanation |
|---------|-------------|
| `proc_create()` | Creates a file entry under `/proc` |
| `.proc_write` | A function that is called when user writes to the file |
| `copy_from_user()` | Transfers data from user-space buffer into kernel-space buffer |
| `char buffer[]` | Used to store the user input string |
| `ssize_t` | Return type showing how many bytes were written |

## Step-by-Step: Create a Writable `/proc` Interface

### 1. Create a Module with Write Support

In a new folder `~/proc_write`, create `proc_write.c`:

```bash
mkdir ~/proc_write
cd ~/proc_write
nano proc_write.c
```

Paste this code:

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define PROC_NAME "writer"
#define MAX_LEN 100

static char message[MAX_LEN];

ssize_t writer_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    return simple_read_from_buffer(buf, count, ppos, message, strlen(message));
}

ssize_t writer_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    if (count > MAX_LEN - 1)
        count = MAX_LEN - 1;

    if (copy_from_user(message, buf, count))
        return -EFAULT;

    message[count] = '\0';  // Ensure null termination

    printk(KERN_INFO "Received from user: %s\n", message);
    return count;
}

static const struct proc_ops writer_fops = {
    .proc_read = writer_read,
    .proc_write = writer_write,
};

static int __init proc_write_init(void)
{
    proc_create(PROC_NAME, 0666, NULL, &writer_fops);
    printk(KERN_INFO "/proc/%s created. You can write to it!\n", PROC_NAME);
    return 0;
}

static void __exit proc_write_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "/proc/%s removed.\n", PROC_NAME);
}

module_init(proc_write_init);
module_exit(proc_write_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Proc write demo module");
```

### 2. Create the Makefile

```bash
nano Makefile
```

```makefile
obj-m += proc_write.o

all:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

### 3. Compile, Load, and Test

```bash
make
sudo insmod proc_write.ko
```

Check dmesg:

```bash
dmesg | tail -10
```

Try writing to `/proc/writer`:

```bash
echo "Hello kernel!" | sudo tee /proc/writer
```

Then read back:

```bash
cat /proc/writer
```

You'll see:

```
Hello kernel!
```

And `dmesg` will log:

```
Received from user: Hello kernel!
```

You've now created a **two-way communication bridge** with the kernel!

## How It Works (Line by Line)

- `proc_create()` registers the `/proc/writer` file and links it to our `writer_fops` struct
- `writer_read()` is called when someone does `cat /proc/writer`
- `writer_write()` is called when someone does `echo "text" > /proc/writer`
- `copy_from_user()` brings the data safely into the kernel buffer
- We store the string in `message[]` and print it with `printk()`

You can extend this to:

- Control logging
- Set device states
- Send messages or commands

This is the **foundation of many real-world Linux kernel interfaces**.

## Key Concepts

### `proc_write()` Function

The `proc_write()` callback is invoked whenever user space writes to the `/proc` file. It receives:

- `file`: File structure pointer
- `buf`: User-space buffer containing the data
- `count`: Number of bytes to write
- `ppos`: File position pointer

**Important**: Always use `copy_from_user()` to safely transfer data from user space to kernel space.

### `copy_from_user()` Function

```c
unsigned long copy_from_user(void *to, const void __user *from, unsigned long n)
```

- Copies data from user space to kernel space
- Returns the number of bytes that could NOT be copied (0 on success)
- Prevents kernel crashes from invalid user-space pointers

### Permission Mode: 0666

The permission mode `0666` means:

- Owner (user): read and write
- Group: read and write
- Others: read and write

This allows any user to read and write to `/proc/writer`. In real drivers, use more restrictive permissions like `0644`.

## Advantages of `/proc` Write Interface

1. **Simple**: Easy to implement and understand
2. **Flexible**: Can process any text commands from user space
3. **No special tools needed**: Use standard `echo` command
4. **Synchronous**: Kernel processes data immediately
5. **Scriptable**: Works seamlessly with shell scripts

## Important Notes

1. **Always null-terminate strings**: `message[count] = '\0'` prevents buffer overruns
2. **Validate input length**: Check `count` against buffer size before `copy_from_user()`
3. **Handle copy errors**: Check return value of `copy_from_user()`
4. **Use appropriate permissions**: 0666 is too open for most scenarios
5. **Clean up on exit**: Always call `remove_proc_entry()` in your exit function
