/*
 * ioremap.cpp — dynamic physical→virtual mapping for MMIO regions.
 *
 * Architecture
 * ────────────
 *  • Virtual address space:  KERN_DYNAMIC_MAP_REGION_BASE … _END  (1 GB)
 *  • VmaAllocator            free-list of VmaRegion nodes, sorted by address,
 *                            first-fit allocation, adjacent regions merged on
 *                            free.  Protected by vma_lock.
 *  • IoMapTree               red-black tree of IoMapEntry nodes keyed by
 *                            virtual address, for O(log n) iounmap look-up.
 *                            Protected by map_lock.
 *
 * Locking order (if both needed): vma_lock before map_lock.
 */

#include <kernel/mm/ioremap.hpp>
#include <kernel/mm/page_mm.hpp>
#include <kernel/mm/heap_mm.hpp>
#include <kernel/mm/virt_layout.hpp>

extern "C"
{
#include <asm/page_types.h>
#include <asm/mmu.h>
}

namespace mm
{

    /* ====================================================================== */
    /* Comparator for the IoMapEntry red-black tree (keyed by vaddr)          */
    /* ====================================================================== */

    struct IoMapCmp
    {
        /* Used by RbTree::Insert */
        static auto compare(const lib::RbNode *a, const lib::RbNode *b) -> int
        {
            auto *ea = lib::container_of(a, &IoMapEntry::rb_node);
            auto *eb = lib::container_of(b, &IoMapEntry::rb_node);
            if (ea->vaddr < eb->vaddr)
                return -1;
            if (ea->vaddr > eb->vaddr)
                return 1;
            return 0;
        }

        /* Used by RbTree::Find — key type is virt_addr_t */
        static auto compare_key(const lib::RbNode *node, const virt_addr_t *key) -> int
        {
            auto *e = lib::container_of(node, &IoMapEntry::rb_node);
            if (e->vaddr > *key)
                return 1;
            if (e->vaddr < *key)
                return -1;
            return 0;
        }
    };

    /* ====================================================================== */
    /* Module-level state                                                      */
    /* ====================================================================== */

    /* Lock protecting the VMA free-list */
    static lib::atomic::SpinLock vma_lock;

    /* Sentinel head of the sorted free-list */
    static lib::ListHead vma_free_list;

    /* Lock protecting the IoMap red-black tree */
    static lib::atomic::SpinLock map_lock;

    /* Red-black tree of active mappings */
    static lib::RbTree<IoMapCmp> io_map_tree;

    /* ====================================================================== */
    /* Small object allocators for VmaRegion and IoMapEntry                   */
    /* These are thin wrappers around the kernel heap.                        */
    /* ====================================================================== */

    static auto vma_region_alloc(void) -> VmaRegion *
    {
        return (VmaRegion *)GloblKHeapPool->Malloc(sizeof(VmaRegion));
    }

    static auto vma_region_free(VmaRegion *r) -> void
    {
        GloblKHeapPool->Free(r);
    }

    static auto iomap_entry_alloc(void) -> IoMapEntry *
    {
        return (IoMapEntry *)GloblKHeapPool->Malloc(sizeof(IoMapEntry));
    }

    static auto iomap_entry_free(IoMapEntry *e) -> void
    {
        GloblKHeapPool->Free(e);
    }

    /* ====================================================================== */
    /* VMA allocator helpers                                                   */
    /* ====================================================================== */

    /*
     * vma_alloc - Allocate page_count pages from the dynamic-map free-list.
     *
     * Returns the virtual base address on success, or 0 on failure.
     *
     * Caller must hold vma_lock.
     */
    static auto vma_alloc(base::size_t page_count) -> virt_addr_t
    {
        lib::ListHead *pos = vma_free_list.next;
        while (pos != &vma_free_list)
        {
            VmaRegion *r = lib::list_entry(pos, &VmaRegion::list);
            pos = pos->next; /* advance before potentially removing r */

            if (r->page_count < page_count)
                continue;

            virt_addr_t base = r->base;

            if (r->page_count == page_count)
            {
                /* Exact fit: remove the region entirely */
                lib::list_del(&r->list);
                vma_region_free(r);
            }
            else
            {
                /* Trim the front of the region */
                r->base += page_count * PAGE_SIZE;
                r->page_count -= page_count;
            }

            return base;
        }

        return 0; /* out of virtual address space */
    }

