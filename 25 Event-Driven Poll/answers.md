# Section 25: Event-Driven Poll

## Assignment 1: Quiz

**1. What triggers a poll notification in your driver?**
The driver calls `wake_up_interruptible()` on the wait queue when an event occurs. This wakes sleeping processes and tells the kernel to re-check the poll conditions.

**2. Why is poll_wait() needed inside .poll()?**
`poll_wait()` registers the process in the driver's wait queue so it receives notifications. Without it, the process won't be woken when events occur and will miss updates.

**3. What if you forget to clear data_ready after read?**
The flag stays set, causing poll() to always report data as available even after it's been read. Subsequent reads may get stale or empty data.

**4. What's the difference between poll() and select()?**
Both wait for events on multiple file descriptors. `select()` is older and limited to 1024 fds, while `poll()` has no limit and is more efficient.

**5. Can poll() wait on multiple devices?**
Yes, pass an array of `struct pollfd` entries to `poll()`. It returns when any file descriptor has an event or the timeout occurs.

---

## Assignment 2: Add Timeout Support

**Concept**: Timeouts let userspace avoid blocking forever. Return value from poll() tells you what happened.

**Timeout Return Values**:
- `0`: Timeout occurred (no events)
- `> 0`: Event occurred (check revents bitmask)
- `< 0`: Error (errno set)

**Complete Userspace Code with Timeout:**
```c
#include <stdio.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

int main()
{
	int fd = open("/dev/button", O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	struct pollfd pfd = {fd, POLLIN, 0};
	int ret = poll(&pfd, 1, 5000);

	if (ret == 0)
		printf("No button press within 5 seconds\n");
	else if (ret > 0 && (pfd.revents & POLLIN)) {
		char buf[10];
		read(fd, buf, sizeof(buf));
		printf("Button pressed!\n");
	} else if (ret < 0)
		perror("poll");

	close(fd);
	return 0;
}
```

## Assignment 3: Combine Poll with Ioctl

Add a custom ioctl() to:

Enable/disable poll notifications dynamically

Maintain a notifications_enabled flag

Only notify when it is enabled.

**Kernel Driver - Poll with Ioctl Control:**
```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/wait.h>

#define BUTTON_MAGIC 'B'
#define ENABLE_NOTIFICATIONS _IOW(BUTTON_MAGIC, 1, int)
#define DISABLE_NOTIFICATIONS _IOW(BUTTON_MAGIC, 2, int)

static wait_queue_head_t button_queue;
static int button_pressed = 0, notifications_enabled = 1;

static __poll_t button_poll(struct file *file, poll_table *wait)
{
	poll_wait(file, &button_queue, wait);
	return (notifications_enabled && button_pressed) ? (POLLIN | POLLRDNORM) : 0;
}

static ssize_t button_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	if (!button_pressed) {
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;
		if (wait_event_interruptible(button_queue, button_pressed))
			return -ERESTARTSYS;
	}
	button_pressed = 0;
	return 0;
}

static long button_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch(cmd) {
		case ENABLE_NOTIFICATIONS:
			notifications_enabled = 1;
			wake_up_interruptible(&button_queue);
			break;
		case DISABLE_NOTIFICATIONS:
			notifications_enabled = 0;
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.poll = button_poll,
	.read = button_read,
	.unlocked_ioctl = button_ioctl,
};

static int __init button_init(void)
{
	init_waitqueue_head(&button_queue);
	return 0;
}

static void __exit button_exit(void) { }

module_init(button_init);
module_exit(button_exit);
MODULE_LICENSE("GPL");
```
