# 16 Modular Architecture - Assignment Answers and Tasks

## Task 1: Theory Questions

1. **What does EXPORT_SYMBOL() do?**
   Makes a kernel symbol (function/variable) visible to other modules at load time.

2. **What's the difference between EXPORT_SYMBOL and EXPORT_SYMBOL_GPL?**
   `EXPORT_SYMBOL` allows any module to use it; `EXPORT_SYMBOL_GPL` only GPL-licensed modules can use it.

3. **Why does module load order matter?**
   Dependencies must be loaded before dependents. Loading sensor.ko before database.ko causes undefined symbol errors.

4. **How do you specify module dependencies?**
   Use `extern` to declare external functions and `modprobe` handles dependencies automatically.

5. **What happens if you rmmod a dependency while dependent modules are loaded?**
   `rmmod` fails with "Module in use" error unless dependent modules are unloaded first.

## Task 2: Create Three-Module System

Create a modular system with:

1. **database.ko**: Provides lookup function (exported)
2. **sensor.ko**: Uses database to register devices (depends on database)
3. **api.ko**: Exposes functions to both (depends on both)

**database.c:**

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kernel");
MODULE_DESCRIPTION("Database module");

const char* database_lookup(int id)
{
    return (id == 1) ? "Device1" : "Unknown";
}
EXPORT_SYMBOL(database_lookup);

static int __init database_init(void)
{
    pr_info("Database module loaded\n");
    return 0;
}

static void __exit database_exit(void)
{
    pr_info("Database module unloaded\n");
}

module_init(database_init);
module_exit(database_exit);
```

**sensor.c:**

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kernel");
MODULE_DESCRIPTION("Sensor module");

extern const char* database_lookup(int id);

static int __init sensor_init(void)
{
    const char *result = database_lookup(1);
    pr_info("Sensor registered: %s\n", result);
    return 0;
}

static void __exit sensor_exit(void)
{
    pr_info("Sensor module unloaded\n");
}

module_init(sensor_init);
module_exit(sensor_exit);
```

**api.c:**

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kernel");
MODULE_DESCRIPTION("API module");

extern const char* database_lookup(int id);

int api_register(int id)
{
    const char *device = database_lookup(id);
    pr_info("API registered device: %s\n", device);
    return 0;
}
EXPORT_SYMBOL(api_register);

static int __init api_init(void)
{
    pr_info("API module loaded\n");
    return 0;
}

static void __exit api_exit(void)
{
    pr_info("API module unloaded\n");
}

module_init(api_init);
module_exit(api_exit);
```

**Makefile:**

```makefile
obj-m += database.o sensor.o api.o

all:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

install:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules_install

clean:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

**Load order:**

```bash
sudo insmod database.ko
sudo insmod sensor.ko
sudo insmod api.ko
```

## Task 3: Module Dependency Graph

Create 4 modules with this dependency structure:

```
core <- helper1 <- app
     <- helper2 <- app
```

**core.c:**

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

MODULE_LICENSE("GPL");

int core_function(void)
{
    pr_info("Core function called\n");
    return 0;
}
EXPORT_SYMBOL(core_function);

static int __init core_init(void)
{
    pr_info("Core loaded\n");
    return 0;
}

static void __exit core_exit(void)
{
    pr_info("Core unloaded\n");
}

module_init(core_init);
module_exit(core_exit);
```

**helper1.c:**

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

MODULE_LICENSE("GPL");

extern int core_function(void);

int helper1_function(void)
{
    core_function();
    pr_info("Helper1 function called\n");
    return 0;
}
EXPORT_SYMBOL(helper1_function);

static int __init helper1_init(void)
{
    pr_info("Helper1 loaded\n");
    return 0;
}

static void __exit helper1_exit(void)
{
    pr_info("Helper1 unloaded\n");
}

module_init(helper1_init);
module_exit(helper1_exit);
```

**helper2.c:**

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

MODULE_LICENSE("GPL");

extern int core_function(void);

int helper2_function(void)
{
    core_function();
    pr_info("Helper2 function called\n");
    return 0;
}
EXPORT_SYMBOL(helper2_function);

static int __init helper2_init(void)
{
    pr_info("Helper2 loaded\n");
    return 0;
}

static void __exit helper2_exit(void)
{
    pr_info("Helper2 unloaded\n");
}

module_init(helper2_init);
module_exit(helper2_exit);
```

**app.c:**

```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

MODULE_LICENSE("GPL");

extern int helper1_function(void);
extern int helper2_function(void);

static int __init app_init(void)
{
    pr_info("App loaded\n");
    helper1_function();
    helper2_function();
    return 0;
}

static void __exit app_exit(void)
{
    pr_info("App unloaded\n");
}

module_init(app_init);
module_exit(app_exit);
```

**Makefile:**

```makefile
obj-m += core.o helper1.o helper2.o app.o

all:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

install:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules_install

clean:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

**Load order (kernel auto-handles with modprobe):**

```bash
# Manual load - must respect order
sudo insmod core.ko
sudo insmod helper1.ko
sudo insmod helper2.ko
sudo insmod app.ko

# OR use modprobe - auto handles dependencies
sudo modprobe app
# This automatically loads: core, helper1, helper2, then app
```

