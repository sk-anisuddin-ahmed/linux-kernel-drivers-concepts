# Writing to the Kernel Using /proc – Q&A and Assignments

## Assignments

### 1. Quiz

Answer the following questions:

1. **What happens if you don't null-terminate the string after `copy_from_user()`?**
   - `printk()` and string functions will read beyond the buffer, causing kernel oops or garbled output

2. **Why is `copy_from_user()` necessary instead of a direct memcpy?**
   - User-space pointers can be invalid or malicious; `copy_from_user()` safely validates the pointer and handles page faults

3. **What does `0666` permission mean for a `/proc` file?**
   - Read and write for owner (user), group, and others (rw-rw-rw-)

4. **How do you prevent buffer overflow when writing to `/proc`?**
   - Check `count > MAX_LEN - 1` and cap it; always leave 1 byte for null terminator

5. **What is `loff_t *ppos` used for?**
   - It tracks the file position pointer to handle partial reads/writes and seeking

---

### 2. Modify `writer_write()` to Handle a Flag

**Task**: Extend the module to accept `ON` or `OFF` commands that enable/disable a kernel flag.

**Requirements**:

- Accept only "ON" or "OFF" (case-insensitive)
- Store the flag state in a static variable
- Log the state change to dmesg
- Return `-EINVAL` if invalid input is received

**Starter Code**:

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define PROC_NAME "writer"
#define MAX_LEN 10

static int logging_enabled = 0;  // Flag to track state
char message[MAX_LEN];

ssize_t writer_read(struct file *file, char __user *buf, 
                    size_t count, loff_t *ppos)
{
    char status[20];
    int len = snprintf(status, sizeof(status), "logging=%d\n", logging_enabled);
    return simple_read_from_buffer(buf, count, ppos, status, len);
}

ssize_t writer_write(struct file *file, const char __user *buf, 
                     size_t count, loff_t *ppos)
{
    if (count > MAX_LEN - 1)
        count = MAX_LEN - 1;
    
    if (copy_from_user(message, buf, count))
        return -EFAULT;
    
    message[count] = '\0';
    
    // TODO: Parse "ON" or "OFF" and set logging_enabled
    // TODO: Log state change
    // TODO: Return -EINVAL for invalid input
    
    return count;
}

static const struct file_operations writer_fops = {
    .read  = writer_read,
    .write = writer_write,
};

static int __init writer_init(void)
{
    proc_create(PROC_NAME, 0666, NULL, &writer_fops);
    printk(KERN_INFO "/proc/%s created (logging initially disabled)\n", PROC_NAME);
    return 0;
}

static void __exit writer_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "/proc/%s removed\n", PROC_NAME);
}

module_init(writer_init);
module_exit(writer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Flag-based /proc control");
```

**Solution Hint**:

- Use `strcmp()` or `strncasecmp()` for string comparison
- Use `printk(KERN_INFO "Logging %s\n", logging_enabled ? "ENABLED" : "DISABLED");`
- Set `logging_enabled = 1` for "ON", `0` for "OFF"

**Test Cases**:

```bash
echo "ON" | sudo tee /proc/writer
cat /proc/writer
# Expected: logging=1

echo "OFF" | sudo tee /proc/writer
cat /proc/writer
# Expected: logging=0

echo "INVALID" | sudo tee /proc/writer
# Should log an error
```

---

### 3. Add Two `/proc` Entries for Command-Response

**Task**: Create a driver with two `/proc` entries that implement a simple command-response interface:

- `/proc/control` - Write commands to the kernel
- `/proc/status` - Read the last status

**Requirements**:

1. **`/proc/control`** (write-only, permission 0200):
   - Accept commands: `START`, `STOP`, `QUERY`
   - Store the state internally
   - Log each command

2. **`/proc/status`** (read-only, permission 0444):
   - Return the current state and timestamp
   - Return format: `State: [IDLE|RUNNING], Last command: [command], Time: [jiffies]`

**Starter Code**:

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

#define MAX_LEN 20

enum state { IDLE, RUNNING };

static enum state current_state = IDLE;
static char last_command[MAX_LEN] = "NONE";
static unsigned long last_time = 0;

// Write to /proc/control
ssize_t control_write(struct file *file, const char __user *buf, 
                      size_t count, loff_t *ppos)
{
    char command[MAX_LEN];
    
    if (count > MAX_LEN - 1)
        count = MAX_LEN - 1;
    
    if (copy_from_user(command, buf, count))
        return -EFAULT;
    
    command[count] = '\0';
    
    // TODO: Parse command and update state
    // TODO: Log command
    // TODO: Update last_command and last_time
    
    return count;
}

// Read from /proc/status
ssize_t status_read(struct file *file, char __user *buf, 
                    size_t count, loff_t *ppos)
{
    char status[128];
    int len;
    
    // TODO: Format status string with current_state, last_command, last_time
    
    return simple_read_from_buffer(buf, count, ppos, status, len);
}

static const struct file_operations control_fops = {
    .write = control_write,
};

static const struct file_operations status_fops = {
    .read = status_read,
};

static int __init cmd_init(void)
{
    proc_create("control", 0200, NULL, &control_fops);
    proc_create("status", 0444, NULL, &status_fops);
    printk(KERN_INFO "/proc/control and /proc/status created\n");
    return 0;
}

static void __exit cmd_exit(void)
{
    remove_proc_entry("control", NULL);
    remove_proc_entry("status", NULL);
    printk(KERN_INFO "/proc entries removed\n");
}

module_init(cmd_init);
module_exit(cmd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Dual /proc command-response interface");
```

**Expected Behavior**:

```bash
# Write command
echo "START" | sudo tee /proc/control
# Logs: "Command received: START"

# Read status
cat /proc/status
# Returns: "State: RUNNING, Last command: START, Time: 123456789"

# Stop
echo "STOP" | sudo tee /proc/control
cat /proc/status
# Returns: "State: IDLE, Last command: STOP, Time: 123456890"
```

**Bonus**: Add error handling for invalid commands (e.g., return `-EINVAL`)

---

## Key Takeaways

| Concept | Key Point |
|---------|-----------|
| `copy_from_user()` | Always use for user→kernel transfers |
| Null termination | Critical for string safety |
| Buffer overflow prevention | Always validate `count` parameter |
| `/proc` permissions | 0200 = write-only, 0444 = read-only, 0666 = read-write |
| `proc_create()` vs `proc_create_data()` | Use `_data()` if you need to pass private data to callbacks |
| `simple_read_from_buffer()` | Simplifies safe kernel→user buffer copies |

---

## Debugging Tips

1. **Module won't load**: Check `dmesg` for kernel oops messages
2. **Permission denied**: Check file permissions with `ls -l /proc/writer`
3. **Data garbled**: Ensure null termination
4. **Buffer overflow warnings**: Use `dmesg | grep -i "overflow"` to spot issues
5. **Input not received**: Verify the write callback is being called with `printk()` logging

---

## Next Steps

- Combine this with character device drivers for more powerful I/O
- Use `/sys` instead of `/proc` for driver attributes
- Implement `ioctl()` for structured commands instead of text parsing
