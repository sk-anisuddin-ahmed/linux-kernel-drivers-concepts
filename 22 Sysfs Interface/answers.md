# Section 22: Sysfs Interface – Assignment

## Task 1: Theory Questions

1. **Sysfs vs ioctl**:
   Sysfs exposes kernel data as a virtual filesystem using standard read/write operations on files, making it simple and tool-friendly with grep, echo, and cat. ioctl is a powerful system call allowing arbitrary command-based communication for complex control and data exchange. Sysfs is preferred for simple controls and status queries, while ioctl is used when sophisticated operations are needed.

2. **Attribute Groups**:
   Attribute groups organize multiple related sysfs attributes using `struct attribute_group` containing an array of `struct attribute` pointers and optional `is_visible()` callbacks. They are registered/unregistered using `sysfs_create_group()` and `sysfs_remove_group()`, simplifying initialization and cleanup. This approach allows selective visibility of attributes based on runtime conditions and supports multiple groups on the same device.

3. **Show/Store Functions**:
   The **show()** function reads attribute values, receiving the device pointer and writing formatted output to a buffer using `snprintf()`. The **store()** function writes attribute values, parsing userspace input, validating it, updating device state, and returning bytes consumed or error codes. Both functions allow userspace to interact with kernel data as if reading/writing regular files, with proper return values for success or failure.

4. **Thread Safety**:
   Multiple processes can simultaneously access the same or different attributes from different CPUs, requiring protection mechanisms like mutexes for serialization and atomic operations for counter updates. The sysfs layer already holds the device mutex during show/store execution, but drivers must synchronize access to state variables shared with other driver operations. Show/store functions must not sleep since sysfs holds internal locks, and reference counting prevents device removal during active attribute access.

5. **DEVICE_ATTR Macros**:
   `DEVICE_ATTR*` macros automatically generate sysfs attribute structures following naming conventions: `DEVICE_ATTR_RW(name)` for read-write, `DEVICE_ATTR_RO(name)` for read-only, `DEVICE_ATTR_WO(name)` for write-only, and `DEVICE_ATTR(name, mode, show, store)` for custom definitions. These macros reduce boilerplate by automatically creating `struct attribute` entries with expected function names `name_show()` and `name_store()`. Drivers simply define the corresponding show/store functions to implement the desired behavior.

## Task 2: LED Control via Sysfs

**Answer:**

Create a `struct led_device` containing a mutex and state variable, then define `DEVICE_ATTR_RW(state)` to generate the attribute with `state_show()` and `state_store()` functions. In `state_show()`, acquire the mutex, read the LED state, format it with `snprintf()`, and return the buffer length; in `state_store()`, parse the input buffer (0 or 1), acquire the mutex, update the GPIO pin, and return the byte count. Finally, create an `attribute_group` array containing the state attribute and register it with `sysfs_create_group()` in your probe function, registering corresponding cleanup in remove function.

## Task 3: Sensor Stats via Sysfs

**Answer:**

Define a `struct sensor_data` tracking current reading, minimum, maximum, and average values protected by a mutex, then create multiple `DEVICE_ATTR_RO()` attributes for each statistic (current, min, max, average). Implement separate `current_show()`, `min_show()`, `max_show()`, and `average_show()` functions that acquire the mutex, format their respective values using `snprintf()`, and return the length. Group all attributes in an `attribute_group` and register with `sysfs_create_group()`; update statistics when new sensor readings arrive by acquiring the mutex and recalculating min, max, and running average values.
