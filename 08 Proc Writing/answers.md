# 08 Proc Writing - Assignment Answers and Tasks

## Task 1: Answer These Short Questions

1. **What does `proc_write()` do?**
   - It is a callback function that the kernel calls whenever a user writes data to the `/proc` file using `echo > /proc/writer` or similar commands. It handles receiving and processing the user input.

2. **Why do we use `copy_from_user()`?**
   - Because user-space pointers cannot be trusted. Using `copy_from_user()` safely transfers data from user-space memory to kernel-space memory, preventing potential kernel panics from invalid pointers or access violations.

3. **What is `0666` permission for?**
   - It's the file permission mode for `/proc/writer`. It means read+write for owner, group, and others. This allows any user to write to the `/proc` file without needing `sudo`.

4. **What happens if you don't null-terminate the string?**
   - String functions like `strlen()` or `printk()` will keep reading memory until they hit a null byte (`\0`). This causes buffer overruns and potential kernel crashes or information disclosure.

5. **What's the difference between `proc_read` and `seq_file`?**
   - `proc_read` is a simple callback that returns data at once. `seq_file` is a helper interface that breaks output into "records" for safe, line-by-line printing to user space. `seq_file` is preferred for large outputs.

## Task 2: Change Your `writer_write()` Function

Modify the `writer_write()` function to:

- If the input is `"ON"` → set a flag `logging_enabled = 1`
- If the input is `"OFF"` → set it to `0`
- In `writer_read()`, show whether logging is ON or OFF

**Sample Solution:**

```c
static int logging_enabled = 0;

ssize_t writer_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    static char status[50];
    snprintf(status, sizeof(status), "Logging is %s\n", 
             logging_enabled ? "ON" : "OFF");
    return simple_read_from_buffer(buf, count, ppos, status, strlen(status));
}

ssize_t writer_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char message[MAX_LEN];
    if (count > MAX_LEN - 1)
        count = MAX_LEN - 1;

    if (copy_from_user(message, buf, count))
        return -EFAULT;

    message[count] = '\0';

    if (strncmp(message, "ON", 2) == 0) {
        logging_enabled = 1;
        printk(KERN_INFO "Logging enabled\n");
    } else if (strncmp(message, "OFF", 3) == 0) {
        logging_enabled = 0;
        printk(KERN_INFO "Logging disabled\n");
    }

    return count;
}
```

**Test it:**

```bash
echo "ON" > /proc/writer
cat /proc/writer
# Output: Logging is ON

echo "OFF" > /proc/writer
cat /proc/writer
# Output: Logging is OFF
```

## Task 3: Add Two `/proc` Entries

Create two separate `/proc` entries:

- `/proc/control` — to receive commands (writable)
- `/proc/status` — to read current state (readable only)

**Key Requirements:**

1. Both entries should use different callbacks
2. `/proc/control` should accept commands like `start`, `stop`, `reset`
3. `/proc/status` should display the current state

**Sample Solution:**

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define MAX_LEN 100

static int device_state = 0;  // 0=stopped, 1=running, 2=error

ssize_t status_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    static char status[100];
    const char *state_str;
    
    if (device_state == 0)
        state_str = "Status: Stopped";
    else if (device_state == 1)
        state_str = "Status: Running";
    else
        state_str = "Status: Error";

    snprintf(status, sizeof(status), "%s\n", state_str);
    return simple_read_from_buffer(buf, count, ppos, status, strlen(status));
}

ssize_t control_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char command[MAX_LEN];
    if (count > MAX_LEN - 1)
        count = MAX_LEN - 1;

    if (copy_from_user(command, buf, count))
        return -EFAULT;

    command[count] = '\0';

    if (strncmp(command, "start", 5) == 0) {
        device_state = 1;
        printk(KERN_INFO "Device started\n");
    } else if (strncmp(command, "stop", 4) == 0) {
        device_state = 0;
        printk(KERN_INFO "Device stopped\n");
    } else if (strncmp(command, "reset", 5) == 0) {
        device_state = 0;
        printk(KERN_INFO "Device reset\n");
    } else {
        printk(KERN_WARNING "Unknown command: %s\n", command);
    }

    return count;
}

static const struct proc_ops status_fops = {
    .proc_read = status_read,
};

static const struct proc_ops control_fops = {
    .proc_write = control_write,
};

static int __init proc_interface_init(void)
{
    proc_create("control", 0666, NULL, &control_fops);
    proc_create("status", 0444, NULL, &status_fops);
    printk(KERN_INFO "Control and status interfaces created\n");
    return 0;
}

static void __exit proc_interface_exit(void)
{
    remove_proc_entry("control", NULL);
    remove_proc_entry("status", NULL);
    printk(KERN_INFO "Interfaces removed\n");
}

module_init(proc_interface_init);
module_exit(proc_interface_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Proc control and status interface");
```

**Test it:**

```bash
echo "start" > /proc/control
cat /proc/status
# Output: Status: Running

echo "stop" > /proc/control
cat /proc/status
# Output: Status: Stopped
```

This builds a **basic command-response interface** similar to real device drivers.
