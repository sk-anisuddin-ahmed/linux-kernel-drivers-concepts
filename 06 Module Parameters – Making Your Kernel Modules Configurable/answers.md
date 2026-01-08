# 06 Module Parameters - Assignment Tasks

## Assignment Tasks

**Task 1:** Modify your hello module to accept:

1. A `message` parameter (string)
2. A `repeat_count` parameter (integer)

**Task 2:** Compile and load with:

```bash
sudo insmod hello.ko message="Custom Text" repeat_count=3
```

Answer: What does `dmesg` show?

**Task 3:** Check parameter visibility:

```bash
cat /sys/module/hello/parameters/message
cat /sys/module/hello/parameters/repeat_count
```

Answer: Can you modify these files after module load?

**Task 4:** Write a module with:

1. A debug level parameter
2. A timeout parameter
3. Print them on load