    /*
     * vma_free - Return page_count pages starting at base to the free-list.
     *
     * Adjacent free regions are merged to reduce fragmentation.
     *
     * Caller must hold vma_lock.
     */
    static auto vma_free(virt_addr_t base, base::size_t page_count) -> void
    {
        virt_addr_t end = base + page_count * PAGE_SIZE;

        /* Find the insertion point (keep list sorted by address) */
        lib::ListHead *prev_node = &vma_free_list;

        for (lib::ListHead *pos = vma_free_list.next; pos != &vma_free_list; pos = pos->next)
        {
            VmaRegion *r = lib::list_entry(pos, &VmaRegion::list);
            if (r->base >= end)
                break;
            prev_node = pos;
        }

        /* Check if we can merge with the successor */
        VmaRegion *succ = nullptr;
        if (prev_node->next != &vma_free_list)
        {
            VmaRegion *candidate = lib::list_entry(prev_node->next, &VmaRegion::list);
            if (candidate->base == end)
                succ = candidate;
        }

        /* Check if we can merge with the predecessor */
        VmaRegion *pred = nullptr;
        if (prev_node != &vma_free_list)
        {
            VmaRegion *candidate = lib::list_entry(prev_node, &VmaRegion::list);
            if (candidate->base + candidate->page_count * PAGE_SIZE == base)
                pred = candidate;
        }

        if (pred && succ)
        {
            /* Merge all three into pred */
            pred->page_count += page_count + succ->page_count;
            lib::list_del(&succ->list);
            vma_region_free(succ);
        }
        else if (pred)
        {
            pred->page_count += page_count;
        }
        else if (succ)
        {
            succ->base = base;
            succ->page_count += page_count;
        }
        else
        {
            /* No merge: allocate a new VmaRegion */
            VmaRegion *nr = vma_region_alloc();
            if (!nr)
                return; /* leak rather than corrupt — shouldn't happen */

            nr->base = base;
            nr->page_count = page_count;
            lib::list_head_init(&nr->list);
            /* Insert nr after prev_node: list_add_next(head, new) inserts new after head */
            lib::list_add_next(prev_node, &nr->list);
        }
    }

    /* ====================================================================== */
    /* Public API                                                              */
    /* ====================================================================== */

    auto ioremap_init(void) -> void
    {
        vma_lock.Reset();
        map_lock.Reset();
        lib::list_head_init(&vma_free_list);

        /* Populate the VMA free-list with the entire dynamic-map region */
        VmaRegion *region = vma_region_alloc();
        if (!region)
            return;

        base::size_t total_pages =
            (KERN_DYNAMIC_MAP_REGION_END - KERN_DYNAMIC_MAP_REGION_BASE + 1) / PAGE_SIZE;

        region->base = KERN_DYNAMIC_MAP_REGION_BASE;
        region->page_count = total_pages;
        lib::list_head_init(&region->list);
        lib::list_add_next(&vma_free_list, &region->list);
    }

    auto ioremap(phys_addr_t paddr, base::size_t size) -> virt_addr_t
    {
        if (!size)
            return 0;

        /* Page-align the physical range, preserving the sub-page offset */
        base::size_t offset = paddr & ~PAGE_MASK;
        phys_addr_t page_phys = paddr & PAGE_MASK;
        base::size_t page_count =
            (offset + size + PAGE_SIZE - 1) / PAGE_SIZE;

        /* Allocate virtual address range */
        vma_lock.Lock();
        virt_addr_t vbase = vma_alloc(page_count);
        vma_lock.UnLock();

        if (!vbase)
            return 0;

        /* Map each page: cache-disabled, write-through (standard for MMIO) */
        page_attr_t attr = PTE_ATTR_P | PTE_ATTR_RW | PTE_ATTR_PCD | PTE_ATTR_PWT;

        for (base::size_t i = 0; i < page_count; i++)
        {
            int ret = kern_pgtable_set(vbase + i * PAGE_SIZE,
                                       page_phys + i * PAGE_SIZE,
                                       attr);
            if (ret < 0)
            {
                /* Roll back successfully installed mappings */
                for (base::size_t j = 0; j < i; j++)
                    kern_pgtable_clear(vbase + j * PAGE_SIZE);

                vma_lock.Lock();
                vma_free(vbase, page_count);
                vma_lock.UnLock();

                return 0;
            }
        }

        /* Record the mapping in the red-black tree */
        IoMapEntry *entry = iomap_entry_alloc();
        if (!entry)
        {
            for (base::size_t i = 0; i < page_count; i++)
                kern_pgtable_clear(vbase + i * PAGE_SIZE);

            vma_lock.Lock();
            vma_free(vbase, page_count);
            vma_lock.UnLock();

            return 0;
        }

        entry->vaddr = vbase;
        entry->paddr = page_phys;
        entry->page_count = page_count;
        entry->orig_offset = offset;

        map_lock.Lock();
        io_map_tree.Insert(&entry->rb_node);
        map_lock.UnLock();

        return vbase + offset;
    }

    auto iounmap(virt_addr_t vaddr) -> int
    {
        /* Recover the page-aligned base */
        virt_addr_t vbase = vaddr & PAGE_MASK;

        map_lock.Lock();
        lib::RbNode *node = io_map_tree.Find(&vbase);
        if (!node)
        {
            map_lock.UnLock();
            return -1;
        }

        IoMapEntry *entry = lib::container_of(node, &IoMapEntry::rb_node);
        io_map_tree.Remove(node);
        map_lock.UnLock();

        /* Remove page-table entries and flush TLB for each page */
        for (base::size_t i = 0; i < entry->page_count; i++)
            kern_pgtable_clear(entry->vaddr + i * PAGE_SIZE);

        /* Return virtual address range to the allocator */
        vma_lock.Lock();
        vma_free(entry->vaddr, entry->page_count);
        vma_lock.UnLock();

        iomap_entry_free(entry);
        return 0;
    }

} /* namespace mm */
