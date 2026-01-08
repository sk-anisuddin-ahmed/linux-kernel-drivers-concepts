# 10 Enhanced Character Driver - Assignment Answers and Tasks

## Task 1: Concept Questions

1. **What is a ring buffer and what problem does it solve?**
   - A ring buffer is a circular queue implemented with an array and two indices (read and write). It solves the problem of storing multiple messages of bounded size without dynamic memory allocation. When full, new writes overwrite the oldest messages in a controlled way.

2. **Why do we need mutex locks in character drivers?**
   - Multiple user processes can access the same device simultaneously. Without locks, one process might read while another writes, causing data corruption or inconsistent state. A mutex ensures only one process accesses the kernel data structure at a time.

3. **What advantage do module parameters provide?**
   - Users can customize driver behavior (device name, buffer size, etc.) without modifying and recompiling source code. Simply pass parameters when loading: `insmod driver.ko param1=value1 param2=value2`.

4. **How does the ring buffer handle overflow when all slots are filled?**
   - The write pointer continues to advance and overwrites the oldest message. The read pointer is also advanced to skip over overwritten data, maintaining FIFO order but dropping the oldest message when new data arrives.

5. **What's the difference between `mutex_lock()` and `mutex_trylock()`?**
   - `mutex_lock()` blocks (sleeps) until the lock is acquired. `mutex_trylock()` returns immediately: 1 if lock acquired, 0 if already held. Use `trylock` when you can't afford to sleep or need non-blocking behavior.

## Task 2: Add Dynamic Buffer Allocation

Modify the driver to:

1. Accept an integer module parameter `max_messages`
2. Use `kmalloc()` to dynamically allocate the message buffer
3. Free it in the exit function with `kfree()`

**Key Requirements:**

- Allocate space for `max_messages` messages, each `MAX_MSG_LEN` bytes
- Handle allocation failure gracefully
- Clean up properly on exit

**Sample Solution:**

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <string.h>

#define MAX_MSG_LEN 256

// Module parameters
static char device_name[64] = "mychardev";
static int buffer_size = MAX_MSG_LEN;
static int max_messages = 10;

module_param_string(device_name, device_name, sizeof(device_name), 0644);
module_param(buffer_size, int, 0644);
module_param(max_messages, int, 0644);

MODULE_PARM_DESC(device_name, "Name of the character device");
MODULE_PARM_DESC(buffer_size, "Maximum message size");
MODULE_PARM_DESC(max_messages, "Maximum number of queued messages");

// Dynamic ring buffer structure
static struct {
    char **messages;              // Pointer to dynamically allocated array
    int write_index;
    int read_index;
    int count;
    int max_slots;               // Dynamic size
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
    char *temp;
    int bytes_to_copy;

    mutex_lock(&device_mutex);

    if (ring_buffer.count == 0) {
        mutex_unlock(&device_mutex);
        return 0;
    }

    // Get message from read position
    temp = ring_buffer.messages[ring_buffer.read_index];
    bytes_to_copy = strlen(temp) + 1;

    // Move read pointer
    ring_buffer.read_index = (ring_buffer.read_index + 1) % ring_buffer.max_slots;
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
    ring_buffer.write_index = (ring_buffer.write_index + 1) % ring_buffer.max_slots;

    // Increment count if not full
    if (ring_buffer.count < ring_buffer.max_slots) {
        ring_buffer.count++;
    } else {
        ring_buffer.read_index = (ring_buffer.read_index + 1) % ring_buffer.max_slots;
    }

    mutex_unlock(&device_mutex);

    printk(KERN_INFO "[%s] Message stored (queue: %d/%d)\n", 
           device_name, ring_buffer.count, ring_buffer.max_slots);

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
    int i;

    major = register_chrdev(0, device_name, &fops);
    if (major < 0) {
        printk(KERN_ALERT "Failed to register character device\n");
        return major;
    }

