# Section 28: Device Tree Overlays � Assignment

## Task 1: Quiz

**1. What file format is .dtbo?**
.dtbo is a compiled device tree overlay (binary format). Created from .dts source by the device tree compiler (dtc). Must be compiled before loading.

**2. What's the purpose of __overlay__?**
__overlay__ is the root node in a .dtbo file that specifies which parts of the main device tree to patch. It defines fragments that merge into the live tree.

**3. Why must compatible match exactly?**
The kernel matches drivers to devices by comparing compatible strings. If compatible doesn't match exactly, the driver won't probe and the device won't be initialized.

**4. What's the result of incorrect GPIO numbers?**
The driver will try to access the wrong GPIO pin. This causes read/write on unrelated hardware, potentially corrupting state or triggering wrong interrupts.

**5. Can multiple DTOs be loaded simultaneously?**
Yes, each DTO is a separate fragment that applies independently. Multiple overlays can coexist as long as they don't conflict (e.g., assign same GPIO to two devices).

## Task 2: Add Multiple Devices via DTO

Log Session two nodes:

gpiobtn@0 { ... compatible = "myvendor,gpiobtn"; }

gpiobtn@1 { ... compatible = "myvendor,gpiobtn"; }

Use different GPIOs, names: gpiobtn0, gpiobtn1.

**Device Tree Overlay (gpiobtn-dual.dts):**
```
/dts-v1/;
/plugin/;

/ {
	fragment@0 {
		target = <&gpio0>;
		__overlay__ {
			gpiobtn@0 {
				compatible = "myvendor,gpiobtn";
				gpios = <&gpio0 17 GPIO_ACTIVE_LOW>;
				label = "gpiobtn0";
			};
		};
	};
	fragment@1 {
		target = <&gpio0>;
		__overlay__ {
			gpiobtn@1 {
				compatible = "myvendor,gpiobtn";
				gpios = <&gpio0 27 GPIO_ACTIVE_LOW>;
				label = "gpiobtn1";
			};
		};
	};
};
```

**Compile overlay:**
```bash
dtc -@ -I dts -O dtb gpiobtn-dual.dts -o gpiobtn-dual.dtbo
```

## Task 3: Dynamic DTO on BeagleBone

Place .dtbo under /lib/firmware

Load it at runtime: echo gpiobtn > /sys/devices/platform/bone_capemgr/slots

Then check: dmesg | grep gpiobtn.

**Setup steps:**
```bash
# Copy compiled overlay to firmware directory
cp gpiobtn-dual.dtbo /lib/firmware/

# Load overlay dynamically
echo "gpiobtn-dual" > /sys/devices/platform/bone_capemgr/slots

# Check if loaded
cat /sys/devices/platform/bone_capemgr/slots

# Check kernel messages
dmesg | grep gpiobtn

# Expected output:
# [  123.456789] gpiobtn gpiobtn@0: registered
# [  123.456901] gpiobtn gpiobtn@1: registered

# Unload if needed
echo -1 > /sys/devices/platform/bone_capemgr/slots
```

**Expected behavior:**
- Both gpiobtn0 and gpiobtn1 devices are created
- Drivers probe and bind to both
- dmesg shows probe messages with correct GPIO numbers
- /dev/gpiobtn0 and /dev/gpiobtn1 become available
