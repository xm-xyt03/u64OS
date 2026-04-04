#ifndef ARCH_X86_ARCH_INIT_HPP
#define ARCH_X86_ARCH_INIT_HPP

#include <boot/multiboot2.h>

namespace arch
{
    /// Initialize all x86-64 architecture-specific subsystems
    /// This includes:
    /// - Early boot paging setup
    /// - TTY (framebuffer + serial) initialization
    /// - Boot-phase memory management
    /// - Interrupt descriptor table setup
    ///
    /// @param mbi Multiboot2 information structure
    auto arch_init(multiboot_uint8_t *mbi) -> void;
}

#endif // !ARCH_X86_ARCH_INIT_HPP
