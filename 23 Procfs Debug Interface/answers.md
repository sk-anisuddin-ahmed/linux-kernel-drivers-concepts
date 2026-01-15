# Section 23: Procfs Debug Interface � Assignment

## Task 1: Theory Questions

1. **Procfs vs Sysfs**:
   Procfs: one-directional, diagnostic data, complex output. Sysfs: hierarchical, device attributes, one-value-per-file. Procfs is older; sysfs is modern standard for drivers.

2. **Seq_file Purpose**:
   Abstracts partial reads and buffer boundaries in proc operations. Provides callbacks (`start()`, `next()`, `show()`, `stop()`) for iteration without manual buffer management.

3. **Write Handler Pattern**:
   Parse command from buffer, validate input, perform operation with mutex locking, return bytes consumed or error code.

4. **Proc Permissions**:
   Set via `mode` parameter (0644 read-write, 0444 read-only). `proc_ops` defines callbacks; absent write = read-only. VFS layer enforces before callbacks.

5. **Cleanup Requirements**:
   Remove entries via `remove_proc_entry()` in exit to prevent leaks. Keep data valid until descriptors close. Remove before freeing data, use locks for shared access.

## Task 2: IRQ Counter via Procfs

**Answer:**

```c
static int seq_show(struct seq_file *s, void *v) {
    mutex_lock(&irq_lock);
    seq_printf(s, "Device %d: %d interrupts\n", (int)v, irq_counters[(long)v]);
    mutex_unlock(&irq_lock);
    return 0;
}

static ssize_t write_handler(struct file *f, const char *buf, size_t count, loff_t *off) {
    mutex_lock(&irq_lock);
    memset(irq_counters, 0, sizeof(irq_counters));
    mutex_unlock(&irq_lock);
    return count;
}
```

## Task 3: Complex Statistics with Seq_file

**Answer:**

```c
static void *start(struct seq_file *s, loff_t *pos) {
    mutex_lock(&activity_lock);
    return (*pos < MAX_ENTRIES) ? (void *)(long)*pos : NULL;
}

static int show(struct seq_file *s, void *v) {
    int idx = (long)v;
    seq_printf(s, "[%d] count=%d duration=%d status=%s\n", idx,
               stats[idx].count, stats[idx].duration, stats[idx].status);
    return 0;
}

static void stop(struct seq_file *s, void *v) {
    mutex_unlock(&activity_lock);
}
```
