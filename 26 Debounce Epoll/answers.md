# Section 26: Debounce Handling � Assignment

## Task 1: Quiz

**1. What problem does debounce fix?**
Mechanical switches bounce/oscillate for milliseconds when pressed, causing multiple false interrupts. Debounce waits for signal stability before accepting state changes.

**2. What unit is jiffies in?**
Jiffies are kernel timer ticks. The duration depends on CONFIG_HZ: at HZ=100, 1 jiffy = 10ms; at HZ=1000, 1 jiffy = 1ms.

**3. How does epoll() differ from poll()?**
Both wait for events on multiple file descriptors. epoll() is more efficient for many fds (uses O(1) lookup), while poll() is O(n). epoll() requires separate epoll_ctl() calls to register fds.

**4. Why do we use O_NONBLOCK with epoll?**
O_NONBLOCK ensures read/write calls don't block when no data is available. epoll_wait() already handles blocking, so operations inside the loop must be non-blocking to avoid deadlock.

**5. Can you combine /dev/ttyS0 and /dev/gpiobtn in one epoll_wait()?**
Yes, add both file descriptors to the same epoll set with epoll_ctl(). epoll_wait() returns all ready fds, and you check which one triggered the event.

---

## Task 2: Make Debounce Configurable

Add a debounce_ms sysfs attribute

User can write 30 to change debounce to 30 ms

Update logic to use this value

**Kernel Driver with Debounce:**
```c
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/device.h>

static unsigned long last_press_jiffies;
static unsigned int debounce_ms = 50;
static unsigned int press_count = 0;
static int data_ready = 0;

static void btn_work_handler(struct work_struct *work)
{
	unsigned long now = jiffies;

	if (time_before(now, last_press_jiffies + msecs_to_jiffies(debounce_ms))) {
		pr_info("[gpiobtn] Ignored bounce\n");
		return;
	}

	last_press_jiffies = now;
	press_count++;
	data_ready = 1;
	wake_up_interruptible(&wq);
	gpiod_set_value(led_gpiod, !gpiod_get_value(led_gpiod));

	pr_info("[gpiobtn] Press %d (debounced)\n", press_count);
}

static ssize_t debounce_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", debounce_ms);
}

static ssize_t debounce_store(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t count)
{
	int val;
	if (sscanf(buf, "%d", &val) == 1 && val > 0 && val < 500) {
		debounce_ms = val;
		pr_info("Debounce set to %d ms\n", debounce_ms);
		return count;
	}
	return -EINVAL;
}

static DEVICE_ATTR(debounce_ms, 0644, debounce_show, debounce_store);
```

**How it works:**
- `last_press_jiffies`: Tracks when last valid press occurred
- `time_before()`: Compares jiffies to check if enough time passed
- `msecs_to_jiffies()`: Converts milliseconds to kernel ticks
- Events within debounce window are ignored
- One press = one counted event

## Task 3: Monitor Multiple Devices

Simulate 2 buttons with:

/dev/gpiobtn0

/dev/gpiobtn1

Use one epoll_wait() to monitor both

Log which button was pressed

**Userspace Code:**
```c
#include <stdio.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>

int main()
{
	int efd = epoll_create(10);
	if (efd < 0) {
		perror("epoll_create");
		return 1;
	}

	int fd0 = open("/dev/gpiobtn0", O_RDONLY | O_NONBLOCK);
	int fd1 = open("/dev/gpiobtn1", O_RDONLY | O_NONBLOCK);

	if (fd0 < 0 || fd1 < 0) {
		perror("open");
		return 1;
	}

	struct epoll_event ev0, ev1, events[10];
	ev0.events = EPOLLIN;
	ev0.data.fd = fd0;
	ev1.events = EPOLLIN;
	ev1.data.fd = fd1;

	epoll_ctl(efd, EPOLL_CTL_ADD, fd0, &ev0);
	epoll_ctl(efd, EPOLL_CTL_ADD, fd1, &ev1);

	printf("Monitoring /dev/gpiobtn0 and /dev/gpiobtn1...\n");

	while (1) {
		int ret = epoll_wait(efd, events, 10, 5000);

		if (ret == 0) {
			printf("Timeout\n");
			continue;
		}

		for (int i = 0; i < ret; i++) {
			char buf[10];
			read(events[i].data.fd, buf, sizeof(buf));

			if (events[i].data.fd == fd0)
				printf("Button 0 pressed\n");
			else if (events[i].data.fd == fd1)
				printf("Button 1 pressed\n");
		}
	}

	close(fd0);
	close(fd1);
	close(efd);
	return 0;
}
```
