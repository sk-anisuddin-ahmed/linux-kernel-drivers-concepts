# Section 30: Final Integration & Validation

## Task 1: Validate Your Driver Lifecycle

Boot system

Auto-load driver via overlay

Interact via /dev/gpiobtn

Suspend + resume

Run debug self-test

Unload module cleanly

**Validation Checklist:**
```bash
reboot
lsmod | grep gpiobtn           # Should show loaded
dmesg | grep gpiobtn           # Should show probe message

rmmod gpiobtn
dmesg | grep "unload"          # Verify cleanup messages
lsmod | grep gpiobtn           # Should not appear

modprobe gpiobtn
lsmod | grep gpiobtn           # Should be back
```

## Task 2: Finalize Your Driver Package

Ensure your driver folder includes:

**Complete folder structure:**
```
gpiobtn_driver/
├── Makefile
├── gpiobtn.c                    # Main driver source
├── gpiobtn.h                    # Header with IOCTL defs
├── gpiobtn-overlay.dtbo         # Compiled device tree overlay
├── README.md                    # Build/install instructions
├── install.sh                   # Installation script
├── uninstall.sh                 # Uninstall script
├── test/
│   ├── test_epoll.c             # Userspace epoll test
│   ├── test_ioctl.c             # Ioctl test
│   └── Makefile
├── udev/
│   └── 99-gpiobtn.rules         # Auto-loading rules
└── dts/
    └── gpiobtn-overlay.dts      # Device tree source
```

## Task 3: (Optional) Explore Beyond

Log Session a Yocto recipe for your driver

Add support for multiple GPIO banks

Port it to SPI/I2C device

Use dynamic device tree overlays from user space