#ifndef KERNEL_INIT_HPP
#define KERNEL_INIT_HPP

#include <boot/multiboot2.h>

namespace init
{
    /// Run all C++ global constructors in __init_array
    ///
    /// @return 0 on success, negative error code on failure
    auto global_constructor_caller(void) -> int;

    /// Initialize all architecture-independent kernel subsystems
    /// Order:
    ///   1. Physical page allocator + kernel heap (mm::mm_core_init)
    ///   2. C++ global constructors
    ///   3. I/O remapping region (mm::ioremap_init)
    ///   4. Architecture-specific init (arch::arch_init)
    ///
    /// @param mbi Multiboot2 information structure
    auto kernel_init(multiboot_uint8_t *mbi) -> void;
}

#endif // !KERNEL_INIT_HPP
