# 16 Modular Architecture - Assignment Answers and Tasks

## Task 1: Theory Questions

1. **What does EXPORT_SYMBOL() do?**
   - Makes a function/variable visible to other kernel modules. Without it, the symbol is only visible within the module.

2. **What's the difference between EXPORT_SYMBOL and EXPORT_SYMBOL_GPL?**
   - EXPORT_SYMBOL allows any module to use the symbol. EXPORT_SYMBOL_GPL restricts usage to modules with GPL license, preventing proprietary code from using GPL'd APIs.

3. **Why does module load order matter?**
   - Dependent modules need their dependencies loaded first. If you load a module that calls an exported symbol that doesn't exist yet, the kernel returns "unknown symbol" error.

4. **How do you specify module dependencies?**
   - In the Makefile, list both objects: `obj-m += core.o helper.o`. Or manually with `insmod` loading core before helper.

5. **What happens if you rmmod a dependency while dependent modules are loaded?**
   - The rmmod fails with "Device or resource busy" error. You must unload dependent modules first.

## Task 2: Create Three-Module System

Create a modular system with:

1. **database.ko**: Provides lookup function (exported)
2. **sensor.ko**: Uses database to register devices (depends on database)
3. **api.ko**: Exposes functions to both (depends on both)

**Sample Solution:**

```c
// database.c
#include <linux/init.h>
#include <linux/module.h>

#define MAX_ENTRIES 10

static struct {
    char name[64];
    int value;
} database[MAX_ENTRIES];

static int db_count = 0;

int db_register(const char *name, int value)
{
    if (db_count >= MAX_ENTRIES)
        return -ENOMEM;

    strcpy(database[db_count].name, name);
    database[db_count].value = value;
    printk(KERN_INFO "[DB] Registered: %s = %d\n", name, value);
    db_count++;
    return db_count - 1;
}
EXPORT_SYMBOL(db_register);

int db_lookup(const char *name)
{
    int i;
    for (i = 0; i < db_count; i++) {
        if (strcmp(database[i].name, name) == 0)
            return database[i].value;
    }
    return -1;
}
EXPORT_SYMBOL(db_lookup);

static int __init database_init(void)
{
    printk(KERN_INFO "[DB] Database module initialized\n");
    return 0;
}

static void __exit database_exit(void)
{
    printk(KERN_INFO "[DB] Database module exited\n");
}

module_init(database_init);
module_exit(database_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Database module");
```

```c
// sensor.c
#include <linux/init.h>
#include <linux/module.h>

extern int db_register(const char *name, int value);

static int __init sensor_init(void)
{
    printk(KERN_INFO "[SENSOR] Initializing\n");
    
    // Register sensors in database
    db_register("temp_sensor", 25);
    db_register("humidity_sensor", 60);
    db_register("pressure_sensor", 1013);
    
    return 0;
}

static void __exit sensor_exit(void)
{
    printk(KERN_INFO "[SENSOR] Exited\n");
}

module_init(sensor_init);
module_exit(sensor_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Sensor registration module");
```

```c
// api.c
#include <linux/init.h>
#include <linux/module.h>

extern int db_lookup(const char *name);

int api_get_sensor(const char *name)
{
    int value = db_lookup(name);
    printk(KERN_INFO "[API] Queried %s = %d\n", name, value);
    return value;
}
EXPORT_SYMBOL(api_get_sensor);

static int __init api_init(void)
{
    printk(KERN_INFO "[API] API module initialized\n");
    
    // Use the API
    int temp = api_get_sensor("temp_sensor");
    printk(KERN_INFO "[API] Temperature: %d\n", temp);
    
    return 0;
}

static void __exit api_exit(void)
{
    printk(KERN_INFO "[API] Exited\n");
}

module_init(api_init);
module_exit(api_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("API module");
```

```makefile
obj-m += database.o sensor.o api.o

all:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
 make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

**Test:**

```bash
make
sudo insmod database.ko
sudo insmod sensor.ko
sudo insmod api.ko

dmesg | tail -10
# Shows all initialization messages with dependencies

sudo rmmod api.ko
sudo rmmod sensor.ko
sudo rmmod database.ko  # Reverse order
```

## Task 3: Module Dependency Graph

Create 4 modules with this dependency structure:

```
core <- helper1 <- app
     <- helper2 <- app
```

Where app depends on both helper1 and helper2, which both depend on core.

**Key idea**: The app module should call functions from both helpers, which in turn call core functions. This creates a diamond dependency pattern.

(Implementation similar to above with 4 modules and appropriate export/extern declarations)
