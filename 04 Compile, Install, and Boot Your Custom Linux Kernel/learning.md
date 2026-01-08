# 04 Compile, Install, and Boot Your Custom Linux Kernel

Learn how to configure, compile, install, and boot into your **own custom kernel** using a real build process — step by step, safely, and with full control.

By the end of this section, you will:

- Configure the Linux kernel using `menuconfig`
- Compile the kernel source code
- Install the kernel and its modules
- Update GRUB to recognize your kernel
- Boot into your own kernel safely

## Step 1: Prepare the Build Environment

Before you can compile the kernel, you need some tools. On Ubuntu/Debian, run:

```bash
sudo apt update

sudo apt install build-essential libncurses-dev bison flex libssl-dev libelf-dev bc
```

These install:

- C compiler and related tools (`gcc`, `make`, etc.)
- Libraries required for kernel options
- Tools for menu configuration

Next, go to your kernel source folder (already installed in earlier weeks):

```bash
cd /usr/src/linux-source-*
```

Extract if needed:

```bash
tar xvf linux-source-*.tar.bz2
cd linux-source-*
```

Create a separate build folder:

```bash
mkdir build
cp -r * build/
cd build
```

## Step 2: Configure the Kernel

Now you configure the features you want:

```bash
make menuconfig
```

This opens a text-based menu (arrow keys to move, space to select, Enter to dive in). It's okay to just save defaults for now.

When you save, it creates a `.config` file containing all selected options.

**Optional Tips:**

To copy your current system config:

```bash
cp /boot/config-$(uname -r) .config
make olddefconfig
```

This gives you a good working starting point.

## Step 3: Compile the Kernel

Now let's compile the kernel and modules:

### To compile the kernel image

```bash
make -j$(nproc)
```

- `-j$(nproc)` uses all CPU cores to speed up build.
- This can take 20–60 minutes depending on your system.

### To compile and install modules

```bash
sudo make modules_install
```

This installs kernel modules into:

```
/lib/modules/<your-kernel-version>
```

## Step 4: Install the Kernel

Once compilation is complete, install the new kernel image:

```bash
sudo make install
```

This copies:

- `vmlinuz-*` → to `/boot/`
- `System.map-*` → to `/boot/`
- `config-*` → to `/boot/`

GRUB is usually auto-updated. If not, run:

```bash
sudo update-grub
```

Check if your kernel appears in GRUB menu:

```bash
grep "menuentry" /boot/grub/grub.cfg
```

You should see your new kernel listed.

## Step 5: Reboot into Your Kernel

Now reboot:

```bash
sudo reboot
```

At the GRUB menu:

- Press **Shift** (if it doesn't show automatically)
- Select your new kernel by version
- Boot into it

After login, confirm:

```bash
uname -r
```

If it shows your custom kernel version (e.g., `6.1.0-custom`), congratulations — you're running your own kernel!

---

## What If It Fails?

If the system freezes or crashes:

- Reboot and select the **older working kernel** from GRUB.
- Check `/var/log/kern.log` or `dmesg` for issues.
- Go back to your source and recheck your `.config`.

**This is why using a Virtual Machine is important.**

## What You Just Did

Let's simplify what just happened:

- You **told Linux** what kind of kernel you want (`menuconfig`)
- You **built** it yourself (`make`)
- You **gave it a home** in `/boot/` (`make install`)
- You **told the bootloader** about it (`update-grub`)
- You **tried it out** and saw your own version running (`uname -r`)

You're now working at the kernel level!
