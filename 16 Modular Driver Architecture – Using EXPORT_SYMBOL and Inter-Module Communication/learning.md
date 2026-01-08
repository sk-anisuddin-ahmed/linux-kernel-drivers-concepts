# 16 Modular Architecture – EXPORT_SYMBOL and Module Dependencies

## Overview

As drivers grow complex, you'll want to split them into **multiple modules** that work together:

- Core functionality module
- Helper/utility modules
- Platform-specific code modules

One module can **export symbols** so other modules can use them. This is done with `EXPORT_SYMBOL()`.

By the end of this section, you will:

- Understand module dependencies
- Use `EXPORT_SYMBOL()` to share functions
- Create modules that depend on other modules
- Understand the module load order

## Exporting Symbols

In your core module, declare functions you want to share:

```c
// core_driver.c
int do_something(int value)
{
    return value * 2;
}
EXPORT_SYMBOL(do_something);

MODULE_LICENSE("GPL");
```

Now other modules can use it:

```c
// helper_driver.c
extern int do_something(int value);

static int helper_init(void)
{
    int result = do_something(5);
    printk(KERN_INFO "Result: %d\n", result);
    return 0;
}

module_init(helper_init);
MODULE_LICENSE("GPL");
```

## Module Dependencies

The helper module **depends** on the core module. Load order matters:

```bash
insmod core_driver.ko    # Load first
insmod helper_driver.ko  # Load after
```

If you try to load helper_driver before core_driver, it fails with "unknown symbol" error.

## EXPORT_SYMBOL vs EXPORT_SYMBOL_GPL

```c
EXPORT_SYMBOL(function);      // Available to all modules (GPL or non-GPL)
EXPORT_SYMBOL_GPL(function);  // Only to GPL-licensed modules
```

Use `EXPORT_SYMBOL_GPL` to restrict proprietary modules from using your GPL code.

## Step-by-Step: Modular Driver System

### 1. Core Module (exports functions)

```c
// core.c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

int core_process_data(const char *input)
{
    printk(KERN_INFO "[CORE] Processing: %s\n", input);
    return strlen(input);
}
EXPORT_SYMBOL(core_process_data);

int core_init(void)
{
    printk(KERN_INFO "[CORE] Initialized\n");
    return 0;
}

void core_exit(void)
{
    printk(KERN_INFO "[CORE] Exited\n");
}

module_init(core_init);
module_exit(core_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Core module");
```

### 2. Helper Module (uses core functions)

```c
// helper.c
#include <linux/init.h>
#include <linux/module.h>

extern int core_process_data(const char *input);

static int helper_init(void)
{
    int result;

    printk(KERN_INFO "[HELPER] Initialized\n");
    
    // Call exported function from core
    result = core_process_data("Hello from helper");
    printk(KERN_INFO "[HELPER] Core returned: %d\n", result);

    return 0;
}

static void helper_exit(void)
{
    printk(KERN_INFO "[HELPER] Exited\n");
}

module_init(helper_init);
module_exit(helper_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Helper module");
```

### 3. Makefile

```makefile
obj-m += core.o helper.o

all:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

### 4. Build and Test

```bash
make
lsmod  # Nothing loaded yet

sudo insmod core.ko
dmesg | tail
# [CORE] Initialized

sudo insmod helper.ko
dmesg | tail
# [HELPER] Initialized
# [HELPER] Core returned: 18

sudo rmmod helper.ko
sudo rmmod core.ko  # Must unload in reverse order
```

## Important Notes

1. **Load order**: Always load dependencies first
2. **GPL licensing**: Both modules usually need `MODULE_LICENSE("GPL")`
3. **Symbol visibility**: Use `EXPORT_SYMBOL_GPL` for GPL-only APIs
4. **Unload order**: Reverse dependency order
5. **Dependencies tracked by modinfo**: `modinfo helper.ko` shows dependencies
6. **Module aliases**: Use `MODULE_ALIAS()` for alternative names
