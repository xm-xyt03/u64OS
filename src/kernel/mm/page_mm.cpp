#include <kernel/mm/page_mm.hpp>

extern "C"
{
#include <asm/mmu.h>
#include <asm/page_types.h>
}

namespace mm
{
    Page *pgdb_base;
    base::size_t pgdb_page_nr;

    base::uint8_t GloblPagePoolMem[sizeof(PagePool)];
    PagePool *GloblPagePool = (PagePool *)&GloblPagePoolMem;
    PagePool *GloblPagePoolGroup[PAGE_POOL_TYPE_NR] = {
        (PagePool *)&GloblPagePoolMem,
    };

    PagePool::PagePool(void) {}
    PagePool::~PagePool(void) {}

    auto PagePool::__reinit_page(Page *p, base::size_t order, bool free) -> void
    {
        for (auto i = 0; i < (1 << order); i++)
        {
            p[i].is_free = free;
            p[i].order = order;
            p[i].freelist = nullptr;
            p[i].kc = nullptr;
            p[i].is_head = false;
            lib::atomic::atomic_set(&p->ref_count, -1);
            lib::atomic::atomic_set(&p->map_count, -1);
        }

        p[0].is_head = true;
    }

    auto PagePool::__alloc_page_direct(base::size_t order) -> Page *
    {
        Page *p = nullptr;
        Page *buddy;
        base::size_t allocated = order;

        while (allocated < MAX_PAGE_ORDER)
        {
            if (!lib::list_empty(&this->freelist[allocated]))
            {
                p = lib::list_entry(this->freelist[allocated].next, &Page::list);
                lib::list_del(&p->list);
                break;
            }
            else
                allocated++;
        }

        if (!p)
            goto out;

        if (allocated != order)
        {
            do
            {
                allocated--;
                buddy = get_page_buddy(p, allocated);
                this->__reinit_page(buddy, allocated, true);
                lib::list_add_next(&this->freelist[allocated], &buddy->list);
            } while (allocated > order);
        }

        this->__reinit_page(p, allocated, false);

    out:
        return p;
    }
    
    auto PagePool::__alloc_pages(base::size_t order) -> Page *
    {
        Page *p = nullptr;
        bool redo = false;

        if (order >= MAX_PAGE_ORDER)
            return nullptr;

        this->lock.Lock();

    redo:
        p = this->__alloc_page_direct(order);
        if (p)
        {
            goto out;
        }

        if (!redo)
        {
            this->__reclaim_memory();
            redo = true;
            goto redo;
        }

    out:
        this->lock.UnLock();
        return p;
    }

    auto PagePool::__free_pages(Page *p, base::size_t order) -> void
    {
        if (!p || order >= MAX_PAGE_ORDER)
            return;

        this->lock.Lock();

        while (order < (MAX_PAGE_ORDER - 1))
        {
            Page *buddy;

            buddy = get_page_buddy(p, order);
            if (buddy->type == PAGE_NORMAL_MEM && buddy->is_head && buddy->is_free)
            {
                lib::list_del(&buddy->list);
                if (buddy < p)
                {
                    p->is_head = false;
                    p = buddy;
                }
                order++;
                continue;
            }

            break;
        }

        this->__reinit_page(p, order, true);
        lib::list_add_next(&(this->freelist[order]), &p->list);
        this->lock.UnLock();
    }
    
    auto PagePool::__reclaim_memory(void) -> void
    {
        // TODO: reclaim movable memory pages
    }

    auto PagePool::AllocPages(base::size_t order) -> Page *
    {
        return this->__alloc_pages(order);
    }

    auto PagePool::FreePages(Page *page, base::size_t order) -> void
    {
        this->__free_pages(page, order);
    }

    auto PagePool::Init(void) -> void
    {
        for (auto i = 0; i < MAX_PAGE_ORDER; i++)
            lib::list_head_init(&this->freelist[i]);

        this->lock.Reset();
    }

