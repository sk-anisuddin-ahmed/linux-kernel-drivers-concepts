# 16 Modular Architecture - Assignment Answers and Tasks

## Task 1: Theory Questions

1. **What does EXPORT_SYMBOL() do?**

2. **What's the difference between EXPORT_SYMBOL and EXPORT_SYMBOL_GPL?**

3. **Why does module load order matter?**

4. **How do you specify module dependencies?**

5. **What happens if you rmmod a dependency while dependent modules are loaded?**

## Task 2: Create Three-Module System

Create a modular system with:

1. **database.ko**: Provides lookup function (exported)
2. **sensor.ko**: Uses database to register devices (depends on database)
3. **api.ko**: Exposes functions to both (depends on both)

**Key Requirements:**

- Use EXPORT_SYMBOL() to export functions
- Use extern to declare dependencies
- Load modules in correct order

## Task 3: Module Dependency Graph

Create 4 modules with this dependency structure:

```
core <- helper1 <- app
     <- helper2 <- app
```

Where app depends on both helper1 and helper2, which both depend on core.
