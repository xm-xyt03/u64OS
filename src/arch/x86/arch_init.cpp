#include <arch_init.hpp>

extern "C"
{
#include <boot/tty.h>
}

// Declared in arch/x86/traps/traps.cpp
extern auto idt_init() -> void;

namespace arch
{
    auto arch_init(multiboot_uint8_t *mbi) -> void
    {
        (void)mbi;

        // Set up the interrupt descriptor table, configure APIC or PIC,
        // and register all exception/interrupt service routines.
        idt_init();
        boot_puts("[+] arch: IDT initialized.\n");
    }
}
