# 03 Building and Booting the Linux Kernel

Understand how to **compile the Linux kernel source**, generate the required files, and **boot into your custom-built kernel** safely. You'll also learn what happens behind the scenes during boot time using tools like GRUB and initramfs.

## Why Would You Build the Kernel?

The Linux kernel that comes pre-installed with your OS (like Ubuntu) is built by someone else — with a specific set of features enabled. But when you want to:

- Add custom device drivers
- Optimize the kernel for embedded or IoT systems
- Remove unnecessary features
- Experiment and learn

…you need to **compile the kernel yourself**.

Compiling the kernel means you're taking human-readable source code and turning it into a machine-understandable binary that your system can boot and run.

## What You Get After Compiling the Kernel

When you compile the kernel, you get:

| File | Purpose |
|------|---------|
| `vmlinuz` | The actual compressed kernel image your system boots into. |
| `System.map` | A symbol table showing all internal kernel addresses (used in debugging). |
| `.ko` files | Kernel object modules, like drivers you can load separately. |
| `initramfs` | Temporary root filesystem for early boot phase. |
| `config` | A copy of the configuration options used to build the kernel. |

## What Happens During Kernel Compilation?

Here's the **overall flow** of compiling and booting a Linux kernel:

1. **Configure the Kernel**  
   Select features using a configuration tool like `make menuconfig`. This creates a file called `.config`.

2. **Compile the Kernel**  
   Run `make`. This will compile the entire source code using the `.config` settings.

3. **Install Modules**  
   Run `make modules_install` to copy all `.ko` modules into `/lib/modules/<version>`.

4. **Install the Kernel**  
   Run `make install` to place `vmlinuz`, `System.map`, and `config` into `/boot/`.

5. **Update GRUB**  
   The GRUB bootloader is updated to include your new kernel in the boot menu.

6. **Reboot and Select Your Kernel**  
   On the next boot, you select your kernel from the GRUB menu and run it!

## Understanding the Boot Process (Step-by-Step)

Let's understand what happens when you **power on your PC**:

1. **BIOS/UEFI starts** — initializes hardware.

2. **GRUB starts** — GRUB is the bootloader. It shows you the menu to choose your OS or kernel version.

3. **Kernel loads (`vmlinuz`)** — The selected kernel is decompressed and loaded into memory.

4. **Initramfs runs** — This small filesystem helps the kernel initialize essential hardware (like disks).

5. **`init` (or `systemd`) starts** — Now the full OS starts booting, mounting disks, starting services.

6. **Login prompt or desktop appears** — Your OS is ready to use.

## Boot Safety Tip: Use Dual Kernel Setup

You never want to be **locked out** because your new kernel failed to boot. Here's the safety tip:

- When you install a new kernel, don't delete the old one.
- GRUB will list both — if your new kernel fails, you can **boot into the old one**.

This is why a VM is ideal — even if it crashes, you can just restart it or take a snapshot.

## Real-World Use Cases of Custom Kernel Builds

- A smartphone company may strip the kernel to only include ARM and touch drivers.
- A robotics engineer may enable specific drivers for servo motors and disable others.
- A security researcher might compile a kernel with custom syscall restrictions.

Learning to build your own kernel unlocks all of these paths.
