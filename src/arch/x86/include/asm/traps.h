#ifndef X86_ASM_TRAPS_H
#define X86_ASM_TRAPS_H

#include <kernel/base.hpp>

#define X64_INTR_GATE 0xE
#define X64_TRAP_GATE 0xF

typedef struct
{
    base::size_t r15;
    base::size_t r14;
    base::size_t r13;
    base::size_t r12;
    base::size_t r11;
    base::size_t r10;
    base::size_t r9;
    base::size_t r8;
    base::size_t rdi;
    base::size_t rsi;
    base::size_t rbp;
    base::size_t rbx;
    base::size_t rdx;
    base::size_t rcx;
    base::size_t rax;

    base::size_t vector;
    base::size_t err_code;

    base::size_t rip;
    base::size_t cs;
    base::size_t rflags;
} isr_frame;

extern auto register_interrupt(base::uint8_t vector, void (*routine)(isr_frame *, bool)) -> void;

#endif // !X86_ASM_TRAPS_H