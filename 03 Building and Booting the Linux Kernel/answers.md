# 03 Building and Booting - Assignment Tasks

## Assignment Tasks

**Task 1:** Answer the following based on your current Linux system:

1. What is your current kernel version?  
   (Use `uname -r`)

2. What files are present in `/boot/` that relate to the kernel?

3. What folder(s) exist in `/usr/src/`? Can you see `linux-source` or `linux-headers`?

**Task 2:** Write 3–4 line answers to each:

1. What is the role of GRUB during boot?

2. What is `vmlinuz`?

3. Why is `initramfs` used?

4. Why should you keep old kernels installed?

5. What does `make menuconfig` do?

**Task 3:** Match the following steps to their command:

| Action | Command |
|--------|---------|
| A. Compile all sources | a. `make install` |
| B. Install modules | b. `make` |
| C. Update boot files | c. `make modules_install` |
| D. Show config menu | d. `make menuconfig` |

**Task 4:** Explore kernel modules

Use this command to list all your currently loaded kernel modules:

```
lsmod | head -10
```

Pick one module, and search online to find out what it does. Example: `snd` → it's a sound card driver.
