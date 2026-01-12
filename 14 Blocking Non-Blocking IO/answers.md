# 14 Blocking I/O - Assignment Answers and Tasks

## Task 1: Understand Wait Queues

1. **What is a wait queue and why is it needed?**
   - It's a list of processes that are sleeping waiting for something to happen. When a process reads but no data exists, it goes to sleep instead of returning immediately. The wait queue holds it there until data arrives.

2. **What does wait_event_interruptible() do?**
   - Puts the process to sleep until a condition is true. If the user presses Ctrl+C (signal), it wakes up and returns -ERESTARTSYS. Otherwise, waits for the condition.

3. **What's the difference between wake_up() and wake_up_interruptible()?**
   - `wake_up()` wakes all sleeping processes. `wake_up_interruptible()` only wakes those that can be interrupted by signals. Most of the time, use `wake_up_interruptible()` with `wait_event_interruptible()`.

   ```c
   // Reader sleeps
   wait_event_interruptible(wq, condition);
   
   // Writer wakes - use matching wake function
   wake_up_interruptible(&wq);  // Correct - matches interruptible
   ```

4. **Why must you check the return value of wait_event_interruptible()?**
   - It returns 0 if the condition became true, or -ERESTARTSYS if a signal interrupted it (like Ctrl+C). Need to check so the driver doesn't crash if interrupted and can clean up properly.

5. **What would happen if you called wake_up() twice?**
   - Nothing bad. The first `wake_up()` wakes sleeping processes. The second `wake_up()` finds no one sleeping and does nothing. It's safe to call multiple times.

## Task 2: Add Timeout Support

Extend the driver to use `wait_event_interruptible_timeout()` so reads timeout after 5 seconds:

**Solution**

```c
static DECLARE_WAIT_QUEUE_HEAD(wq);
static int data_ready = 0;
static char data[256];

static ssize_t dev_read(struct file *f, char __user *buf, size_t len, loff_t *pos)
{
 long timeout = wait_event_interruptible_timeout(wq, data_ready, 5 * HZ);
 
 if (timeout < 0)
  return -ERESTARTSYS;  // Signal interrupted
 if (timeout == 0)
  return 0;  // Timeout - return EOF
 
 len = strlen(data);
 copy_to_user(buf, data, len);
 data_ready = 0;
 return len;
}
```

**Return values:**

- Positive: Condition became true, data available
- 0: Timeout occurred
- Negative: Signal interrupted (return -ERESTARTSYS)

## Task 3: Multiple Readers on Same Wait Queue

Create a driver that:

1. Allows multiple processes to sleep on the same wait queue
2. When data arrives, all readers wake up
3. Only the first reader gets the data, others go back to sleep

**Example**

```c
static DECLARE_WAIT_QUEUE_HEAD(wq);
static char data[256];
static int data_ready = 0;

static ssize_t dev_read(struct file *f, char __user *buf, size_t len, loff_t *pos)
{
 if (wait_event_interruptible(wq, data_ready))
  return -ERESTARTSYS;
 
 len = strlen(data);
 copy_to_user(buf, data, len);
 data_ready = 0;
 wake_up_interruptible(&wq);
 return len;
}

static ssize_t dev_write(struct file *f, const char __user *buf, size_t len, loff_t *pos)
{
 if (len > 255) len = 255;
 copy_from_user(data, buf, len);
 data[len] = '\0';
 data_ready = 1;
 wake_up_interruptible(&wq);
 return len;
}
```

**Sequence:**

1. Multiple readers call `wait_event_interruptible()` and sleep on same queue
2. Writer writes data, sets `data_ready = 1`, calls `wake_up_interruptible()`
3. All readers wake up and check condition `data_ready == 1` (true)
4. First reader to acquire lock reads data, clears `data_ready = 0`
5. First reader wakes others; they check `data_ready == 0` (false) and go back to sleep
