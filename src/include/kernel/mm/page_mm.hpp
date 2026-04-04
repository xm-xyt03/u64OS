#ifndef KERNEL_MM_PAGE_MM_HPP
#define KERNEL_MM_PAGE_MM_HPP

#include <kernel/mm/virt_layout.hpp>
#include <kernel/mm/types.hpp>

#include <u64OS/compiler.h>

#include <kernel/lib.hpp>
#include <kernel/base.hpp>

extern "C"
{
#include <asm/page_types.h>
}

namespace mm
{
    class KMemCache;
    class PagePool;

    typedef base::uint64_t page_attr_t;

    enum
    {
        PAGE_NON_EXISTED,
        PAGE_NORMAL_MEM,
        PAGE_RESERVED,
        PAGE_ACPI_RECLAIMABLE,
        PAGE_NVS,
        PAGE_BADRAM,
        PAGE_UNKNOWN
    };

    enum migrate_type
    {
        MIGRATE_UNMOVABLE,
        MIGRATE_MOVABLE
    };

    struct Page
    {
        lib::ListHead list;
        struct
        {
            unsigned type : 4;
            unsigned migrate_type : 4;
            unsigned is_free : 1;
            unsigned is_head : 1;
            unsigned order : 4;
        };
        lib::atomic::atomic_t ref_count;
        lib::atomic::atomic_t map_count;
        lib::atomic::SpinLock lock;
        void **freelist;
        KMemCache *kc;
        PagePool *pool;
        base::size_t obj_nr;

        base::size_t unused[0];
    } __attribute__((aligned(64)));

    extern Page *pgdb_base;
    extern base::size_t pgdb_page_nr;

    inline constexpr base::size_t PGDB_PG_PAGE_NR(PAGE_SIZE / sizeof(Page));

    __always_inline auto page_to_pfn(Page *p) -> pfn_t
    {
        return p - pgdb_base;
    }

    __always_inline auto pfn_to_page(pfn_t pfn) -> Page *
    {
        return &pgdb_base[pfn];
    }

    __always_inline auto page_to_phys(Page *p) -> phys_addr_t
    {
        return (p - pgdb_base) * PAGE_SIZE;
    }

    __always_inline auto page_to_virt(Page *p) -> virt_addr_t
    {
        return physmem_base + page_to_phys(p);
    }

    __always_inline auto virt_to_phys(virt_addr_t addr) -> phys_addr_t
    {
        return addr - physmem_base;
    }

    __always_inline auto phys_to_virt(phys_addr_t addr) -> virt_addr_t
    {
        return addr + physmem_base;
    }

    __always_inline auto phys_to_page(phys_addr_t addr) -> Page *
    {
        return &pgdb_base[(addr & PAGE_MASK) / PAGE_SIZE];
    }

    __always_inline auto virt_to_page(virt_addr_t addr) -> Page *
    {
        return phys_to_page(virt_to_phys(addr));
    }

    __always_inline auto get_head_page(Page *p) -> Page *
    {
        return p->is_head
                   ? p
                   : (Page *)((virt_addr_t)p & ~((sizeof(*p) * (1 << p->order)) - 1));
    }

    __always_inline auto buddy_page_pfn(pfn_t pfn, int order) -> base::size_t
    {
        return pfn ^ (1 << order);
    }

    __always_inline auto get_page_buddy(Page *p, int order) -> Page *
    {
        pfn_t pfn = page_to_pfn(p);
        return pfn_to_page(buddy_page_pfn(pfn, order));
    }

    inline constexpr base::size_t MAX_PAGE_ORDER = 11;

    class PagePool
    {
    public:
        PagePool(void);
        ~PagePool(void);

        auto AllocPages(base::size_t order) -> Page *;
        auto FreePages(Page *page, base::size_t order) -> void;

        auto Init(void) -> void;
        auto AddPages(Page *page, base::size_t order) -> void;

    private:
        lib::ListHead freelist[MAX_PAGE_ORDER];
        lib::atomic::SpinLock lock;

        auto __reinit_page(Page *p, base::size_t order, bool free) -> void;

        auto __alloc_page_direct(base::size_t order) -> Page *;
        auto __alloc_pages(base::size_t order) -> Page *;

        auto __free_pages(Page *p, base::size_t order) -> void;

        auto __reclaim_memory(void) -> void;
    };

    enum page_pool_types
    {
        PAGE_POOL_TYPE_NORMAL = 0,
        PAGE_POOL_TYPE_NR,
    };

    extern base::uint8_t GloblPagePoolMem[sizeof(PagePool)];
    extern PagePool *GloblPagePool;
    extern PagePool *GloblPagePoolGroup[PAGE_POOL_TYPE_NR];

    __always_inline auto get_page(Page *p) -> void
    {
        lib::atomic::atomic_inc(&get_head_page(p)->ref_count);
    }

    __always_inline auto put_page(Page *p) -> void
    {
        p = get_head_page(p);

        if (lib::atomic::atomic_dec(&p->ref_count) < 0)
            p->pool->FreePages(p, p->order);
    }

    /*
     * Runtime page table operations.
     *
     * These functions operate on the currently loaded page table (CR3) at
     * runtime, after the MM subsystem has been fully initialised.  They are
     * intended for use by ioremap and similar subsystems that need to add or
     * remove kernel virtual→physical mappings dynamically.
     */

    /*
     * kern_pgtable_walk - Walk the current page table to the PTE for vaddr.
     *
     * @vaddr : The virtual address whose PTE is sought.
     * @alloc : If true, allocate intermediate page-table pages as needed.
     *
     * Returns a pointer to the pte_t for vaddr, or nullptr on failure.
     */
    auto kern_pgtable_walk(virt_addr_t vaddr, bool alloc) -> pte_t *;

    /*
     * kern_pgtable_set - Install a mapping for vaddr → paddr with attr.
     *
     * Walks/creates the page table, sets the PTE, and invalidates the TLB
     * entry for vaddr.
     *
     * Returns 0 on success, negative on error.
     */
    auto kern_pgtable_set(virt_addr_t vaddr, phys_addr_t paddr, page_attr_t attr) -> int;

    /*
     * kern_pgtable_clear - Remove the mapping for vaddr.
     *
     * Clears the PTE and invalidates the TLB entry.  Does nothing if the
     * PTE is already absent.
     */
    auto kern_pgtable_clear(virt_addr_t vaddr) -> void;
}

#endif // !KERNEL_MM_PAGE_MM_HPP