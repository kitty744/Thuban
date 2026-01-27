/*
 * Copyright (c) 2026 Trollycat
 * Module system for Thuban
 * Supports both built-in drivers and loadable modules
 */

#ifndef THUBAN_MODULE_H
#define THUBAN_MODULE_H

#include <stdint.h>

// module states
enum module_state
{
    MODULE_STATE_UNLOADED,
    MODULE_STATE_LOADING,
    MODULE_STATE_LIVE,
    MODULE_STATE_UNLOADING
};

// module structure
struct module
{
    const char *name;
    enum module_state state;

    // metadata
    const char *author;
    const char *description;
    const char *license;
    const char *version;

    // init and exit functions
    int (*init)(void);
    void (*exit)(void);

    // reference counting
    int refcount;

    // linked list
    struct module *next;
};

// init level priorities
#define INIT_LEVEL_EARLY 0  // critical early init
#define INIT_LEVEL_CORE 1   // core kernel subsystems
#define INIT_LEVEL_ARCH 2   // architecture specific
#define INIT_LEVEL_SUBSYS 3 // subsystems
#define INIT_LEVEL_FS 4     // filesystems
#define INIT_LEVEL_DEVICE 5 // device drivers
#define INIT_LEVEL_LATE 6   // late init

// init function pointer type
typedef int (*initcall_t)(void);

// structure for init functions
struct initcall
{
    initcall_t func;
    int level;
    const char *name;
};

// section macros for placing init calls
#define __init __attribute__((__section__(".init.text")))
#define __exit __attribute__((__section__(".exit.text")))
#define __initdata __attribute__((__section__(".init.data")))

// define init call at specific level
#define __define_initcall(fn, level)                            \
    static initcall_t __initcall_##fn __attribute__((__used__)) \
    __attribute__((__section__(".initcall" #level ".init"))) = fn

// init call macros
#define early_initcall(fn) __define_initcall(fn, 0)
#define core_initcall(fn) __define_initcall(fn, 1)
#define arch_initcall(fn) __define_initcall(fn, 2)
#define subsys_initcall(fn) __define_initcall(fn, 3)
#define fs_initcall(fn) __define_initcall(fn, 4)
#define device_initcall(fn) __define_initcall(fn, 5)
#define late_initcall(fn) __define_initcall(fn, 6)

// module metadata macros (stored in .modinfo section)
#define __MODULE_INFO(tag, name, info)       \
    static const char __mod_##name##_##tag[] \
        __attribute__((__used__))            \
        __attribute__((__section__(".modinfo"))) = #tag "=" info

#define MODULE_AUTHOR(a) __MODULE_INFO(author, __COUNTER__, a)
#define MODULE_DESCRIPTION(d) __MODULE_INFO(description, __COUNTER__, d)
#define MODULE_LICENSE(l) __MODULE_INFO(license, __COUNTER__, l)
#define MODULE_VERSION(v) __MODULE_INFO(version, __COUNTER__, v)
#define MODULE_ALIAS(a) __MODULE_INFO(alias, __COUNTER__, a)

// module init/exit for loadable modules
// these get renamed to init_module/cleanup_module by the build system
#define module_init(initfn) \
    int init_module(void) __attribute__((alias(#initfn)))

#define module_exit(exitfn) \
    void cleanup_module(void) __attribute__((alias(#exitfn)))

// for built-in drivers use device_initcall instead
#define builtin_driver_init(initfn) device_initcall(initfn)

// module reference counting
#define try_module_get(mod) __try_module_get(mod)
#define module_put(mod) __module_put(mod)

// module management functions
void module_init_builtin(void);
int module_load(const char *name);
int module_unload(const char *name);
struct module *module_find(const char *name);
void module_list(void);

// internal functions
int __try_module_get(struct module *mod);
void __module_put(struct module *mod);

#endif