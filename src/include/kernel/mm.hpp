#ifndef KERNEL_MM_HPP
#define KERNEL_MM_HPP

#include <kernel/mm/types.hpp>
#include <kernel/mm/virt_layout.hpp>
#include <kernel/mm/page_mm.hpp>
#include <kernel/mm/heap_mm.hpp>
#include <kernel/mm/ioremap.hpp>

namespace mm
{
    auto mm_core_init(void) -> void;
};

#endif // !KERNEL_MM_HPP