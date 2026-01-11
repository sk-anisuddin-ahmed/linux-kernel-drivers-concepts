# Section 22: Sysfs Interface – Assignment

## Task 1: Theory Questions

1. **Sysfs vs ioctl**:

2. **Attribute Groups**:

3. **Show/Store Functions**:

4. **Thread Safety**:

5. **DEVICE_ATTR Macros**:

## Task 2: LED Control via Sysfs

Implement sysfs interface for LED on/off control with thread safety:

**Key Requirements:**

- Create DEVICE_ATTR_RW() attributes for state control
- Use mutex to protect shared data
- Implement show() and store() functions
- Use sysfs_create_group() to create attribute files

## Task 3: Sensor Stats via Sysfs

Implement multi-attribute sysfs interface for sensor with min/max/average
