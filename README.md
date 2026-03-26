# kfs — Kernel From Scratch (x86)

![Language](https://img.shields.io/badge/language-C%20%2F%20ASM-blue)
![Arch](https://img.shields.io/badge/arch-x86__32-orange)
![License](https://img.shields.io/badge/license-MIT-green)

> A monolithic x86 kernel built from scratch — actively developed, from bare-metal boot to preemptive multitasking, virtual memory, and syscalls.

---

## What is this?

**kfs** is an educational kernel targeting the **x86** architecture. The goal: understand how a real OS works by building one — no shortcuts, no abstractions hiding the hardware. Every layer is implemented from scratch, from the bootloader handoff to the scheduler.

---

## Features

**Memory**
- Buddy allocator (physical pages)
- Slab allocator (object caching)
- `kmalloc` / `vmalloc` / `kmap`
- Higher-half kernel mapped at `0xC0000000`
- Copy-On-Write (COW) for `fork()`

**Process & Scheduling**
- Preemptive multitasking (timer-driven)
- Context switching in Assembly
- `fork()` + `exec()` + userspace transitions
- Scheduling: FCFS, Round Robin, MLFQ
- Spinlocks & critical sections

**Arch x86**
- GDT / IDT / TSS
- IRQ & ISR handlers
- ACPI support
- Multiboot2 boot

**Drivers & I/O**
- VGA text mode
- PS/2 Keyboard (with layout support)
- TTY layer
- Custom `libk` (libc reimplemented from scratch)

**Syscalls**
- Syscall table (`syscall.tbl`)
- `write`, `fork`, `exit`, `wait`, and others... 

---

## Wiki & Documentation

Every subsystem implemented in this kernel is documented in the [Wiki](https://github.com/K1tox-Inc/kernel-legacy/wiki). It's structured as a **bottom-up tutorial** — starting from bare metal boot and building up through memory, processes, and syscalls. Work in progress.

---

## Roadmap

What's coming next:

- [ ] Virtual filesystem (VFS) — unified FS abstraction layer
- [ ] Device mapper — generic block device management
- [ ] Driver loading — dynamic kernel module system
- [ ] ELF loader — load and execute ELF binaries from userspace
- [ ] More and more syscalls ...

---

## References

- [Linux Kernel](https://github.com/torvalds/linux) — monolithic design reference
- [OSDev Wiki](https://wiki.osdev.org) — x86 hardware details
- *Operating System Concepts* — Silberschatz et al. (the Dinosaur Book)

*Part of the [K1tox-Inc](https://github.com/K1tox-Inc) org — low-level systems & OS research.*