    auto PagePool::AddPages(Page *page, base::size_t order) -> void
    {
        for (auto i = 0; i < (1 << order); i++)
            page->pool = this;

        this->FreePages(page, order);
    }

    /*
     * kern_pgtable_walk - Walk the live 4-level page table for vaddr.
     *
     * The current CR3 points to the PGD physical address.  All intermediate
     * page-table pages reside inside the direct-map region, so we can reach
     * them with phys_to_virt().
     *
     * If alloc == true and an intermediate entry is absent, a new zeroed page
     * is allocated from GloblPagePool and installed.
     *
     * Returns a pointer to the pte_t for vaddr, or nullptr on failure.
     */
    auto kern_pgtable_walk(virt_addr_t vaddr, bool alloc) -> pte_t *
    {
        pgd_t *pgd;
        pud_t *pud;
        pmd_t *pmd;
        pte_t *pte;

        int pgd_i = PGD_ENTRY(vaddr);
        int pud_i = PUD_ENTRY(vaddr);
        int pmd_i = PMD_ENTRY(vaddr);
        int pte_i = PTE_ENTRY(vaddr);

        /* PGD — obtained from current CR3 (physical address → virtual) */
        phys_addr_t cr3 = read_cr3() & PAGE_MASK;
        pgd = (pgd_t *)phys_to_virt(cr3);

        /* PUD */
        if (!pgd[pgd_i])
        {
            if (!alloc)
                return nullptr;

            Page *p = GloblPagePool->AllocPages(0);
            if (!p)
                return nullptr;

            virt_addr_t va = page_to_virt(p);
            for (base::size_t i = 0; i < PAGE_SIZE / sizeof(base::size_t); i++)
                ((base::size_t *)va)[i] = 0;
            pgd[pgd_i] = (pgd_t)virt_to_phys(va) | PDE_DEFAULT;
        }
        pud = (pud_t *)phys_to_virt((phys_addr_t)(pgd[pgd_i] & PAGE_MASK));

        /* PMD */
        if (!pud[pud_i])
        {
            if (!alloc)
                return nullptr;

            Page *p = GloblPagePool->AllocPages(0);
            if (!p)
                return nullptr;

            virt_addr_t va = page_to_virt(p);
            for (base::size_t i = 0; i < PAGE_SIZE / sizeof(base::size_t); i++)
                ((base::size_t *)va)[i] = 0;
            pud[pud_i] = (pud_t)virt_to_phys(va) | PDE_DEFAULT;
        }
        pmd = (pmd_t *)phys_to_virt((phys_addr_t)(pud[pud_i] & PAGE_MASK));

        /* PTE */
        if (!pmd[pmd_i])
        {
            if (!alloc)
                return nullptr;

            Page *p = GloblPagePool->AllocPages(0);
            if (!p)
                return nullptr;

            virt_addr_t va = page_to_virt(p);
            for (base::size_t i = 0; i < PAGE_SIZE / sizeof(base::size_t); i++)
                ((base::size_t *)va)[i] = 0;
            pmd[pmd_i] = (pmd_t)virt_to_phys(va) | PDE_DEFAULT;
        }
        pte = (pte_t *)phys_to_virt((phys_addr_t)(pmd[pmd_i] & PAGE_MASK));

        return &pte[pte_i];
    }

    auto kern_pgtable_set(virt_addr_t vaddr, phys_addr_t paddr, page_attr_t attr) -> int
    {
        pte_t *pte = kern_pgtable_walk(vaddr, true);
        if (!pte)
            return -1;

        *pte = (paddr & PAGE_MASK) | attr;
        flush_tlb_single(vaddr);
        return 0;
    }

    auto kern_pgtable_clear(virt_addr_t vaddr) -> void
    {
        pte_t *pte = kern_pgtable_walk(vaddr, false);
        if (!pte || !(*pte & PTE_ATTR_P))
            return;

        *pte = 0;
        flush_tlb_single(vaddr);
    }
}