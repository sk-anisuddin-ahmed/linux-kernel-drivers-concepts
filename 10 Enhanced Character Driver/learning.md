# 10 Enhancing Your Character Driver – Buffers, Parameters, and Thread Safety

## Overview

A basic character driver is great, but production drivers need:

- **Module parameters** — configure behavior when loading (e.g., device name, buffer size)
- **Ring buffers** — store multiple messages without losing data
- **Thread safety** — handle concurrent reads/writes safely

By the end of this section, you will:

- Accept parameters when inserting your module
- Implement a ring buffer for queuing multiple messages
- Use mutex locks to prevent data corruption
- Create a thread-safe character driver

## Why Do We Need These Features?

1. **Module Parameters**: Instead of hardcoding device names or buffer sizes, users can customize them at load time:

   ```bash
   insmod mydriver.ko device_name="smartsensor" buffer_size=512
   ```

2. **Ring Buffers**: Store multiple messages in a circular queue. If the buffer fills up, new messages overwrite the oldest ones (controlled behavior instead of data loss).

3. **Mutex Locks**: When multiple processes access `/dev/mydevice` simultaneously, a mutex ensures that only one accesses kernel data structures at a time, preventing corruption.

## Feature 1: Module Parameters

You can make your module configurable using `module_param()`:

```c
#include <linux/moduleparam.h>

static char device_name[64] = "mychardev";
static int buffer_size = 1024;

module_param(device_name, charp, 0644);
module_param(buffer_size, int, 0644);

MODULE_PARM_DESC(device_name, "Name of the device");
MODULE_PARM_DESC(buffer_size, "Size of the buffer");
```

The third parameter (`0644`) is the sysfs permission — users can read and the owner can write.

**Usage:**

```bash
insmod char_enhanced.ko device_name="sensord" buffer_size=2048
```

## Feature 2: Ring Buffer

A **ring buffer** is a circular queue implemented with an array and two indices:

- **write_index**: Points to the next position to write
- **read_index**: Points to the next position to read

When they're equal, the buffer is empty. When writing catches up to reading, the oldest data is overwritten.

```
Ring Buffer (size 4):
[msg1] [msg2] [msg3] [msg4]
  ↑
read_index = 0
write_index = 0
(buffer empty)

After writing msg1, msg2, msg3:
[msg1] [msg2] [msg3] [___]
  ↑           ↑
read_index   write_index = 3
(3 messages stored)

After writing msg4:
[msg1] [msg2] [msg3] [msg4]
  ↑                        ↑
read_index=0      write_index=0
(buffer full)

If we write again:
[msg5] [msg2] [msg3] [msg4]
  ↑
write_index=1, read_index=0
(oldest message overwritten)
```

## Feature 3: Mutex Locks

A **mutex** (mutual exclusion lock) ensures only one process access a resource at a time:

```c
#include <linux/mutex.h>

static DEFINE_MUTEX(device_mutex);

// To acquire the lock:
mutex_lock(&device_mutex);
// ... access shared data ...
mutex_unlock(&device_mutex);

// For atomic lock/try:
if (mutex_trylock(&device_mutex)) {
    // ... work ...
    mutex_unlock(&device_mutex);
}
```

If two processes call `read()` simultaneously, the first one acquires the lock and reads. The second one sleeps until the first releases the lock.

## Step-by-Step: Enhanced Character Driver

### 1. Create the Enhanced Driver

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <string.h>

#define MAX_MESSAGES 10
#define MAX_MSG_LEN 256

// Module parameters
static char device_name[64] = "mychardev";
static int buffer_size = MAX_MSG_LEN;

module_param_string(device_name, device_name, sizeof(device_name), 0644);
module_param(buffer_size, int, 0644);

MODULE_PARM_DESC(device_name, "Name of the character device");
MODULE_PARM_DESC(buffer_size, "Maximum message size");

// Ring buffer structure
static struct {
    char messages[MAX_MESSAGES][MAX_MSG_LEN];
    int write_index;
    int read_index;
    int count;
} ring_buffer;

static int major;
static DEFINE_MUTEX(device_mutex);

