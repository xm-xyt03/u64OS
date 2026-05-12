# u64OS — Domain Context

Bare-metal x86-64 kernel written in C17/C++20/Assembly. Boots via GRUB2/Multiboot2.

---

## Glossary

### page

Overloaded term — meaning depends on context:

- **Physical page (page frame)**: a 4 KB aligned region of physical memory. The fundamental unit of physical memory allocation. Referred to by its Page Frame Number (PFN).
- **Page descriptor (`struct Page`)**: a 64-byte metadata struct stored in the **PGDB** (Page Descriptor Database) that describes one physical page. Tracks ownership, allocation state, order, reference counts, and Slab freelist linkage.

When precision matters, say "page frame" or "page descriptor" rather than just "page".

### PFN (Page Frame Number)

The index of a physical page frame. `PFN = physical_address >> PAGE_SHIFT` (i.e. divided by 4096). Used to convert between physical addresses and page descriptors via `pgdb_base`.

### PGDB (Page Descriptor Database)

The array of all `struct Page` descriptors, one per physical page frame. Base pointer: `pgdb_base`. Lives at a fixed virtual address in the kernel address space. Allows O(1) lookup from PFN to descriptor and back.

### order

The buddy-system allocation granularity. An allocation of order _n_ covers 2ⁿ contiguous page frames. Valid range: 0 (1 page, 4 KB) to `MAX_PAGE_ORDER - 1` (1024 pages, 4 MB). The head page descriptor of an allocation records the order.

### Physical Page Allocator

`PagePool`. Manages physical page frames using the **buddy algorithm**: free pages are grouped into 11 free-lists by order (0–10). Allocation splits larger blocks; freeing coalesces buddies upward. The single global instance is `GloblPagePool`.

Key operations: `AllocPages(order)`, `FreePages(page, order)`, `AddPages(page, order)`.

### buddy

Given a page at PFN _p_ with order _n_, its buddy is the page at PFN `p XOR (1 << n)`. Two buddies can be coalesced into a single block of order _n + 1_ only when both are free, of the same order, and of the same memory type.

### coalesce

The process of merging a freed page with its buddy (repeated upward) to form larger free blocks. Performed inside `PagePool::__free_pages`.

### Kernel Heap

`KHeapPool`. Byte-granularity allocator for kernel objects. Sits above `PagePool` in the allocation hierarchy. Holds 11 `KMemCache` instances (one per size class: 16 B – 8 KB) and a direct `PagePool` reference for oversized allocations. The single global instance is `GloblKHeapPool`.

`kmalloc` / `kfree` route through `KHeapPool`.

### KMemCache

A Slab cache for a single fixed object size. Borrows pages from a `PagePool`, slices them into equal-sized objects via `__page_obj_slicing`, and maintains three page lists: **current** (active), **partial** (some objects free), **full** (no free objects). Each `KMemCache` holds up to `KMEM_MAX_POOL_NR` `PagePool` references.

### compound page

An allocation of order _n_ > 0: 2ⁿ contiguous page frames treated as a single unit. The first frame's descriptor has `is_head = true`; the remaining frames have `is_head = false`. The head descriptor records the order for the whole compound page. `KMemCache` may use compound pages when the object size requires more than one page frame per slab.

### Slab

The technique used inside `KMemCache`: a single page or compound page is divided into equal-sized slots. Free slots are linked via an embedded freelist (`Page::freelist`). Not a separate type — "slab" describes the page's role while it belongs to a `KMemCache`.

### direct map region

Virtual address range `0xFFFF800000000000 – 0xFFFFBFFFFFFFFFFF`. All physical memory is mapped here 1:1 with a fixed offset (`physmem_base`). Allows the kernel to access any physical address as a virtual address without additional page-table manipulation.

Conversions: `page_to_virt(p)` / `virt_to_page(va)` use `physmem_base` and `pgdb_base`.

### physmem_base

Runtime variable holding the virtual base of the direct map region. Physical address `pa` maps to `pa + physmem_base`. Set during early boot when the initial page tables are established.

