/*
 * Copyright (c) 2026 Trollycat
 * Module system implementation
 */

#include <thuban/module.h>
#include <thuban/stdio.h>
#include <thuban/string.h>

static struct module *module_list_head = NULL;

// external symbols from linker for initcalls
extern initcall_t __initcall0_start[];
extern initcall_t __initcall1_start[];
extern initcall_t __initcall2_start[];
extern initcall_t __initcall3_start[];
extern initcall_t __initcall4_start[];
extern initcall_t __initcall5_start[];
extern initcall_t __initcall6_start[];
extern initcall_t __initcall_end[];

/*
 * Initialize's all built-in modules
 * Calls all initcalls in order by level
 */
void module_init_builtin(void)
{
    printf("[MODULE] Initializing built-in drivers\n");

    // array of initcall start pointers
    initcall_t *initcall_levels[] = {
        __initcall0_start, // early
        __initcall1_start, // core
        __initcall2_start, // arch
        __initcall3_start, // subsys
        __initcall4_start, // fs
        __initcall5_start, // device
        __initcall6_start, // late
        __initcall_end};

    const char *level_names[] = {
        "early", "core", "arch", "subsys", "fs", "device", "late"};

    for (int level = 0; level < 7; level++)
    {
        initcall_t *start = initcall_levels[level];
        initcall_t *end = initcall_levels[level + 1];

        for (initcall_t *fn = start; fn < end; fn++)
        {
            if (*fn)
            {
                int ret = (*fn)();
                if (ret != 0)
                {
                    printf("[MODULE] Init failed at %s level: %d\n", level_names[level], ret);
                }
            }
        }
    }

    printf("[MODULE] Built-in initialization complete\n");
}

/*
 * Register's a module
 */
static int module_register(struct module *mod)
{
    if (!mod)
        return -1;

    mod->next = module_list_head;
    module_list_head = mod;
    mod->state = MODULE_STATE_LIVE;

    printf("[MODULE] Registered: %s v%s\n", mod->name, mod->version ? mod->version : "unknown");

    return 0;
}

/*
 * Load's a module by name
 * NOTE: For now this is a stub, actual module loading requires ELF parsing
 */
int module_load(const char *name)
{
    printf("[MODULE] Module loading not yet implemented: %s\n", name);
    return -1;
}

/*
 * Unload's a module by name
 */
int module_unload(const char *name)
{
    struct module *mod = module_find(name);

    if (!mod)
    {
        printf("[MODULE] Module not found: %s\n", name);
        return -1;
    }

    if (mod->refcount > 0)
    {
        printf("[MODULE] Module in use: %s (refcount: %d)\n", name, mod->refcount);
        return -1;
    }

    if (mod->exit)
    {
        mod->exit();
    }

    // remove from list
    struct module *curr = module_list_head;
    struct module *prev = NULL;

    while (curr)
    {
        if (curr == mod)
        {
            if (prev)
            {
                prev->next = curr->next;
            }
            else
            {
                module_list_head = curr->next;
            }
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    mod->state = MODULE_STATE_UNLOADED;
    printf("[MODULE] Unloaded: %s\n", name);

    return 0;
}

/*
 * Find's module by name
 */
struct module *module_find(const char *name)
{
    struct module *mod = module_list_head;

    while (mod)
    {
        if (strcmp(mod->name, name) == 0)
        {
            return mod;
        }
        mod = mod->next;
    }

    return NULL;
}

/*
 * List's all loaded modules
 */
void module_list(void)
{
    struct module *mod = module_list_head;
    int count = 0;

    printf("Loaded Modules:\n");
    printf("%-20s %-10s %-30s %-10s %s\n", "Name", "Version", "Description", "Refcount", "State");
    printf("--------------------------------------------------------------------------------\n");

    while (mod)
    {
        const char *state_str = "unknown";
        switch (mod->state)
        {
        case MODULE_STATE_UNLOADED:
            state_str = "unloaded";
            break;
        case MODULE_STATE_LOADING:
            state_str = "loading";
            break;
        case MODULE_STATE_LIVE:
            state_str = "live";
            break;
        case MODULE_STATE_UNLOADING:
            state_str = "unloading";
            break;
        }

        printf("%-20s %-10s %-30s %-10d %s\n",
               mod->name,
               mod->version ? mod->version : "N/A",
               mod->description ? mod->description : "N/A",
               mod->refcount,
               state_str);

        count++;
        mod = mod->next;
    }

    printf("\nTotal modules: %d\n", count);
}

/*
 * Try to get module reference
 */
int __try_module_get(struct module *mod)
{
    if (!mod)
        return 0;

    if (mod->state != MODULE_STATE_LIVE)
        return 0;

    mod->refcount++;
    return 1;
}

/*
 * Put module reference
 */
void __module_put(struct module *mod)
{
    if (!mod)
        return;

    if (mod->refcount > 0)
    {
        mod->refcount--;
    }
}