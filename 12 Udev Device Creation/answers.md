# 12 Automatic /dev Creation with udev, class_create, and Dynamic Registration - Q&A

## Assignments

1. Implement class_create() and device_create() in your character driver
2. Create a udev rules file for automatic permissions
3. Test that /dev file is created without manual mknod
4. Verify permissions are set correctly by udev rules
5. Test device cleanup on module unload

## Questions & Answers

1. What does class_create() do?
2. How does device_create() differ from mknod?
3. What is the purpose of udev rules?
4. How do you use dynamic major numbers (major=0)?
5. What permissions should be set for device files?
6. How does udev automatically create /dev files?
7. What is THIS_MODULE used for?
8. How do you handle errors from class_create()?
