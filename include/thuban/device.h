/*
 * Copyright (c) 2026 Trollycat
 * Device driver model for Thuban
 */

#ifndef THUBAN_DEVICE_H
#define THUBAN_DEVICE_H

#include <stdint.h>
#include <stddef.h>

// ssize_t definition
typedef long ssize_t;

// forward declarations
struct device;
struct device_driver;
struct bus_type;

// device types
enum device_type
{
    DEVICE_TYPE_CHAR,    // character device
    DEVICE_TYPE_BLOCK,   // block device
    DEVICE_TYPE_NETWORK, // network device
    DEVICE_TYPE_INPUT,   // input device
    DEVICE_TYPE_VIDEO,   // video device
    DEVICE_TYPE_AUDIO,   // audio device
    DEVICE_TYPE_OTHER    // other device
};

// device structure (represents a hardware device)
struct device
{
    const char *name;
    enum device_type type;

    // parent device (for device hierarchy)
    struct device *parent;

    // associated driver
    struct device_driver *driver;

    // device-specific data
    void *driver_data;

    // bus this device is on
    struct bus_type *bus;

    // linked list
    struct device *next;
};

// device driver structure (represents a driver)
struct device_driver
{
    const char *name;
    enum device_type type;

    // probe and remove
    int (*probe)(struct device *dev);
    int (*remove)(struct device *dev);

    // power management
    int (*suspend)(struct device *dev);
    int (*resume)(struct device *dev);

    // device operations
    int (*open)(struct device *dev);
    int (*close)(struct device *dev);
    ssize_t (*read)(struct device *dev, void *buf, size_t count);
    ssize_t (*write)(struct device *dev, const void *buf, size_t count);
    int (*ioctl)(struct device *dev, unsigned int cmd, unsigned long arg);

    // reference count
    int refcount;

    // linked list
    struct device_driver *next;
};

// bus type (like PCI, USB, etc)
struct bus_type
{
    const char *name;

    // match device to driver
    int (*match)(struct device *dev, struct device_driver *drv);

    // probe device
    int (*probe)(struct device *dev);

    // remove device
    int (*remove)(struct device *dev);

    struct bus_type *next;
};

// device registration
int device_register(struct device *dev);
void device_unregister(struct device *dev);

// driver registration
int driver_register(struct device_driver *drv);
void driver_unregister(struct device_driver *drv);

// bus registration
int bus_register(struct bus_type *bus);
void bus_unregister(struct bus_type *bus);

// device/driver matching
int device_bind_driver(struct device *dev, struct device_driver *drv);
void device_unbind_driver(struct device *dev);

// device data accessors
void *dev_get_drvdata(struct device *dev);
void dev_set_drvdata(struct device *dev, void *data);

// list devices and drivers
void device_list(void);
void driver_list(void);

// find device/driver by name
struct device *device_find(const char *name);
struct device_driver *driver_find(const char *name);

#endif