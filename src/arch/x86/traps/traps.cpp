extern "C"
{
#include <asm/cpu_types.h>
#include <asm/traps.h>
}

#include <kernel/mm.hpp>
#include <kernel/base.hpp>
#include <u64OS/cpp_base.hpp>

/* Declared in apic.cpp */
extern auto apic_check() -> bool;
extern auto apic_init()  -> void;
extern auto apic_enabled() -> bool;
extern auto apic_eoi()   -> void;

/* Declared in pic.cpp */
extern auto pic_init() -> void;
extern auto pic_eoi()  -> void;

typedef struct
{
    base::uint16_t offset_low_word;
    base::uint16_t selector;
    unsigned ist       : 3;
    unsigned reserved0 : 5;
    unsigned type      : 4;
    unsigned reserved1 : 1;
    unsigned dpl       : 2;
    unsigned p         : 1;
    base::uint16_t offset_mid_word;
    base::uint32_t offset_high_dword;
    base::uint32_t reserved2;
} idt_desc;

/* Generic ISR stub: saves all registers, calls __isr_common, restores and iretq.
   The rel32 for the call is patched at runtime in idt_desc_init(). */
uint8_t __isr[] = {
    /* push rax..rdi, r8..r15 */
    0x50,             /* push rax  */
    0x51,             /* push rcx  */
    0x52,             /* push rdx  */
    0x53,             /* push rbx  */
    0x55,             /* push rbp  */
    0x56,             /* push rsi  */
    0x57,             /* push rdi  */
    0x41, 0x50,       /* push r8   */
    0x41, 0x51,       /* push r9   */
    0x41, 0x52,       /* push r10  */
    0x41, 0x53,       /* push r11  */
    0x41, 0x54,       /* push r12  */
    0x41, 0x55,       /* push r13  */
    0x41, 0x56,       /* push r14  */
    0x41, 0x57,       /* push r15  */
    /* mov rdi, rsp  (pass isr_frame* as first argument) */
    0x48, 0x89, 0xe7,
    /* call __isr_common  (rel32 patched at init time) */
    0xe8, 0x00, 0x00, 0x00, 0x00,
    /* pop r15..r8, rdi..rax */
    0x41, 0x5f,       /* pop r15 */
    0x41, 0x5e,       /* pop r14 */
    0x41, 0x5d,       /* pop r13 */
    0x41, 0x5c,       /* pop r12 */
    0x41, 0x5b,       /* pop r11 */
    0x41, 0x5a,       /* pop r10 */
    0x41, 0x59,       /* pop r9  */
    0x41, 0x58,       /* pop r8  */
    0x5f,             /* pop rdi */
    0x5e,             /* pop rsi */
    0x5d,             /* pop rbp */
    0x5b,             /* pop rbx */
    0x5a,             /* pop rdx */
    0x59,             /* pop rcx */
    0x58,             /* pop rax */
    /* add rsp, 16  (discard vector + err_code) */
    0x48, 0x83, 0xc4, 0x10,
    /* iretq */
    0x48, 0xcf,
};

inline constexpr base::size_t IDT_DESC_CNT = 256;

static void (*(interrupt_handlers[256]))(isr_frame *, bool is_user);

static base::uint8_t isr[256][9];
static idt_desc *idt;

static auto interrupt_eoi(base::uint8_t vector) -> void
{
    (void)vector;
    if (apic_enabled())
        apic_eoi();
    else
        pic_eoi();
}

auto register_interrupt(base::uint8_t vector, void (*routine)(isr_frame *, bool)) -> void
{
    interrupt_handlers[vector] = routine;

    /* Bitmask of exception vectors that push an error code onto the stack */
    base::uint32_t erc = 0x00027D00;
    base::uint8_t *isrx = ::isr[vector];

    /* Vectors with an error code: two nops (error code already on stack).
       Others: push 0 as a dummy error code, then push the vector number. */
    if ((vector < 32) && (erc & (1 << vector)))
    {
        isrx[0] = 0x90; /* nop */
        isrx[1] = 0x90; /* nop */
    }
    else
    {
        isrx[0] = 0x6a; /* push imm8 (0x00 — dummy error code) */
        isrx[1] = 0x00;
    }

    isrx[2] = 0x6a;     /* push imm8 (vector number) */
    isrx[3] = vector;

    /* Relative jump into the shared __isr stub */
    isrx[4] = 0xe9;
    base::size_t rva = base::size_t(isrx + 5);
    *((base::uint32_t *)rva) = base::uint32_t(mm::virt_addr_t(__isr) - (rva + 4));

    const mm::virt_addr_t isr0a = mm::virt_addr_t(isrx);
    idt[vector].offset_low_word    = (isr0a >> 0)  & 0xFFFF;
    idt[vector].offset_mid_word    = (isr0a >> 16) & 0xFFFF;
    idt[vector].offset_high_dword  = (isr0a >> 32);
    idt[vector].p = 1;
}

static auto __isr_common(isr_frame *frame) -> void
{
    interrupt_eoi(frame->vector);
    if (interrupt_handlers[frame->vector])
        interrupt_handlers[frame->vector](frame, (frame->cs & 3) == 3);
}

static auto idt_desc_init(idt_desc idt[IDT_DESC_CNT]) -> void
{
    for (base::size_t i = 0; i < IDT_DESC_CNT; i++)
    {
        idt[i].selector = (1 << SELECTOR_INDEX);
        idt[i].type     = X64_INTR_GATE;
    }
    /* Patch the call rel32 inside the shared __isr stub to point at __isr_common */
    base::size_t rva = base::size_t(__isr + 0x1b);
    *((uint32_t *)rva) = uint32_t(mm::virt_addr_t(__isr_common) - (rva + 4));
}

auto idt_init() -> void
{
    idt = new idt_desc[IDT_DESC_CNT];
    idt_desc_init(idt);

    if (apic_check())
        apic_init();
    else
        pic_init();

    struct __attribute__((packed))
    {
        uint16_t         limit;
        mm::virt_addr_t  base;
    } idtr = {.limit = 0x1000, .base = (mm::virt_addr_t)idt};

    asm volatile("lidt %0" ::"m"(idtr));
}
