/*
 * Copyright (c) 2026 Trollycat
 * Module/Driver metadata system for Thuban
 */

#ifndef THUBAN_MODULE_H
#define THUBAN_MODULE_H

// module metadata structure
struct module_info
{
    const char *name;
    const char *author;
    const char *description;
    const char *version;
    const char *license;
    int (*init)(void);
    void (*exit)(void);
};

// module metadata macros
#define MODULE_AUTHOR(author)                                                  \
    static const char __module_author[] __attribute__((section(".modinfo"))) = \
        "author=" author

#define MODULE_DESCRIPTION(desc)                                                    \
    static const char __module_description[] __attribute__((section(".modinfo"))) = \
        "description=" desc

#define MODULE_LICENSE(license)                                                 \
    static const char __module_license[] __attribute__((section(".modinfo"))) = \
        "license=" license

#define MODULE_VERSION(version)                                                 \
    static const char __module_version[] __attribute__((section(".modinfo"))) = \
        "version=" version

// driver initialization macros
#define module_init(initfn)                                 \
    int __init_##initfn(void) __attribute__((constructor)); \
    int __init_##initfn(void) { return initfn(); }

#define module_exit(exitfn)                                 \
    void __exit_##exitfn(void) __attribute__((destructor)); \
    void __exit_##exitfn(void) { exitfn(); }

// simpler init macro for built-in drivers
#define driver_init(initfn)                                              \
    static int __attribute__((constructor)) __driver_init_##initfn(void) \
    {                                                                    \
        return initfn();                                                 \
    }

#endif