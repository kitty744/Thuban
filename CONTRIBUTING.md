# Contributing to Thuban

Thank you for your interest in Thuban! Kernel development requires a high degree of precision. Because Thuban is a low-level system, we prioritize **memory safety**, **predictability**, and **maintainability**.

---

## 1. Core Principles

- **Safety First:** Any code that could cause a kernel panic or memory corruption must be heavily scrutinized.
- **Transparency:** Every PR should clearly explain _why_ a change is necessary.

---

## 2. Technical Requirements

### Coding Style

To maintain a unified codebase, we follow specific formatting rules:

- **Naming:** Use `snake_case` for functions and variables.
- **Headers:** All public headers **must** be placed inside `<include/thuban/>`.

### Documentation

Documentation is not optional.

- Use Doxygen-style comments for all public-facing functions.
- **Explain "The Why":** Comments should explain the logic behind complex pointer arithmetic or hardware interactions, not just what the code does.

---

## 3. Development Workflow

| Step  | Action          | Description                                                           |
| :---- | :-------------- | :-------------------------------------------------------------------- |
| **1** | **Fork & Sync** | Fork the repo and ensure your `main` branch is up to date.            |
| **2** | **Research**    | Read `docs/updates/TODO.md` and check issues to avoid duplicate work. |
| **3** | **Branch**      | Create a feature branch: `git checkout -b feature/driver-x`.          |
| **4** | **Implement**   | Write your code and accompanying tests if applicable.                 |
| **5** | **Credit**      | Add your name to `CREDITS` and your email to `.mailmap`.              |
| **6** | **Submit**      | Open a Pull Request with a detailed description of changes.           |

---

## 4. Testing & Verification

Before submitting a Pull Request, ensure:

1. The kernel builds without warnings (`-Werror` is highly encouraged).
2. Your changes do not introduce regressions in existing drivers.
3. (Optional) Provide a log of the kernel booting successfully with your changes.

> **Security Note:** If you discover a vulnerability, please do not open a public issue. Email [kittygamingbuisness@gmail.com] instead.
