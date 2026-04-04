#define ASM_FILE 1
#include <boot/multiboot2.h>
#include <asm/cpu_types.h>
#include <asm/page_types.h>

#define GRUB_MULTIBOOT_ARCHITECTURE_I386 (0)
#define MULTIBOOT2_HEADER_LEN (multiboot_header_end - multiboot_header)
#define MULTIBOOT2_HEADER_CHECKSUM -(MULTIBOOT2_HEADER_MAGIC + GRUB_MULTIBOOT_ARCHITECTURE_I386 + MULTIBOOT2_HEADER_LEN)

.section .boot.loader.header
    .align 8

    multiboot_header:
        .long MULTIBOOT2_HEADER_MAGIC
        .long GRUB_MULTIBOOT_ARCHITECTURE_I386
        .long MULTIBOOT2_HEADER_LEN
        .long MULTIBOOT2_HEADER_CHECKSUM

    tag_entry:
        .align 8
        .short MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS
        .short 0
        .long 12
        .long _start

    tag_frame_buffer:
        .align 8
        .short MULTIBOOT_HEADER_TAG_FRAMEBUFFER
        .short 0
        .long 20
        .long 1024
        .long 768
        .long 32

    tag_end:
        .align 8
        .short MULTIBOOT_HEADER_TAG_END
        .short 0
        .long 8
    multiboot_header_end:

.section .boot.loader.text32:
    .code32
    .align 0x1000
    .extern boot_main
    
    .globl _start

    _start:
        cli
        
        cmpl $MULTIBOOT2_BOOTLOADER_MAGIC, %eax
        jne .loop

        movl $boot_stack_top, %esp

        movl %eax, %edi
        movl %ebx, %esi

        pushl $0
        popf

        movl %cr0, %eax
        movl $CR0_PG, %ebx
        notl %ebx
        andl %ebx, %eax
        movl %eax, %cr0

        movl $boot_pud, %eax
        orl $0b11, %eax /* 0b11 -> PTE_ATTR_P | PTE_ATTR_RW */
        movl %eax, boot_pgd

        xorl %eax, %eax
        orl $0b10000011, %eax /* 0b10000011 -> PDE_ATTR_P | PDE_ATTR_RW | PDE_ATTR_PS */
        movl %eax, boot_pud

        xorl %eax, %eax
        movl %eax, (boot_pgd + 4)
        movl %eax, (boot_pud + 4)

        movl $boot_pgd, %eax
        movl %eax, %cr3

        movl %cr4, %eax
        orl $CR4_PAE, %eax
        orl $CR4_PGE, %eax
        movl %eax, %cr4

        movl $0xC0000080, %ecx
        rdmsr
        orl $0b100000000, %eax
        wrmsr

        movl %cr0, %eax
        orl $CR0_PG, %eax
        movl %eax, %cr0
        
        movl $gdt64_ptr, %eax
        lgdt (%eax)

        movw $0b10000, %ax /* 0b10000 -> (2 << SELECTOR_INDEX) */
        mov %ax, %ds
        mov %ax, %ss
        mov %ax, %es
        mov %ax, %fs
        mov %ax, %gs

        jmp $0b1000, $boot_main /* 0b1000 -> (1 << SELECTOR_INDEX) */
    .loop:
        hlt
        jmp .loop

.section .boot.loader.data
    .align 0x1000

    .globl boot_stack, boot_stack_top, boot_pgd, boot_pud

    boot_stack:
        .space 0x1000
    boot_stack_top:

    boot_pgd:
        .space 0x1000
    boot_pud:
        .space 0x1000

    .align 0x1000
    .globl gdt64, gdt64_ptr
    gdt64:
        .quad 0
    gdt64_code_segment:
        .quad 0x00209A0000000000
    gdt64_data_segment:
        .quad 0x0000920000000000
    gdt64_ptr:
        .short gdt64_ptr - gdt64 - 1
        .long gdt64