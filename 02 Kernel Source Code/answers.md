## Assignment Tasks

**Task 1:** Go to your Linux system (or VM) and open the kernel source directory.

```
cd /usr/src
ls
```

Find a folder like `linux-source-*` or `linux-headers-*`. Explore and answer:

- What folders do you see inside?
- What folder likely contains USB driver code?
- What folder has memory management code?

**Task 2:** Match the kernel folders with what they do (write it down):

| Folder | Purpose |
|--------|---------|
| 1. fs/ | A. Startup and boot code |
| 2. mm/ | B. Filesystem support |
| 3. drivers/net/ | C. Network interface drivers |
| 4. arch/x86/ | D. x86-specific architecture code |
| 5. init/ | E. Memory allocation and paging |

**Task 3:** Answer these in your notes:

1. What is the purpose of a Makefile in the kernel?
2. What does Kconfig do?
3. Why is the kernel source divided into many folders?
4. Can we remove features from the kernel before building? How?
5. Why should we avoid modifying the top-level kernel Makefile directly?
