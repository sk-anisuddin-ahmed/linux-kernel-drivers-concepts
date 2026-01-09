# 04 Custom Kernel Build - Assignment Tasks

## Assignment Tasks

**Task 1:** Write down what these commands do:

1. `make menuconfig` - Opens interactive menu to select kernel features, drivers, and modules to compile
2. `make -j$(nproc)` - Compiles kernel using all available CPU cores in parallel for faster build
3. `make modules_install` - Installs compiled modules to `/lib/modules/[version]/` directory
4. `make install` - Installs kernel image, System.map, and config to `/boot/` and updates GRUB
5. `update-grub` - Regenerates GRUB bootloader configuration to include newly installed kernels
6. `uname -r` - Displays currently running kernel version

**Task 2:** After booting:

1. **What does `uname -r` show before your new kernel?**
   Shows the default system kernel version (e.g., `6.8.0-90-generic`)

2. **What does it show after?**
   Shows your custom kernel version (e.g., `6.9.0-custom` or the version you compiled)

3. **Did anything stop working (like WiFi, sound, etc.)? Make notes.**
   Depends on menuconfig selections. Missing drivers cause issues: WiFi (network drivers disabled), sound (audio drivers disabled), USB (USB driver disabled). Note any hardware failures for future kernel configs.

**Task 3:** Open this file:

```bash
less /boot/grub/grub.cfg
```

Answer:

1. **How many kernels are listed under `menuentry`?**
   Usually 2-3 kernels (newest and previous versions). Each kernel gets its own menuentry block.

2. **Which is the default?**
   The first menuentry listed is the default kernel that boots automatically.

3. **How can you manually choose which one to boot?**
   Press arrow keys to select a different menuentry when GRUB menu appears, then press Enter. Hold Shift during boot to force GRUB menu to show.

**Task 4:** Fill in the blanks:

| File | What it does |
|------|-------------|
| `vmlinuz-*` | Compressed kernel executable image that GRUB loads during boot |
| `System.map-*` | Kernel symbol table mapping memory addresses to function/variable names for debugging |
| `.config` | Kernel configuration file recording enabled/disabled features and drivers |
| `/lib/modules/*` | Directory containing compiled kernel modules (.ko files) loadable at runtime |
| `initramfs` | Initial RAM filesystem with essential drivers to mount root filesystem before main kernel |
