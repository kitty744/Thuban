# Thuban TODO List

## Version 0.2

### System

- Write assembly code for all arch (x86_64, arm, etc...)

### Driver API

- Create a device.h that every driver will be a sub-class of
- Create a module system for pre-loading and built-in's.

### Security

- Create a panic and extract logic to It, (BSOD based.)
- Create a kernel ring mode, (0 kernel) (3 user mode), with syscall API.

### Performance

- Create and implement spinlock's.

### Tools

- Create a cocci config and setup cocci
- Improve Kconfig with more option's.
- Seperate into multi file make system

### Documentation

- Rewrite Documentation for CONTRIBUTING, and other current .md to include better guide's and include COCCI
- Write the entire docs/ folder, include docs for tool's such as 'kconfig', 'coccinelle', then also documentation all code such as 'driver api', 'stdio library', 'mm, etc...
