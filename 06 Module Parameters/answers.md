# 06 Module Parameters - Assignment Tasks

## Assignment Tasks

**Task 1:** Modify your hello module to accept:

1. A `message` parameter (string)
2. A `repeat_count` parameter (integer)

```bash
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static int repeat_count = 0;
static char *message = "Hello";

module_param(repeat_count, int, 0644);
module_param(message, charp, 0644);

MODULE_LICENSE("GPL");

static int __init msg_init(void)
{
    for (int i = 0; i < repeat_count; i++)
    {
        pr_info("%s\n", message);
    }
    pr_info("Module Loaded\n");
    return 0;
}

static void __exit msg_exit(void)
{
    pr_info("Module Unloaded\n");
}

module_init(msg_init);
module_exit(msg_exit);
```

**Task 2:** Compile and load with:

```bash
sudo insmod hello.ko message="Custom Text" repeat_count=3
```

Answer: What does `dmesg` show?

```
ahmed@ahmed:~/BitLearnTasks/msg_cnt$ touch main.c Makefile
ahmed@ahmed:~/BitLearnTasks/msg_cnt$ micro main.c
ahmed@ahmed:~/BitLearnTasks/msg_cnt$ micro Makefile
ahmed@ahmed:~/BitLearnTasks/msg_cnt$ make
make -C /lib/modules/`uname -r`/build M=/home/ahmed/BitLearnTasks/msg_cnt modules
make[1]: Entering directory '/usr/src/linux-headers-6.8.0-90-generic'
warning: the compiler differs from the one used to build the kernel
  The kernel was built by: x86_64-linux-gnu-gcc-12 (Ubuntu 12.3.0-1ubuntu1~22.04.2) 12.3.0
  You are using:           gcc-12 (Ubuntu 12.3.0-1ubuntu1~22.04.2) 12.3.0
  CC [M]  /home/ahmed/BitLearnTasks/msg_cnt/main.o
  MODPOST /home/ahmed/BitLearnTasks/msg_cnt/Module.symvers
  CC [M]  /home/ahmed/BitLearnTasks/msg_cnt/main.mod.o
  LD [M]  /home/ahmed/BitLearnTasks/msg_cnt/main.ko
  BTF [M] /home/ahmed/BitLearnTasks/msg_cnt/main.ko
Skipping BTF generation for /home/ahmed/BitLearnTasks/msg_cnt/main.ko due to unavailability of vmlinux
make[1]: Leaving directory '/usr/src/linux-headers-6.8.0-90-generic'
ahmed@ahmed:~/BitLearnTasks/msg_cnt$ sudo insmod main.ko repeat_count=5 message="Hello from Anisuddin"
ahmed@ahmed:~/BitLearnTasks/msg_cnt$ sudo dmesg | tail -9
[86161.851187] Module Unloaded
[86211.831654] main: unknown parameter 'from' ignored
[86211.831658] main: unknown parameter 'Anisuddin' ignored
[86211.831698] Hello
[86211.831699] Hello
[86211.831699] Hello
[86211.831699] Hello
[86211.831699] Hello
[86211.831700] Module Loaded
ahmed@ahmed:~/BitLearnTasks/msg_cnt$
```

**Task 3:** Check parameter visibility:

```bash
cat /sys/module/hello/parameters/message
cat /sys/module/hello/parameters/repeat_count
```

```
ahmed@ahmed:~/BitLearnTasks/msg_cnt$ cat /sys/module/main/parameters/message
Hello
ahmed@ahmed:~/BitLearnTasks/msg_cnt$ cat /sys/module/main/parameters/repeat_count
5
```

Answer: Can you modify these files after module load?
yes
echo "Hi" > /sys/module/main/parameters/message
echo 3 > /sys/module/main/parameters/repeat_count

**Task 4:** Write a module with:

1. A debug level parameter
2. A timeout parameter
3. Print them on load

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static int debug_level;
static int timeout;

module_param(debug_level, int, 0644);
module_param(timeout, int, 0644);

MODULE_LICENSE("GPL");

static int __init debug_init(void)
{
    pr_info("Debug Module Loaded!\n");
    pr_info("Debug Level: %d\n", debug_level);
    pr_info("Timeout: %d seconds\n", timeout);
    
    if (debug_level)
        pr_info("Debug mode enabled\n");
    
    return 0;
}

static void __exit debug_exit(void)
{
    pr_info("Debug Module Unloaded\n");
}

module_init(debug_init);
module_exit(debug_exit);
```
