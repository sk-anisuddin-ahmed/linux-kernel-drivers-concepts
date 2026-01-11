# Writing to the Kernel Using /proc – Q&A and Assignments

## Assignments

### 1. Quiz

Answer the following questions:

1. **What happens if the string is not null-terminated after `copy_from_user()`?**
   String functions like `printk()` and `strcmp()` read beyond the buffer boundary, causing kernel crashes (oops), garbled output, or information leaks.

2. **Why is `copy_from_user()` necessary instead of a direct memcpy?**
   Direct dereferencing of user pointers in kernel code is forbidden for security reasons. `copy_from_user()` safely validates pointers, handles page faults, and prevents user code from corrupting kernel memory.

3. **What does `0666` permission mean for a `/proc` file?**
   It means read and write access (rw-) for owner, group, and others—equivalent to chmod 666.

4. **How is buffer overflow prevented when writing to `/proc`?**
   The count should be checked to ensure `count > MAX_LEN - 1`, capped to the maximum safe size, and null-terminated by setting `message[count] = '\0'` after `copy_from_user()`.

5. **What is the purpose of `loff_t *ppos`?**
   It tracks the current file position for partial reads/writes; it enables `simple_read_from_buffer()` to resume reading from where the previous operation left off.

---

### 2. Modify `writer_write()` to Handle a Flag

**Task**: Extend the module to accept `ON` or `OFF` commands that enable/disable a kernel flag.

**Requirements**:

- Accept only "ON" or "OFF" (case-insensitive)
- Store the flag state in a static variable
- Log the state change to dmesg
- Return `-EINVAL` if invalid input is received

**Solution**:

```c
static int logging_enabled = 0;

ssize_t writer_write(struct file *file, const char __user *buf, 
                     size_t count, loff_t *ppos)
{
    char message[MAX_LEN];
    
    if (count > MAX_LEN - 1)
        count = MAX_LEN - 1;
    
    if (copy_from_user(message, buf, count))
        return -EFAULT;
    
    message[count] = '\0';
    
    if (strncasecmp(message, "ON", 2) == 0) {
        logging_enabled = 1;
        printk(KERN_INFO "Logging ENABLED\n");
    } else if (strncasecmp(message, "OFF", 3) == 0) {
        logging_enabled = 0;
        printk(KERN_INFO "Logging DISABLED\n");
    } else {
        printk(KERN_ERR "Invalid command: %s\n", message);
        return -EINVAL;
    }
    
    return count;
}
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

**Bonus**: Add error handling for invalid commands (e.g., return `-EINVAL`)

**Solution**:

```c
#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/uaccess.h>
#include<linux/proc_fs.h>
#include<linux/seq_file.h>
#include<linux/kdev_t.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/errno.h>
#include<linux/types.h>
#include<linux/string.h>
#include<linux/jiffies.h>

unsigned long stime;

const char *cmd[] = {
 "START",
 "STOP",
 "QUERY"
};

char prev_rec[] = "DEFAULT";
char rec[] = "DEFAULT";

static ssize_t ctrl_store(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
 _Bool matched = 0;
 char temp[] = "DEFAULT";
 u32 len_t = sizeof(temp) < len ? sizeof(temp) : len;
 int ret = copy_from_user(temp, buf, len_t);
 if (ret < 0)
 {
  return -EINVAL;
 }
    temp[len_t - 1] = '\0';
 for (int i = 0; i < 3; i++)
 {
  if (strcmp(cmd[i], temp) == 0)
  {
   pr_info("Received Command: %s", temp);
   matched = 1;
   break;
  }
 }
 if (!matched)
 {
  pr_info("INVALID Command: %s", rec);
  memset(rec, '\0', sizeof(rec));
 }
 else
 {
  strscpy(prev_rec, rec, sizeof(rec));
  strscpy(rec, temp, sizeof(rec));
 }
 return len;
}

static struct proc_ops ctrl_fops = {
 .proc_write = ctrl_store
};

static int stat_show(struct seq_file *file, void *v)
{
 seq_printf(file, "State:%s, Last command: %s, Time: %lu\n", rec, prev_rec, ((jiffies/HZ) - stime));
 return 0;
}

static int stat_open(struct inode *inode, struct file *file)
{
 return single_open(file, stat_show, NULL);
}

static struct proc_ops stat_fops = {
  .proc_open = stat_open,
  .proc_read = seq_read,
  .proc_release = seq_release
};

static int __init my_init(void)
{
 stime = jiffies/HZ;
 proc_create("control", 0200, NULL, &ctrl_fops);
 proc_create("status", 0444, NULL, &stat_fops);
 
 pr_info("Module loaded\n");
 return 0;
}

static void __exit my_exit(void)
{
 remove_proc_entry("control", NULL);
 remove_proc_entry("status", NULL);
 
 pr_info("Module unloaded\n");
}

MODULE_LICENSE("GPL");
module_init(my_init);
module_exit(my_exit);
```

**Output**

```
root@ahmed:/home/ahmed/BitLearnTasks/proc_rw# cd /proc/
root@ahmed:/proc# cat status
State:DEFAULT, Last command: DEFAULT, Time: 16
root@ahmed:/proc# echo 'START' > control
root@ahmed:/proc# cat status
State:START, Last command: DEFAULT, Time: 35
root@ahmed:/proc# echo 'STOP' > control
root@ahmed:/proc# cat status
State:STOP, Last command: START, Time: 47
root@ahmed:/proc#
```

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
