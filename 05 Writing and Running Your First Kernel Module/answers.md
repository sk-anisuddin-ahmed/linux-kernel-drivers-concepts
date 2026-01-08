# 05 First Module - Assignment Tasks

## Assignment Tasks

**Task 1:** Write 3–4 sentence answers:

1. What is a kernel module?
2. Why do we use `printk()` instead of `printf()`?
3. What does `insmod` do?
4. What is the purpose of `module_init()` and `module_exit()`?
5. Why should kernel modules be tested in VMs?

**Task 2:** Change your module to:

1. Print your name and current time when loading
2. Print a goodbye message with module name when unloading

**Task 3:** Try Error Handling

1. Try loading the module twice — what happens?
2. Remove it and try removing it again — what does the kernel say?
3. Delete `MODULE_LICENSE("GPL")` and reload — what warning do you get in `dmesg`?

**Task 4:** Write a new module called `counter.c`:

1. It maintains a counter that increases every time the module is loaded.
2. When removed, it shows the final count.
