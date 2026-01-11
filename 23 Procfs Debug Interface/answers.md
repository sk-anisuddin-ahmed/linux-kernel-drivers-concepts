# Section 23: Procfs Debug Interface � Assignment

## Task 1: Theory Questions

1. **Procfs vs Sysfs**:

2. **Seq_file Purpose**:

3. **Write Handler Pattern**:

4. **Proc Permissions**:

5. **Cleanup Requirements**:

## Task 2: IRQ Counter via Procfs

Implement `/proc/irq_stats` tracking interrupt counts per device:

**Key Requirements:**

- Use proc_create() to create /proc/irq_stats file
- Implement seq_file show() function for reading
- Support write() handler for "reset" command
- Protect data with mutex

## Task 3: Complex Statistics with Seq_file

Implement multi-entry proc interface for device activity tracking
