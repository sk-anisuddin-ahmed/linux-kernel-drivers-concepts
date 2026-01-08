# 05 Writing and Running Your First Kernel Module

Understand what a **kernel module** is, how to write one in C, compile it into a `.ko` file, **insert** it into the kernel, **see output using `dmesg`**, and **remove it** safely.

By the end of this section, you'll be able to:

- Write a simple "Hello, kernel!" module
- Compile it using a Makefile
- Insert (`insmod`) and remove (`rmmod`) the module
- View kernel logs using `dmesg`

## What is a Kernel Module?

A **kernel module** is like a plugin for the Linux kernel. You don't need to rebuild or reboot the kernel to use it. You simply write your own `.ko` file (kernel object), and load it into the running kernel using `insmod`.

Modules are used for:

- Device drivers (USB, Bluetooth, storage)
- File systems
- Debugging tools
- Experimenting and learning (like you!)

Kernel modules run in **kernel space**, not user space — so they have full access to system resources. That's why we have to be careful.

## Write Your First Kernel Module

Let's go through this step-by-step like a mini project.

### 1. Create the Module Source File

Create a folder:

```
mkdir ~/mymodule
cd ~/mymodule
```

Create a C file:

```
nano hello.c
```

Paste this code:

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init hello_init(void)
{
    printk(KERN_INFO "Hello, Kernel! Module Loaded.\n");
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "Goodbye, Kernel! Module Removed.\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("My first kernel module");
```

### 2. Create the Makefile

Create a file called `Makefile` in the same folder:

```
nano Makefile
```

Paste this:

```
obj-m += hello.o

all:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

This tells `make` how to compile your module using the current kernel's build system.

### 3. Compile the Module

Now compile it:

```
make
```

After a few seconds, you'll see:

- `hello.ko` → Your compiled kernel module
- `hello.o`, `hello.mod.o`, etc.

If successful, your `.ko` file is ready to insert!

### 4. Insert the Module into the Kernel

Use `sudo` to insert it:

```
sudo insmod hello.ko
```

Nothing visible will happen yet — but the kernel will log the message!

View the message:

```
dmesg | tail -10
```

You should see:

```
[...timestamp...] Hello, Kernel! Module Loaded.
```

### 5. Remove the Module

To unload the module:

```
sudo rmmod hello
```

Check `dmesg` again:

```
dmesg | tail -10
```

You should now see:

```
[...timestamp...] Goodbye, Kernel! Module Removed.
```

Congrats! You just wrote your first **live kernel extension**.

## Important Notes

- If there's an error in your module and you insert it, your system may crash (especially if not using a VM). So test in VM.
- `printk()` is like `printf()` but for the kernel — use it to log messages.
- Always use `MODULE_LICENSE("GPL")` to avoid kernel warnings.
