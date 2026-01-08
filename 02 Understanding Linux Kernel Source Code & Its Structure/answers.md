# 02 Understanding Linux Kernel Source Code & Its Structure - Q&A

## Assignments

### Task 1: Explore Kernel Source Directory

Go to your Linux system (or VM) and open the kernel source directory:

```bash
cd /usr/src
ls
```

Find a folder like `linux-source-*` or `linux-headers-*`. Explore and answer:

- What folders do you see inside?
- What folder likely contains USB driver code?
- What folder has memory management code?

### Task 2: Match Kernel Folders with Their Purpose

Match the kernel folders with what they do (write it down):

| Folder | Purpose |
|--------|---------|
| 1. fs/ | A. Startup and boot code |
| 2. mm/ | B. Filesystem support |
| 3. drivers/net/ | C. Network interface drivers |
| 4. arch/x86/ | D. x86-specific architecture code |
| 5. init/ | E. Memory allocation and paging |

### Task 3: Answer These Questions

1. What is the purpose of a Makefile in the kernel?
2. What does Kconfig do?
3. Why is the kernel source divided into many folders?
4. Can we remove features from the kernel before building? How?
5. Why should we avoid modifying the top-level kernel Makefile directly?

---

## Questions & Answers

### Q1: What is the Linux Kernel Source Code?

### Q2: How is the Linux kernel source organized?

### Q3: What is a Makefile?

### Q4: What is Kconfig?

### Q5: What are the three main steps in the kernel build process?

### Q6: How do Kconfig, Makefile, and source code files work together?

### Q7: Can you explore kernel source on Ubuntu without downloading anything?
