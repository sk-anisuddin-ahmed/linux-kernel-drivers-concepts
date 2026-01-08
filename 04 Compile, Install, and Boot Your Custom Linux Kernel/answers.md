# 04 Custom Kernel Build - Assignment Tasks

## Assignment Tasks

**Task 1:** Write down what these commands do:

1. `make menuconfig`
2. `make -j$(nproc)`
3. `make modules_install`
4. `make install`
5. `update-grub`
6. `uname -r`

**Task 2:** After booting:

1. What does `uname -r` show before your new kernel?
2. What does it show after?
3. Did anything stop working (like WiFi, sound, etc.)? Make notes.

**Task 3:** Open this file:

```bash
less /boot/grub/grub.cfg
```

Answer:

1. How many kernels are listed under `menuentry`?
2. Which is the default?
3. How can you manually choose which one to boot?

**Task 4:** Fill in the blanks:

| File | What it does |
|------|-------------|
| `vmlinuz-*` | ? |
| `System.map-*` | ? |
| `.config` | ? |
| `/lib/modules/*` | ? |
| `initramfs` | ? |

**Q3:**
**A3:**
