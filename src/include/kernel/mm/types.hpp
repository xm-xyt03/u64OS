#ifndef KERNEL_MM_TYPES_HPP
#define KERNEL_MM_TYPES_HPP

#include <kernel/base.hpp>

namespace mm
{
    typedef base::size_t virt_addr_t;
    typedef base::size_t phys_addr_t;
    typedef base::size_t dma_addr_t;
    typedef base::size_t pfn_t;

    inline constexpr base::size_t PAGE_SHIFT = 12;
    inline constexpr base::size_t PAGE_SIZE = (1UL << PAGE_SHIFT);
    inline constexpr base::size_t PAGE_MASK = (~(PAGE_SIZE - 1));
}

#endif // !KERNEL_MM_TYPES_HPP