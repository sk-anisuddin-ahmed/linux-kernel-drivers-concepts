# 10 Enhanced Character Driver - Assignment Answers and Tasks

## Task 1: Concept Questions

1. **What is a ring buffer and what problem does it solve?**
   - A circular queue that wraps around when the end is reached and returns to the beginning. Instead of constantly creating new buffers, the same space is reused. This is fast and memory-efficient.

2. **Why do mutex locks need to be used in character drivers?**
   - When multiple people try to write to the same notepad at the same time, the writing gets corrupted. A lock ensures only one process can write at a time. This prevents data corruption when multiple processes use the device together.

3. **What advantage do module parameters provide?**
   - Instead of editing code and recompiling whenever configuration changes are needed, settings can be passed when loading the driver. For example, `insmod mydriver size=100`. This makes testing and configuration straightforward.

4. **How does the ring buffer handle overflow when all slots are filled?**
   - Several options exist: overwrite the oldest data (like recording over old tape), wait for space to free up (slow but safe), or return an error indicating the buffer is full. The choice depends on application requirements.

5. **What's the difference between `mutex_lock()` and `mutex_trylock()`?**
   - `mutex_lock()` is like waiting in line - threads stand there until it is their turn. `mutex_trylock()` is like checking if the line is empty - if it is, proceed; if not, leave and do something else instead of waiting.

## Task 2: Add Dynamic Buffer Allocation

Modify the driver to:

1. Accept an integer module parameter `max_messages`
2. Use `kmalloc()` to dynamically allocate the message buffer
3. Free it in the exit function with `kfree()`

**Key Requirements:**

- Allocate space for `max_messages` messages, each `MAX_MSG_LEN` bytes
- Handle allocation failure gracefully
- Clean up properly on exit

**Solution**

```c
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/slab.h>

#define Dev_Name   "rw_dev"
#define Class_Name "rw_class"

static dev_t devt;
static struct cdev rw_cdev;
static struct class *rw_class;
static struct device *rw_device;

int max_msg = 10;
module_param(max_msg, int, 0644);

static int rcnt, wcnt;
static char *kbuf;

#define print(fmt, ...) pr_info(fmt "\n", ##__VA_ARGS__)

static ssize_t rw_read(struct file *file, char __user *buf,
                       size_t len, loff_t *pos)
{
    size_t avail = max_msg - *pos;
    size_t n;

    if (avail == 0)
        return 0; // EOF

    n = (len > avail) ? avail : len;

    if (copy_to_user(buf, kbuf + *pos, n))
        return -EFAULT;

    *pos += n;
    rcnt += n;
    print("Total read bytes: %d", rcnt);

    return n;
}

static ssize_t rw_write(struct file *file, const char __user *buf,
                        size_t len, loff_t *pos)
{
    size_t avail = max_msg - *pos;
    size_t n;

    if (avail == 0)
        return -ENOSPC;

    n = (len > avail) ? avail : len;

    if (copy_from_user(kbuf + *pos, buf, n))
        return -EFAULT;

    *pos += n;
    wcnt += n;
    print("Total written bytes: %d", wcnt);

    return n;
}

static int rw_open(struct inode *inode, struct file *file)
{
    print("device opened");
    return 0;
}

static int rw_release(struct inode *inode, struct file *file)
{
    print("device closed");
    return 0;
}

static const struct file_operations rw_fops = {
    .owner   = THIS_MODULE,
    .read    = rw_read,
    .write   = rw_write,
    .open    = rw_open,
    .release = rw_release,
};

static int __init dev_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&devt, 0, 1, Dev_Name);
    if (ret < 0)
        return ret;

    cdev_init(&rw_cdev, &rw_fops);
    rw_cdev.owner = THIS_MODULE;

    ret = cdev_add(&rw_cdev, devt, 1);
    if (ret < 0) {
        unregister_chrdev_region(devt, 1);
        return ret;
    }

    rw_class = class_create(THIS_MODULE, Class_Name);
    if (IS_ERR(rw_class)) {
        cdev_del(&rw_cdev);
        unregister_chrdev_region(devt, 1);
        return PTR_ERR(rw_class);
    }

    rw_device = device_create(rw_class, NULL, devt, NULL, Dev_Name);
    if (IS_ERR(rw_device)) {
        class_destroy(rw_class);
        cdev_del(&rw_cdev);
        unregister_chrdev_region(devt, 1);
        return PTR_ERR(rw_device);
    }

    kbuf = kzalloc(max_msg, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    print("module loaded");
    return 0;
}

static void __exit dev_exit(void)
{
    device_destroy(rw_class, devt);
    class_destroy(rw_class);
    cdev_del(&rw_cdev);
    unregister_chrdev_region(devt, 1);
    kfree(kbuf);
    print("module unloaded");
}

MODULE_LICENSE("GPL");

module_init(dev_init);
module_exit(dev_exit);
```

## Task 3: Write a Multi-Message C Application

Create a user-space C program that:

1. Writes 5 messages to `/dev/mydevice`
2. Reads all 5 messages back in order
3. Displays them on screen

**Solution**

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
 int fd = open("/dev/rw_dev", O_RDWR);
 if (fd < 0) { perror("open"); return 1; }
 
 // Write 5 messages
 const char *messages[] = {"Hello", "World", "Linux", "Kernel", "Driver"};
 for (int i = 0; i < 5; i++) {
  write(fd, messages[i], strlen(messages[i]));
  printf("Wrote: %s\n", messages[i]);
 }
 
 // Reset file pointer
 lseek(fd, 0, SEEK_SET);
 
 // Read and display
 char buf[256];
 int n = read(fd, buf, sizeof(buf) - 1);
 if (n > 0) {
  buf[n] = '\0';
  printf("Read back: %s\n", buf);
 }
 
 close(fd);
 return 0;
}
```