### dynamic map region

Virtual address range `0xFFFFC00000000000 – 0xFFFFCFFFFFFFFFFF`. Used for temporary or non-direct-map virtual mappings (e.g. mapping MMIO or pages that have no 1:1 physical counterpart). Runtime base in `vmremap_base`.

### page database region

Virtual address range `0xFFFFF00000000000 – 0xFFFFF7FFFFFFFFFF`. Holds the PGDB — the flat array of all `struct Page` descriptors. Base pointer: `pgdb_base`.

### kernel stack region

Virtual address range `0xFFFFFA0000000000 – 0xFFFFFA0FFFFFFFFF`. Reserved for kernel stacks.

### kernel segment regions

The kernel ELF sections occupy fixed high-canonical ranges:

| Segment | Range |
|---------|-------|
| `.text` | `0xFFFFFF8000000000 – 0xFFFFFF8007FFFFFF` |
| `.data` | `0xFFFFFF8008000000 – 0xFFFFFF800FFFFFFF` |
| `.rodata` | `0xFFFFFF8010000000 – 0xFFFFFF8017FFFFFF` |
| `.bss` | `0xFFFFFF8018000000 – 0xFFFFFF801FFFFFFF` |

---

## Memory Subsystem Hierarchy

```
kmalloc / kfree
      │
  KHeapPool  (GloblKHeapPool)
  ├─ KMemCache[0..10]   — 16 B, 32 B, 64 B … 8 KB
  │       └─ borrows pages from ──┐
  └─ direct PagePool ref ─────────┤
                                  ▼
                            PagePool  (GloblPagePool)
                            (buddy allocator, order 0–10)
```

---

## Virtual Address Space (kernel)

| Region | Range | Description |
|--------|-------|-------------|
| Direct map | `0xFFFF800000000000 – 0xFFFFBFFFFFFFFFFF` | Physical memory 1:1 mapped |
| Dynamic map | `0xFFFFC00000000000 – 0xFFFFCFFFFFFFFFFF` | Temporary / MMIO mappings |
| Page database | `0xFFFFF00000000000 – 0xFFFFF7FFFFFFFFFF` | `struct Page` array (PGDB) |
| Kernel stacks | `0xFFFFFA0000000000 – 0xFFFFFA0FFFFFFFFF` | Kernel stack region |
| `.text` | `0xFFFFFF8000000000 – 0xFFFFFF8007FFFFFF` | Kernel code |
| `.data` | `0xFFFFFF8008000000 – 0xFFFFFF800FFFFFFF` | Kernel data |
| `.rodata` | `0xFFFFFF8010000000 – 0xFFFFFF8017FFFFFF` | Kernel read-only data |
| `.bss` | `0xFFFFFF8018000000 – 0xFFFFFF801FFFFFFF` | Kernel BSS |

### C++ runtime support

The minimal freestanding C++ runtime implemented in `cxx_base_abi.cpp`. Provides the ABI symbols the compiler assumes exist (`operator new/delete`, `__cxa_atexit`) so C++ can be used without linking against a standard library. Does not include exceptions, RTTI, or iostreams — only what is needed for `new`/`delete` and static object destruction.

---

## Page Table Terminology

This project uses the Linux-style 4-level naming convention. Intel manual equivalents are noted for reference only — do not use Intel names in code or discussion.

| This project | Intel manual | Bit range in virtual address | Type |
|---|---|---|---|
| PGD (Page Global Directory) | PML4 | [47:39] | `pgd_t` |
| PUD (Page Upper Directory) | PDPT | [38:30] | `pud_t` |
| PMD (Page Middle Directory) | PD | [29:21] | `pmd_t` |
| PTE (Page Table Entry) | PT | [20:12] | `pte_t` |

Each level holds 512 entries (9 bits). A virtual address is decoded as:
`PGD[47:39] → PUD[38:30] → PMD[29:21] → PTE[20:12] → page offset[11:0]`

### page fault

