# CODEBUDDY.md

This file provides guidance to CodeBuddy Code when working with code in this repository.

## Project Overview

**u64OS** — a bare-metal x86-64 kernel written in C (C17), C++ (C++20), and x86-64 Assembly. It boots via GRUB2/Multiboot2, transitions from 32-bit to 64-bit mode, and implements foundational kernel subsystems: memory management, exception handling, and a C++ runtime.

## Build Commands

```bash
# Configure and build the kernel binary
make all          # Runs cmake + builds build/kernel.bin

# Clean build artifacts
make clean

# Build bootable ISO (from repo root, delegates to test/Makefile)
make -C test

# Run in QEMU (requires OVMF and qemu-system-x86_64)
make run          # Builds ISO then launches QEMU

# Check dependencies
python3 test.py
```

**QEMU invocation** (from `test/Makefile`):
```bash
qemu-system-x86_64 \
  -bios /usr/share/ovmf/OVMF.fd \
  -cpu kvm64,+smep,+smap \
  -smp sockets=1,cores=4,threads=2 \
  -m 4G --machine q35 \
  -drive file=kernel.iso,format=raw,index=0,media=disk \
  -serial stdio
```

There is no automated test framework; validation is done by booting in QEMU and observing serial output.

## Key Compiler Flags

All kernel code is compiled freestanding with no standard libraries:
```
-ffreestanding -nostdlib -nostdinc -nostdinc++ -fno-pie
-fno-stack-protector -mcmodel=large -fno-asynchronous-unwind-tables
-fno-exceptions
```
Linker: `-nostdlib -z max-page-size=0x1000 -Wl,--build-id=none -static`

## Architecture & Code Structure

```
src/
├── arch/x86/
│   ├── boot/          # Early boot phase (Multiboot2 entry, paging init, TTY)
│   ├── traps/         # IDT setup and exception/ISR handling
│   ├── include/asm/   # x86-specific headers (CPU regs, paging, I/O, COM port)
│   └── linker.lds     # Kernel linker script
├── kernel/
│   ├── main.cpp       # Kernel entry point (called after boot phase)
│   ├── mm/            # Memory management subsystem
│   ├── atomic/        # Locking primitives
│   └── cxx_base_abi.cpp  # C++ runtime (new/delete, __cxa_atexit)
└── include/
    ├── stdtypes.h
    ├── u64OS/         # u64OS namespace: cpp_base, compiler attrs, elf, err
    ├── boot/          # Multiboot2 structures
    ├── graphics/tty/  # Framebuffer TTY and PSF font
    └── kernel/        # Public kernel headers (mm, lib, containers, atomics)
```

### Boot Flow

```
_start (boot.S)  — Multiboot2 entry, 32→64 bit mode setup
  └─ boot_main() (arch/x86/boot/boot_main.cpp)
       ├─ boot_init_pgtable()   — enable 4-level paging
       ├─ boot_tty_init()       — framebuffer + PSF font init
       ├─ boot_mm_init()        — parse Multiboot2 mmap tag
       └─ main() (kernel/main.cpp)
            ├─ mm::mm_core_init()          — page allocator + kernel heap
            ├─ global_constructor_caller() — run C++ static constructors
            ├─ idt_init()                  — load IDT, register ISRs
            └─ while(1) hlt
```

### Memory Management (`src/kernel/mm/`)

- **page_mm.cpp** — Physical page allocator; free-list based, tracks page descriptors.
- **heap_mm.cpp** — Kernel heap; slab-like multi-size cache groups.
- **virt_layout.cpp** — Kernel virtual address space; high-canonical base at `0xffffff8000000000`.
- **mm_main.cpp** — Orchestrates initialization of page pool and heap pool.

### Virtual Memory Layout (from `linker.lds`)

| Region | Address |
|--------|---------|
| Boot loader | `0x00100000` (1 MB) |
| Kernel ELF base | `0xffffff8000000000` |
| Kernel text/data/BSS | immediately above base |

### Exception Handling (`src/arch/x86/traps/traps.cpp`)

- `idt_desc` — 64-bit IDT gate descriptor (type, DPL, IST, offset split across 3 fields).
- `isr_frame` — Full register save frame pushed on exception entry.
- `idt_init()` — Populates and loads the IDT.

### C++ Runtime (`src/kernel/cxx_base_abi.cpp`)

Implements `operator new`/`delete`, sized-delete, and `__cxa_atexit` so C++ can be used without a standard library.

### Key Headers

| Header | Purpose |
|--------|---------|
| `src/arch/x86/include/asm/page_types.h` | 4-level page table types and macros |
| `src/arch/x86/include/asm/cpu_types.h` | CR0/CR4 flag definitions |
| `src/arch/x86/include/asm/io.h` | `inb`/`outb` I/O port helpers |
| `src/include/u64OS/compiler.h` | Compiler attributes (`__always_inline`, etc.) |
| `src/include/kernel/mm.hpp` | Public MM interface |
| `src/include/kernel/lib.hpp` | Aggregates lib.atomic, lib.container, lib.list |

## Agent skills

### Issue tracker

Issues live in GitHub Issues for `xm-xyt03/u64OS`. See `docs/agents/issue-tracker.md`.

### Triage labels

Default label vocabulary (`needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`). See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: one `CONTEXT.md` + `docs/adr/` at the repo root. See `docs/agents/domain.md`.