    // Allocate array of message pointers
    ring_buffer.messages = kmalloc(max_messages * sizeof(char*), GFP_KERNEL);
    if (!ring_buffer.messages) {
        unregister_chrdev(major, device_name);
        printk(KERN_ALERT "Failed to allocate message buffer\n");
        return -ENOMEM;
    }

    // Allocate each message buffer
    for (i = 0; i < max_messages; i++) {
        ring_buffer.messages[i] = kmalloc(buffer_size, GFP_KERNEL);
        if (!ring_buffer.messages[i]) {
            // Cleanup partial allocation
            while (--i >= 0)
                kfree(ring_buffer.messages[i]);
            kfree(ring_buffer.messages);
            unregister_chrdev(major, device_name);
            printk(KERN_ALERT "Failed to allocate message slot %d\n", i);
            return -ENOMEM;
        }
    }

    // Initialize ring buffer
    ring_buffer.write_index = 0;
    ring_buffer.read_index = 0;
    ring_buffer.count = 0;
    ring_buffer.max_slots = max_messages;

    printk(KERN_INFO "Enhanced driver loaded: major=%d, max_messages=%d, msg_size=%d\n", 
           major, max_messages, buffer_size);
    return 0;
}

static void __exit char_enhanced_exit(void)
{
    int i;

    // Free each message buffer
    for (i = 0; i < ring_buffer.max_slots; i++)
        kfree(ring_buffer.messages[i]);

    // Free the array of pointers
    kfree(ring_buffer.messages);

    unregister_chrdev(major, device_name);
    printk(KERN_INFO "Enhanced driver unloaded\n");
}

module_init(char_enhanced_init);
module_exit(char_enhanced_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Enhanced char driver with dynamic buffer");
```

**Test it:**

```bash
make
sudo insmod char_enhanced.ko max_messages=20 buffer_size=512
# Now supports 20 messages instead of hardcoded 10
```

## Task 3: Write a Multi-Message C Application

Create a user-space C program that:

1. Writes 5 messages to `/dev/mydevice`
2. Reads all 5 messages back in order
3. Displays them on screen

**Sample Solution:**

```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
    int fd;
    char write_data[5][100] = {
        "First message",
        "Second message",
        "Third message",
        "Fourth message",
        "Fifth message"
    };
    char read_buffer[256];
    int i;

    // Open the device
    fd = open("/dev/mychardev", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/mychardev");
        return 1;
    }

    printf("=== Writing 5 messages ===\n");
    
    // Write 5 messages
    for (i = 0; i < 5; i++) {
        printf("Writing: %s\n", write_data[i]);
        if (write(fd, write_data[i], strlen(write_data[i])) < 0) {
            perror("Write failed");
            close(fd);
            return 1;
        }
    }

    printf("\n=== Reading 5 messages ===\n");

    // Read all 5 messages back
    for (i = 0; i < 5; i++) {
        ssize_t bytes_read = read(fd, read_buffer, sizeof(read_buffer) - 1);
        if (bytes_read < 0) {
            perror("Read failed");
            close(fd);
            return 1;
        } else if (bytes_read == 0) {
            printf("No more messages\n");
            break;
        }

        read_buffer[bytes_read] = '\0';
        printf("Read message %d: %s\n", i + 1, read_buffer);
    }

    close(fd);
    printf("\n=== Test completed ===\n");

    return 0;
}
```

**Compile and run:**

```bash
gcc -o multi_msg_app multi_msg_app.c
./multi_msg_app
```

**Expected output:**

```
=== Writing 5 messages ===
Writing: First message
Writing: Second message
Writing: Third message
Writing: Fourth message
Writing: Fifth message

=== Reading 5 messages ===
Read message 1: First message
Read message 2: Second message
Read message 3: Third message
Read message 4: Fourth message
Read message 5: Fifth message

=== Test completed ===
```

This demonstrates the complete ring buffer functionality with dynamic allocation and proper message queueing.
