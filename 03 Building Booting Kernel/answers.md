# 03 Building and Booting - Assignment Tasks

## Assignment Tasks

**Task 1:** Answer the following based on your current Linux system:

1. **What is your current kernel version?**  
   (Use `uname -r`)

   ```
   ahmed@ahmed:~$ uname -r
   6.8.0-90-generic
   ```

2. **What files are present in `/boot/` that relate to the kernel?**
   - `vmlinuz-*`: Compressed kernel image
   - `initrd.img-*` or `initramfs-*`: Initial ramdisk with drivers and modules
   - `System.map-*`: Kernel symbol table
   - `config-*`: Kernel configuration file used for compilation

3. **What folder(s) typically exist in `/usr/src/`? Are `linux-source` or `linux-headers` folders present?**
   - Typically `linux-headers-*` folders are present, which contain kernel headers for building modules
   - `linux-source-*` may exist if the full source code was installed
   - These folders are used for kernel development and building custom modules

**Task 2:** Write 3–4 line answers to each:

1. **What is the role of GRUB during boot?**
   GRUB (Grand Unified Bootloader) is the first program that runs when your computer starts. It displays a menu allowing you to select which kernel to boot, loads the selected kernel image into memory, and then hands over control to the kernel. GRUB also passes important boot parameters to the kernel.

2. **What is `vmlinuz`?**
   `vmlinuz` is the compressed Linux kernel image file located in `/boot/`. The name stands for "Virtual Memory Linux Uncompressed" (though it's actually compressed). This is the actual kernel executable that GRUB loads and decompresses during boot. It contains all the core kernel code for process management, memory handling, and hardware support.

3. **Why is `initramfs` used?**
   `initramfs` (initial RAM filesystem) is a temporary filesystem loaded by GRUB before the real root filesystem becomes available. It contains essential drivers and modules needed to access storage devices, decrypt encrypted partitions, and mount the root filesystem. Without it, the kernel couldn't access the disk to load the rest of the system.

4. **Why are old kernels kept installed?**
   Old kernels provide a fallback if a new kernel has bugs, hardware incompatibilities, or causes system instability. The previous kernel can be booted from GRUB to troubleshoot issues, making it a safe recovery option. It also allows new kernels to be tested without immediately replacing the working version.

5. **What does `make menuconfig` do?**
   `make menuconfig` launches an interactive text-based menu interface for configuring kernel options before compilation. Features, drivers, and modules can be enabled or disabled based on system requirements. The selections are saved in a `.config` file that the Makefile reads during the build process.

**Task 3:** Match the following steps to their command:

| Action | Command |
|--------|---------|
| A. Compile all sources | b. `make` |
| B. Install modules | c. `make modules_install` |
| C. Update boot files | a. `make install` |
| D. Show config menu | d. `make menuconfig` |

**Task 4:** Explore kernel modules

Use this command to list all your currently loaded kernel modules:

```
lsmod | head -10
```

**Example Module Analysis:**

- tls                   155648  0
- xt_recent              24576  0
- tcp_diag               12288  0
- udp_diag               12288  0
- inet_diag              28672  2 tcp_diag,udp_diag
- ip6t_REJECT            12288  1
- nf_reject_ipv6         24576  1 ip6t_REJECT
- xt_hl                  12288  22
- ip6t_rt                16384  3

Each module adds specific functionality to the kernel and can be loaded/unloaded dynamically without rebooting.