An exception (vector 14, `#PF`) raised when address translation fails — page not present, permission violation, or reserved bit set. The faulting address is in `CR2`.

### large page

A PMD entry with the PS bit (`PDE_ATTR_PS`) set maps 2 MB directly, skipping the PTE level. Used during early boot (`boot_init_pgtable`) to establish the initial direct map before fine-grained page tables are set up.

### PIT (Programmable Interval Timer)

The 8253/8254 chip providing a basic periodic timer via the legacy PIC (IRQ 0, vector 32). Used as the initial timer source before the Local APIC timer is calibrated and enabled.

### APIC Timer

The timer built into the Local APIC. Preferred over the PIT once the APIC is initialized; higher resolution and per-CPU. Configured in one-shot or periodic mode via APIC MMIO registers.

### kstat

A single global object recording the boolean initialization state of each kernel subsystem. Each field is a `bool`: `false` = not yet initialized, `true` = initialized. Subsystems check `kstat` before assuming a dependency is ready.

Example fields (planned): `tty_initialized`, `idt_initialized`, `mm_initialized`.

### printk

The kernel's primary log output function. Routing depends on `kstat`: if `kstat.tty_initialized` is `false`, output falls back to `boot_puts` (the early framebuffer writer available before the full TTY is set up); otherwise writes through the kernel TTY.

---

## Boot Flow

```
_start (boot.S)          — Multiboot2 entry; 32-bit → 64-bit mode
  └─ boot_main()
       ├─ boot_init_pgtable()    — initial 4-level page tables (2 MB large pages)
       ├─ boot_tty_init()        — framebuffer + PSF font
       ├─ boot_mm_init()         — parse Multiboot2 memory map
       └─ main()
            ├─ mm_core_init()           — PagePool + KHeapPool init
            ├─ global_constructor_caller() — C++ static constructors
            └─ idt_init()               — load IDT, register ISRs
```

---

## Exception / Interrupt Handling

> Note: `traps.cpp` is named after the Linux convention, but the project distinguishes **exceptions** from **interrupts** as separate concepts.

### exception

A synchronous CPU fault triggered by the currently executing instruction — e.g. divide-by-zero (#DE), general protection fault (#GP), page fault (#PF). Always uses a vector in the range 0–31. Some exceptions push a CPU-supplied **error code** onto the stack; others receive a dummy zero (determined by the bitmask `0x27D00` in `register_interrupt`).

Do not use "trap" as a synonym — that name is inherited from Linux and does not reflect this project's terminology.

### interrupt

An asynchronous signal from hardware (IRQ) or software delivered via the APIC or PIC. Uses vectors 32–255. Does not push an error code; `register_interrupt` inserts a dummy zero so all ISR stubs have a uniform stack layout.

### ISR (Interrupt Service Routine)

The handler registered for a specific vector (0–255) via `register_interrupt`. Each ISR stub saves all registers into an `isr_frame` on the stack, then calls `__isr_common`, which sends EOI and dispatches to `interrupt_handlers[vector]`.

The term "ISR" covers both exception handlers and interrupt handlers — it refers to the mechanism, not the source type.

### isr_frame

The full CPU register save area pushed on exception or interrupt entry (224 bytes: 16 general-purpose registers + vector + error code + RIP/CS/RFLAGS/RSP/SS).

### vector

The 8-bit number (0–255) identifying the exception or interrupt source. Vectors 0–31 are CPU exceptions; vectors 32–255 are hardware IRQs or software interrupts.

### IDT (Interrupt Descriptor Table)

Array of 256 `idt_desc` gate descriptors loaded via `LIDT`. Each entry encodes the ISR offset (split across three fields), privilege level (DPL), IST index, and gate type. Populated by `idt_init()`.

### EOI (End Of Interrupt)

Signal sent to the interrupt controller (APIC or PIC) after an interrupt handler completes, allowing further interrupts on that line. Sent by `interrupt_eoi(vector)` inside `__isr_common`. Required for hardware interrupts (vectors 32+); a no-op for exceptions.
