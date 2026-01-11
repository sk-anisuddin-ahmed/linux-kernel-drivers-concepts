# Section 24: Device File Operations – Assignment

## Task 1: Theory Questions

1. **Seek Operations**:

2. **Fasync Handler**:

3. **Interruptible Locks**:

4. **Concurrent Access**:

5. **Copy Functions**:

## Task 2: Seekable Buffer Device

Implement character device supporting seek and positional read/write:

**Key Requirements:**
- Implement llseek() for SEEK_SET, SEEK_CUR, SEEK_END
- Use mutex to protect buffer
- Support positional read and write via file->f_pos

## Task 3: Async I/O Notifications

Create driver that signals user process when data is available
