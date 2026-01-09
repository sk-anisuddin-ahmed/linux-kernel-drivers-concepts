# 06 Module Parameters

Make your kernel modules configurable by accepting parameters passed during `insmod`, making them flexible and reusable without recompilation.

## What Are Module Parameters?

Module parameters allow users to pass configuration values to your kernel module when loading it. Instead of hardcoding values, you define parameters that can be changed at runtime without recompiling.

### Defining Module Parameters

Use `module_param()` macro to expose variables as parameters.

### Common Use Cases

- Set debug levels
- Configure hardware addresses
- Enable/disable features
- Set timeouts and thresholds

## Creating a Parameterized Module

Create a module that accepts name and count parameters:

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static char *name = "default";
static int count = 1;

module_param(name, charp, 0644);
module_param(count, int, 0644);

MODULE_PARM_DESC(name, "Name parameter");
MODULE_PARM_DESC(count, "Count parameter");

static int __init param_init(void)
{
    printk(KERN_INFO "Module loaded with name=%s, count=%d\n", name, count);
    return 0;
}

static void __exit param_exit(void)
{
    printk(KERN_INFO "Module with name=%s unloaded\n", name);
}

module_init(param_init);
module_exit(param_exit);

MODULE_LICENSE("GPL");
```

## Loading Module with Parameters

```bash
sudo insmod module.ko name="MyModule" count=5
dmesg | tail -5
```

## Important Notes

- Permission bits (0644) control who can read/write the parameter
- Use proper data types: `int`, `charp` (string), `bool`
- Always document parameters with `MODULE_PARM_DESC()`
- Parameters persist in `/sys/module/<name>/parameters/`
