# Section 24: Device File Operations - Character Device with /dev/gpiobtn

## Quiz Questions

### 1. What is the purpose of cdev_add()?
`cdev_add()` registers a character device with the kernel's device infrastructure, making it discoverable and accessible. It links the device operations structure (file_operations) to the device number, enabling the system to route system calls from userspace to the driver's handler functions.

### 2. How does echo 1 > /dev/gpiobtn reach the driver?
When you execute `echo 1 > /dev/gpiobtn`, the shell opens the device file and calls the driver's `write()` function with the data "1\n". The kernel routes this system call to your registered `write` handler in the file_operations structure.

### 3. Can multiple apps access /dev/gpiobtn concurrently?
Yes, multiple applications can access the device concurrently, but the driver must implement proper synchronization (mutexes, spinlocks) to prevent race conditions when accessing shared hardware or data structures.

### 4. What happens if read() is called without a newline?
The `read()` system call blocks until data is available in the driver's buffer or until the driver signals EOF. Without a newline, if the driver waits for it, the read will hang until the driver explicitly marks the data as ready or the read timeout occurs.

### 5. When should you prefer ioctl() over write()?
Use `ioctl()` for command-based operations that don't involve bulk data transfer (like LED control, configuration, or status queries). `write()` is better for sending data streams, whereas `ioctl()` provides a cleaner interface for hardware control with specific command codes.

---

## Assignment 2: Add LED Control via ioctl()

Define commands:
- LED_ON
- LED_OFF
Let user apps toggle LED via ioctl(fd, LED_ON)

**Kernel Driver:**
```c
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>

#define LED_ON _IOW('L', 0, int)
#define LED_OFF _IOW('L', 1, int)

static dev_t device_number;
static struct cdev device_cdev;
static struct class *device_class;
static int led_state = 0;

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch(cmd) {
		case LED_ON:
			led_state = 1;
			pr_info("LED turned ON\n");
			break;
		case LED_OFF:
			led_state = 0;
			pr_info("LED turned OFF\n");
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = device_ioctl,
};

static int __init led_init(void)
{
	alloc_chrdev_region(&device_number, 0, 1, "gpiobtn");
	cdev_init(&device_cdev, &fops);
	cdev_add(&device_cdev, device_number, 1);
	device_class = class_create("gpiobtn_class");
	device_create(device_class, NULL, device_number, NULL, "gpiobtn");
	return 0;
}

static void __exit led_exit(void)
{
	device_destroy(device_class, device_number);
	class_destroy(device_class);
	cdev_del(&device_cdev);
	unregister_chrdev_region(device_number, 1);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
```

**Userspace Test:**
```c
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define LED_ON _IOW('L', 0, int)
#define LED_OFF _IOW('L', 1, int)

int main(int argc, char *argv[])
{
	int fd = open("/dev/gpiobtn", O_RDWR);
	if (argc > 1) {
		if (argv[1][0] == '1')
			ioctl(fd, LED_ON);
		else if (argv[1][0] == '0')
			ioctl(fd, LED_OFF);
	}
	close(fd);
	return 0;
}
```

## Assignment 3: Multi-Device Registration

- Register /dev/gpiobtn0, /dev/gpiobtn1 with different GPIOs
- Add a minor number loop
- Track press counts per device

**Kernel Driver:**
```c
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>

#define NUM_DEVICES 2
#define GPIO1 17
#define GPIO2 27

struct device_data {
	int gpio;
	int press_count;
	struct cdev cdev;
};

static dev_t device_number;
static struct class *device_class;
static struct device_data devices[NUM_DEVICES];

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct device_data *dev = file->private_data;
	
	if (cmd == 0) {  // READ_COUNT
		dev->press_count++;
		pr_info("Device %s: Press count = %d\n", dev_name(file->f_inode->i_devices), dev->press_count);
	}
	return dev->press_count;
}

static int device_open(struct inode *inode, struct file *file)
{
	int minor = iminor(inode);
	file->private_data = &devices[minor];
	return 0;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.unlocked_ioctl = device_ioctl,
};

static int __init multi_init(void)
{
	int i;
	int gpios[] = {GPIO1, GPIO2};
	
	alloc_chrdev_region(&device_number, 0, NUM_DEVICES, "gpiobtn");
	device_class = class_create("gpiobtn_class");
	
	for (i = 0; i < NUM_DEVICES; i++) {
		devices[i].gpio = gpios[i];
		devices[i].press_count = 0;
		
		cdev_init(&devices[i].cdev, &fops);
		cdev_add(&devices[i].cdev, MKDEV(MAJOR(device_number), i), 1);
		device_create(device_class, NULL, MKDEV(MAJOR(device_number), i), 
			      NULL, "gpiobtn%d", i);
		
		pr_info("Device gpiobtn%d registered (GPIO %d)\n", i, devices[i].gpio);
	}
	return 0;
}

static void __exit multi_exit(void)
{
	int i;
	for (i = 0; i < NUM_DEVICES; i++) {
		device_destroy(device_class, MKDEV(MAJOR(device_number), i));
		cdev_del(&devices[i].cdev);
	}
	class_destroy(device_class);
	unregister_chrdev_region(device_number, NUM_DEVICES);
}

module_init(multi_init);
module_exit(multi_exit);
MODULE_LICENSE("GPL");
```

**Usage:**
```bash
# Load module
insmod multi_dev.ko

# Test devices
./test_dev 0  # Access /dev/gpiobtn0
./test_dev 1  # Access /dev/gpiobtn1

# Each device maintains separate press count
```