# 05 First Module - Assignment Tasks

## Assignment Tasks

**Task 1:** Write 3–4 sentence answers:

1. **What is a kernel module?**
   A kernel module is dynamically loadable code that extends kernel functionality without rebooting. It can be inserted with `insmod` and removed with `rmmod`.

2. **Why do we use `printk()` instead of `printf()`?**
   `printk()` writes to the kernel log (viewable via `dmesg`) and works in kernel mode where `printf()` cannot. It supports log levels like KERN_INFO and KERN_ERR to categorize messages.

3. **What does `insmod` do?**
   `insmod` loads a compiled kernel module (.ko file) into the running kernel and executes its `module_init()` function. It requires the exact path and will fail if dependencies aren't already loaded.

4. **What is the purpose of `module_init()` and `module_exit()`?**
   `module_init()` defines the function that runs when the module loads, while `module_exit()` defines the function that runs when it unloads. These macros tell the kernel the entry and exit points for the module.

5. **Why should kernel modules be tested in VMs?**
   Kernel modules run in privileged kernel space and can crash the entire system if buggy. Testing in VMs provides isolation and allows easy rollback via snapshots without risking your main system.

**Task 2:** Change your module to:

Example code snippet:

```c
static int __init hello_init(void) {
    printk(KERN_INFO "Hello! I'm Ahmed. Built on: %s %s\n", __DATE__, __TIME__);
    return 0;
}

static void __exit hello_exit(void) {
    printk(KERN_INFO "Goodbye\n");
}
```

**Task 3:** Try Error Handling

1. **Try loading the module twice — what happens?**
   The kernel returns: `ERROR: could not insert module ... File exists`. The module is already loaded and can't be duplicated.

2. **Remove it and try removing it again — what does the kernel say?**
   First `rmmod` succeeds and runs the exit function. Second `rmmod` returns: `ERROR: Module not found in /proc/modules`.

3. **Delete `MODULE_LICENSE("GPL")` and reload — what warning do you get in `dmesg`?**
   You'll see: `module: module_name: license 'unspecified' taints kernel`. This marks the kernel as tainted to indicate a non-GPL module is loaded.

**Task 4:** Write a new module called `counter.c`:

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
MODULE_LICENSE("GPL");
static int load_count = 0;
static int __init counter_init(void) {
    load_count++;
    printk(KERN_INFO "Counter module loaded! Load count: %d\n", load_count);
    return 0;
}
static void __exit counter_exit(void) {
    printk(KERN_INFO "Counter module unloading. Final count: %d\n", load_count);
}
module_init(counter_init);
module_exit(counter_exit);
```
