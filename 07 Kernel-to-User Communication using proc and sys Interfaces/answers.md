# 07 Proc Communication - Assignment Tasks

## Assignment Tasks

**Task 1: Short Answer Questions**

1. What is the /proc filesystem and how is it different from a regular filesystem?
2. Explain what `seq_printf()` does and why it's preferred over regular `sprintf()`
3. What is the purpose of `proc_create()` and what are its main parameters?
4. What happens if you forget to call `remove_proc_entry()` in your exit function?
5. How does the kernel handle multiple users reading `/proc/myinfo` simultaneously?

**Task 2: Custom Data Display**

Modify the `proc_demo` module to:

1. Display the number of times the `/proc/myinfo` file has been read
2. Show a timestamp (using `jiffies`) when the file was first accessed
3. Add a module parameter `author_name` and display it in the output

**Task 3: Explore Built-in /proc Entries**

Run the following commands and examine the output:

```bash
cat /proc/uptime
cat /proc/version
cat /proc/modules
cat /proc/meminfo
cat /proc/cpuinfo
```

Answer:

1. What type of information does each file provide?
2. Which file shows all currently loaded modules?
3. Which file shows your kernel version?
4. What information can you get from `/proc/meminfo`?

**Task 4: Create a Counter /proc Entry**

Create a new kernel module that:

1. Creates a `/proc/counter` entry
2. Implements a counter that increments every time the module is initialized (keep it global)
3. Displays the counter value when read
4. Shows how many times it's been read
5. Clean up properly on module exit

Compile and test your module, then show the output of multiple reads of `/proc/counter`.
