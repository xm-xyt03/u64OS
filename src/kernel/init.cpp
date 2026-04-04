#include <kernel/init.hpp>
#include <kernel/base.hpp>
#include <kernel/mm.hpp>
#include <u64OS/cpp_base.hpp>
#include <arch_init.hpp>

extern "C"
{
#include <boot/tty.h>
}

namespace init
{
    auto global_constructor_caller(void) -> int
    {
        int (**init_array)(void) = &__init_array;
        int (**init_array_end)(void) = &__init_array_end;
        int error;

        for (base::size_t i = 0; init_array[i] && ((init_array + i) < init_array_end); i++)
        {
            error = init_array[i]();
            if (error)
                return error;
        }

        return 0;
    }

    auto kernel_init(multiboot_uint8_t *mbi) -> void
    {
        // Initialize physical page allocator and kernel heap
        mm::mm_core_init();
        boot_puts("[+] Core memory management initialized.\n");

        // Run C++ global constructors (must run after mm is ready)
        if (global_constructor_caller() < 0)
        {
            boot_puts("[x] FAILED at invoking global constructors. Halting...\n");
            asm volatile("hlt");
        }

        // Initialize the I/O remapping virtual memory region
        mm::ioremap_init();
        boot_puts("[+] I/O remap region initialized.\n");

        // Initialize all architecture-specific subsystems
        // (IDT, APIC/PIC — requires ioremap to already be set up)
        arch::arch_init(mbi);
    }
}