static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[%s] Device opened\n", device_name);
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    char temp[MAX_MSG_LEN];
    int bytes_to_copy;

    mutex_lock(&device_mutex);

    if (ring_buffer.count == 0) {
        mutex_unlock(&device_mutex);
        return 0;  // No data available
    }

    // Get message from read position
    strcpy(temp, ring_buffer.messages[ring_buffer.read_index]);
    bytes_to_copy = strlen(temp) + 1;

    // Move read pointer
    ring_buffer.read_index = (ring_buffer.read_index + 1) % MAX_MESSAGES;
    ring_buffer.count--;

    mutex_unlock(&device_mutex);

    if (bytes_to_copy > len)
        bytes_to_copy = len;

    if (copy_to_user(buf, temp, bytes_to_copy))
        return -EFAULT;

    return bytes_to_copy;
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    char temp[MAX_MSG_LEN];

    if (len > buffer_size - 1)
        len = buffer_size - 1;

    if (copy_from_user(temp, buf, len))
        return -EFAULT;

    temp[len] = '\0';

    mutex_lock(&device_mutex);

    // Store in ring buffer
    strcpy(ring_buffer.messages[ring_buffer.write_index], temp);

    // Move write pointer
    ring_buffer.write_index = (ring_buffer.write_index + 1) % MAX_MESSAGES;

    // Increment count if not full
    if (ring_buffer.count < MAX_MESSAGES) {
        ring_buffer.count++;
    } else {
        // Buffer full, overwrite old data
        ring_buffer.read_index = (ring_buffer.read_index + 1) % MAX_MESSAGES;
    }

    mutex_unlock(&device_mutex);

    printk(KERN_INFO "[%s] Stored message: %s (queue size: %d)\n", 
           device_name, temp, ring_buffer.count);

    return len;
}

static int dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[%s] Device closed\n", device_name);
    return 0;
}

static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = dev_open,
    .read    = dev_read,
    .write   = dev_write,
    .release = dev_release,
};

static int __init char_enhanced_init(void)
{
    major = register_chrdev(0, device_name, &fops);
    if (major < 0) {
        printk(KERN_ALERT "Failed to register character device\n");
        return major;
    }

    // Initialize ring buffer
    ring_buffer.write_index = 0;
    ring_buffer.read_index = 0;
    ring_buffer.count = 0;

    printk(KERN_INFO "Enhanced char driver loaded: major=%d, device=%s, buffer=%d\n", 
           major, device_name, buffer_size);
    return 0;
}

static void __exit char_enhanced_exit(void)
{
    unregister_chrdev(major, device_name);
    printk(KERN_INFO "Enhanced char driver unloaded\n");
}

module_init(char_enhanced_init);
module_exit(char_enhanced_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Enhanced character device with parameters, ring buffer, and mutex");
```

### 2. Makefile

```makefile
obj-m += char_enhanced.o

all:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

### 3. Build and Test

```bash
make
sudo insmod char_enhanced.ko device_name="smartdevice" buffer_size=512
dmesg | tail
sudo mknod /dev/smartdevice c <major> 0
sudo chmod 666 /dev/smartdevice
```

**Write multiple messages:**

```bash
echo "msg1" > /dev/smartdevice
echo "msg2" > /dev/smartdevice
echo "msg3" > /dev/smartdevice
```

**Read them back:**

```bash
cat /dev/smartdevice  # msg1
cat /dev/smartdevice  # msg2
cat /dev/smartdevice  # msg3
```

**Check dmesg:**

```bash
dmesg | tail -20
```

## Key Concepts

### Module Parameters

```c
module_param(name, type, perm);
```

- `type`: `int`, `charp` (string), `bool`, etc.
- `perm`: sysfs file permissions (0644 = readable by all, writable by owner)
- Accessible from `/sys/module/<module_name>/parameters/`

### Ring Buffer Benefits

1. **Fixed memory**: Size doesn't grow dynamically
2. **FIFO behavior**: First message written is first read
3. **Automatic overwrite**: Oldest data discarded when buffer fills
4. **No fragmentation**: Circular structure stays compact

### Mutex Protection

Mutex functions:

- `mutex_lock()`: Sleep until lock acquired
- `mutex_unlock()`: Release lock
- `mutex_trylock()`: Try to acquire, returns 1 if success, 0 if already held

Use mutex around any access to shared kernel data structures.

## Advantages of These Enhancements

1. **Flexibility**: Users can customize driver behavior without recompiling
2. **Reliability**: Ring buffer preserves multiple messages safely
3. **Robustness**: Mutex prevents race conditions and data corruption
4. **Scalability**: Can handle concurrent access from multiple user processes
5. **Production-ready**: These patterns are used in real Linux drivers

## Important Notes

1. **Always pair `mutex_lock()` with `mutex_unlock()`**: Use them symmetrically to prevent deadlocks
2. **Don't hold locks during `copy_to_user()` or `copy_from_user()`**: These can sleep; release the lock first if possible
3. **Ring buffer wraparound**: Use modulo (`%`) to calculate circular indices
4. **String safety**: Use `strcpy()` carefully or switch to `strncpy()` for production
5. **Test concurrency**: Use multiple processes writing/reading simultaneously to verify thread safety
6. **Validate buffer sizes**: Check `len` against `buffer_size` before copying
