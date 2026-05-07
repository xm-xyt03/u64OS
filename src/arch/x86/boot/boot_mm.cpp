#include <kernel/base.hpp>
#include <kernel/lib.hpp>
#include <kernel/mm.hpp>

extern "C"
{

#include <u64OS/elf.h>
#include <u64OS/err.h>
#include <boot/multiboot2.h>
#include <boot/tty.h>
#include <boot/string.h>
#include <asm/page_types.h>
}

static struct multiboot_tag_mmap *mmap_tag = nullptr;
static uint32_t mmap_entry_nr;

struct multiboot_mmap_entry *curr_entry = nullptr;
int curr_entry_idx = -1;
mm::phys_addr_t curr_avail, curr_end;

static auto boot_mm_palloc_internal(void) -> void *
{
    void *res;

    if (!curr_entry)
    {
        for (int i = (curr_entry_idx + 1); i < mmap_entry_nr; i++)
        {
            if (mmap_tag->entries[i].type == MULTIBOOT_MEMORY_AVAILABLE)
            {
                mm::phys_addr_t base = mmap_tag->entries[i].addr;
                mm::phys_addr_t end = base + mmap_tag->entries[i].len;

                if (base > end)
                {
                    boot_puts("[x] FATAL ERROR: "
                              "integeter overflow at parsing multiboot tags");
                    asm volatile("hlt");
                }

                base = PAGE_ALIGN(base);
                end &= PAGE_MASK;

                if ((end < base) || ((end - base) < PAGE_SIZE))
                    continue;

                curr_avail = base;
                curr_end = end;
                curr_entry = &mmap_tag->entries[i];
                curr_entry_idx = i;
                break;
            }
        }
    }

    if (!curr_entry)
    {
        boot_puts("[x] FATAL ERROR: NO MEMORY AVAILABLE!");
        return ERR_PTR(-ENOMEM);
    }

    res = (void *)curr_avail;
    curr_avail += PAGE_SIZE;

    if (curr_avail == curr_end)
        curr_entry = nullptr;

    return res;
}

static struct multiboot_tag_elf_sections *elf_info_tag = nullptr;
mm::phys_addr_t multiboot_tag_start, multiboot_tag_end;

/* Pre-computed sorted array of [start, end) physical ranges that are in use.
 * Built once in boot_mm_build_used_ranges(); afterwards addr_in_used_range()
 * uses a binary search instead of a linear ELF-section scan on every call.
 */
struct UsedRange {
    mm::phys_addr_t start;
    mm::phys_addr_t end;
};

#define MAX_USED_RANGES 64
static UsedRange used_ranges[MAX_USED_RANGES];
static int used_range_count = 0;

/* Insert a range into the sorted used_ranges array (insertion sort – called
 * only a handful of times during boot, so O(N²) is irrelevant here). */
static auto used_range_insert(mm::phys_addr_t start, mm::phys_addr_t end) -> void
{
    if (used_range_count >= MAX_USED_RANGES)
        return;

    int i = used_range_count - 1;
    while (i >= 0 && used_ranges[i].start > start)
    {
        used_ranges[i + 1] = used_ranges[i];
        i--;
    }
    used_ranges[i + 1] = {start, end};
    used_range_count++;
}

/* Build the used_ranges cache from ELF sections, framebuffer, and multiboot
 * tags.  Must be called after elf_info_tag / multiboot_tag_{start,end} are
 * set.  O(N log N) one-time cost. */
static auto boot_mm_build_used_ranges(void) -> void
{
    Elf64_Shdr *shdr;
    void *shdr_end;

    shdr_end = (void *)((mm::phys_addr_t)&elf_info_tag->sections +
                        elf_info_tag->num * sizeof(*shdr));

    for (shdr = (Elf64_Shdr *)&elf_info_tag->sections;
         (void *)shdr < shdr_end;
         shdr = (Elf64_Shdr *)((mm::phys_addr_t)shdr + sizeof(*shdr)))
    {
        if ((shdr->sh_type != SHT_PROGBITS) || !(shdr->sh_flags & SHF_ALLOC))
            continue;

        mm::phys_addr_t seg_start = (0x100000 + shdr->sh_offset) & PAGE_MASK;
        mm::phys_addr_t seg_end   = PAGE_ALIGN(seg_start + shdr->sh_size);
        used_range_insert(seg_start, seg_end);
    }

    if (boot_tty_has_fb())
    {
        mm::phys_addr_t fb_start = ((mm::phys_addr_t)boot_fb_base) & PAGE_MASK;
        mm::phys_addr_t fb_end   = PAGE_ALIGN((mm::phys_addr_t)boot_fb_end + 1);
        used_range_insert(fb_start, fb_end);
    }

    used_range_insert(multiboot_tag_start, multiboot_tag_end);
}

/* O(log N) binary-search replacement for the old O(N) linear scan. */
static auto addr_in_used_range(mm::phys_addr_t addr) -> bool
{
    int lo = 0, hi = used_range_count - 1;
    while (lo <= hi)
    {
        int mid = (lo + hi) / 2;
        if (addr >= used_ranges[mid].end)
            lo = mid + 1;
        else if (addr < used_ranges[mid].start)
            hi = mid - 1;
        else
            return true;
    }
    return false;
}

static auto boot_mm_palloc(void) -> void *
{
    void *res;
    do
    {
        res = boot_mm_palloc_internal();
    } while (addr_in_used_range((mm::phys_addr_t)res));

    return res;
}

static auto boot_mm_pgtable_map(mm::phys_addr_t pgtable,
                                mm::virt_addr_t vaddr,
                                mm::phys_addr_t paddr,
                                mm::page_attr_t attr) -> int
{
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    int pgd_i = PGD_ENTRY(vaddr);
    int pud_i = PUD_ENTRY(vaddr);
    int pmd_i = PMD_ENTRY(vaddr);
    int pte_i = PTE_ENTRY(vaddr);

    pgd = (pgd_t *)pgtable;
    if (!pgd[pgd_i])
    {
        pgd[pgd_i] = (pgd_t)boot_mm_palloc();
        if (IS_ERR_PTR((void *)pgd[pgd_i]))
        {
            pgd[pgd_i] = (pgd_t) nullptr;
            return -ENOMEM;
        }

        boot_memset((void *)((mm::phys_addr_t)pgd[pgd_i]), 0, PAGE_SIZE);
        pgd[pgd_i] |= PDE_DEFAULT;
    }

    pud = (pud_t *)(pgd[pgd_i] & PAGE_MASK);
    if (!pud[pud_i])
    {
        pud[pud_i] = (pud_t)boot_mm_palloc();
        if (IS_ERR_PTR((void *)pud[pud_i]))
        {
            pud[pud_i] = (pud_t) nullptr;
            return -ENOMEM;
        }

        boot_memset((void *)((mm::phys_addr_t)pud[pud_i]), 0, PAGE_SIZE);
        pud[pud_i] |= PDE_DEFAULT;
    }

    pmd = (pmd_t *)(pud[pud_i] & PAGE_MASK);
    if (!pmd[pmd_i])
    {
        pmd[pmd_i] = (pmd_t)boot_mm_palloc();
        if (IS_ERR_PTR((void *)pmd[pmd_i]))
        {
            pmd[pmd_i] = (pmd_t) nullptr;
            return -ENOMEM;
        }

        boot_memset((void *)((mm::phys_addr_t)pmd[pmd_i]), 0, PAGE_SIZE);
        pmd[pmd_i] |= PDE_DEFAULT;
    }

    pte = (pte_t *)(pmd[pmd_i] & PAGE_MASK);
    pte[pte_i] = paddr | attr;

    return 0;
}

#define PMD_SIZE       (1UL << PMD_OFFSET)   /* 2 MB */
#define PMD_SIZE_MASK  (PMD_SIZE - 1)

/* Map a single 2 MB large page into the page table (PGD → PUD → PMD entry
 * with PS bit set).  Both vaddr and paddr must be 2 MB-aligned. */
static auto boot_mm_pgtable_map_2mb(mm::phys_addr_t pgtable,
                                    mm::virt_addr_t vaddr,
                                    mm::phys_addr_t paddr,
                                    mm::page_attr_t attr) -> int
{
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    int pgd_i = PGD_ENTRY(vaddr);
    int pud_i = PUD_ENTRY(vaddr);
    int pmd_i = PMD_ENTRY(vaddr);

    pgd = (pgd_t *)pgtable;
    if (!pgd[pgd_i])
    {
        pgd[pgd_i] = (pgd_t)boot_mm_palloc();
        if (IS_ERR_PTR((void *)pgd[pgd_i]))
        {
            pgd[pgd_i] = (pgd_t) nullptr;
            return -ENOMEM;
        }
        boot_memset((void *)((mm::phys_addr_t)pgd[pgd_i]), 0, PAGE_SIZE);
        pgd[pgd_i] |= PDE_DEFAULT;
    }

    pud = (pud_t *)(pgd[pgd_i] & PAGE_MASK);
    if (!pud[pud_i])
    {
        pud[pud_i] = (pud_t)boot_mm_palloc();
        if (IS_ERR_PTR((void *)pud[pud_i]))
        {
            pud[pud_i] = (pud_t) nullptr;
            return -ENOMEM;
        }
        boot_memset((void *)((mm::phys_addr_t)pud[pud_i]), 0, PAGE_SIZE);
        pud[pud_i] |= PDE_DEFAULT;
    }

    pmd = (pmd_t *)(pud[pud_i] & PAGE_MASK);
    /* Set PS bit to make this a 2 MB leaf entry. */
    pmd[pmd_i] = paddr | attr | PDE_ATTR_PS;

    return 0;
}

mm::phys_addr_t boot_kern_pgtable;

static auto boot_mm_load_pgtable(mm::phys_addr_t pgtable) -> void
{
    asm volatile(
        "mov %0, %%cr3" ::"a"(pgtable));
}

static auto boot_mm_pgtable_init(void) -> int
{
    Elf64_Shdr *shdr;
    void *shdr_end;
    mm::phys_addr_t seg_phys_start, seg_phys_end;
    mm::virt_addr_t seg_virt_start, seg_virt_end;
    mm::page_attr_t pte_attr;
    mm::phys_addr_t physmem_start, physmem_end;
    mm::Page *pgdb_base;
    base::size_t pgdb_page_nr;
    int ret;

    boot_kern_pgtable = (mm::phys_addr_t)boot_mm_palloc();
    if (IS_ERR_PTR((void *)boot_kern_pgtable))
    {
        boot_puts("[x] FAILED to allocate new page table!");
        return -ENOMEM;
    }
    boot_memset((void *)boot_kern_pgtable, 0, PAGE_SIZE);

    shdr_end = (void *)((mm::phys_addr_t)&elf_info_tag->sections + elf_info_tag->num * sizeof(*shdr));

    for (shdr = (Elf64_Shdr *)&elf_info_tag->sections; (void *)shdr < shdr_end; shdr = (Elf64_Shdr *)((mm::phys_addr_t)shdr + sizeof(*shdr)))
    {
        if (!(shdr->sh_flags & SHF_ALLOC))
            continue;

        if (shdr->sh_type & SHT_PROGBITS)
        {
            seg_phys_start = 0x100000 + shdr->sh_offset - PAGE_SIZE;
            seg_phys_end = PAGE_ALIGN(seg_phys_start + shdr->sh_size);
            seg_virt_start = shdr->sh_addr;

            while (seg_phys_start < seg_phys_end)
            {
                pte_attr = PTE_ATTR_P;
                if (shdr->sh_flags & SHF_WRITE)
                    pte_attr |= PTE_ATTR_RW;

                ret = boot_mm_pgtable_map(boot_kern_pgtable, seg_virt_start, seg_phys_start, pte_attr);
                if (ret < 0)
                    return ret;

                seg_phys_start += PAGE_SIZE;
                seg_virt_start += PAGE_SIZE;
            }
        }
        else if (shdr->sh_type & SHT_NOBITS)
        {
            seg_virt_start = shdr->sh_addr;
            seg_virt_end = PAGE_ALIGN(seg_virt_start + shdr->sh_size);

            while (seg_virt_start < seg_virt_end)
            {
                pte_attr = PTE_ATTR_P;
                if (shdr->sh_flags & SHF_WRITE)
                {
                    pte_attr |= PTE_ATTR_RW;
                }

                seg_phys_start = (mm::phys_addr_t)boot_mm_palloc();
                if (IS_ERR_PTR((void *)seg_phys_start))
                    return -ENOMEM;

                boot_memset((void *)seg_phys_start, 0, PAGE_SIZE);
                ret = boot_mm_pgtable_map(boot_kern_pgtable, seg_virt_start, seg_phys_start, pte_attr);
                if (ret < 0)
                    return ret;

                seg_virt_start += PAGE_SIZE;
            }
        }
        else
        {
            boot_puts("[x] Unknown segment. ELF mapping stopped.");
            return -EFAULT;
        }
    }

    if (boot_tty_has_fb())
    {
        seg_phys_start = ((mm::phys_addr_t)boot_fb_base) & PAGE_MASK;
        seg_virt_start = seg_phys_start;
        seg_phys_end = PAGE_ALIGN(seg_phys_start + boot_tty_fb_sz());
        while (seg_phys_start < seg_phys_end)
        {
            ret = boot_mm_pgtable_map(boot_kern_pgtable,
                                      seg_virt_start,
                                      seg_phys_start,
                                      PTE_ATTR_P | PTE_ATTR_RW);
            if (ret < 0)
                return ret;

            seg_phys_start += PAGE_SIZE;
            seg_virt_start += PAGE_SIZE;
        }
    }

    physmem_start = physmem_end = 0x0000000000000000;

    for (int i = 0; i < mmap_entry_nr; i++)
    {
        mm::phys_addr_t base = mmap_tag->entries[i].addr & PAGE_MASK;
        mm::phys_addr_t end = base + mmap_tag->entries[i].len;
        mm::virt_addr_t vaddr = base + mm::KERN_DIRECT_MAP_REGION_BASE;

        if (end > physmem_end)
            physmem_end = end;

        if (vaddr < base)
        {
            boot_puts("[x] FATAL: memory region base 0x");
            boot_puth(base);
            boot_puts(" has caused an integer overflow in memory mapping.");
            asm volatile(" hlt ");
        }

        while (base < end)
        {
            if (vaddr > mm::KERN_DIRECT_MAP_REGION_END)
                break;

            /* Use 2 MB large pages when both base and vaddr are 2 MB-aligned
             * and at least 2 MB of range remains.  This reduces the number of
             * page-table entries (and boot_mm_palloc calls) by a factor of 512
             * for the bulk of physical memory. */
            if (!(base & PMD_SIZE_MASK) && !(vaddr & PMD_SIZE_MASK) &&
                (end - base) >= PMD_SIZE &&
                (mm::KERN_DIRECT_MAP_REGION_END - vaddr + 1) >= PMD_SIZE)
            {
                ret = boot_mm_pgtable_map_2mb(boot_kern_pgtable, vaddr, base,
                                              PDE_ATTR_P | PDE_ATTR_RW);
                if (ret < 0)
                    return ret;
                base  += PMD_SIZE;
                vaddr += PMD_SIZE;
            }
            else
            {
                ret = boot_mm_pgtable_map(boot_kern_pgtable, vaddr, base,
                                          PTE_ATTR_P | PTE_ATTR_RW);
                if (ret < 0)
                    return ret;
                base  += PAGE_SIZE;
                vaddr += PAGE_SIZE;
            }
        }
    }

    pgdb_base = (mm::Page *)mm::KERN_PAGE_DATABASE_REGION_BASE;
    pgdb_page_nr = (physmem_end - physmem_start) / PAGE_SIZE;

    for (base::size_t i = 0; i < pgdb_page_nr; i += mm::PGDB_PG_PAGE_NR)
    {
        void *new_page = boot_mm_palloc();
        if (IS_ERR_PTR(new_page))
            return PTR_ERR(new_page);

        boot_memset(new_page, 0, PAGE_SIZE);
        ret = boot_mm_pgtable_map(boot_kern_pgtable, (mm::virt_addr_t)&pgdb_base[i], (mm::phys_addr_t)new_page, PTE_ATTR_P | PTE_ATTR_RW);
        if (ret < 0)
            return ret;
    }

    boot_mm_load_pgtable(boot_kern_pgtable);

    mm::physmem_base = mm::KERN_DIRECT_MAP_REGION_BASE;
    mm::kernel_base = mm::KERN_SEG_TEXT_REGION_START;
    mm::vmremap_base = mm::KERN_DYNAMIC_MAP_REGION_BASE;
    mm::pgdb_base = pgdb_base;
    mm::pgdb_page_nr = pgdb_page_nr;

    elf_info_tag = (multiboot_tag_elf_sections *)mm::phys_to_virt((mm::phys_addr_t)elf_info_tag);
    mmap_tag = (multiboot_tag_mmap *)mm::phys_to_virt((mm::phys_addr_t)mmap_tag);

    return 0;
}

static auto boot_mm_page_database_init(void) -> int
{
    int ret;

    for (int i = 0; i < mmap_entry_nr; i++)
    {
        mm::phys_addr_t base = mmap_tag->entries[i].addr;
        mm::phys_addr_t end = base + mmap_tag->entries[i].len;
        base::size_t pfn;

        while (base < end)
        {
            pfn = base / PAGE_SIZE;
            mm::pgdb_base[pfn].migrate_type = mm::MIGRATE_UNMOVABLE;
            mm::pgdb_base[pfn].lock.Reset();
            mm::pgdb_base[pfn].kc = nullptr;
            mm::pgdb_base[pfn].pool = nullptr;
            mm::pgdb_base[pfn].freelist = nullptr;
            mm::pgdb_base[pfn].obj_nr = 0;
            lib::list_head_init(&mm::pgdb_base[pfn].list);

            {
                auto ptype = mm::PAGE_UNKNOWN;
                int pref = 0;

                switch (mmap_tag->entries[i].type)
                {
                case MULTIBOOT_MEMORY_AVAILABLE:
                    ptype = mm::PAGE_NORMAL_MEM;
                    pref = (base < curr_avail || addr_in_used_range(base)) ? 0 : -1;
                    break;
                case MULTIBOOT_MEMORY_RESERVED:
                    ptype = mm::PAGE_RESERVED;
                    break;
                case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                    ptype = mm::PAGE_ACPI_RECLAIMABLE;
                    break;
                case MULTIBOOT_MEMORY_NVS:
                    ptype = mm::PAGE_NVS;
                    break;
                case MULTIBOOT_MEMORY_BADRAM:
                    ptype = mm::PAGE_BADRAM;
                    break;
                default:
                    ptype = mm::PAGE_UNKNOWN;
                    break;
                }

                mm::pgdb_base[pfn].type = ptype;
                lib::atomic::atomic_set(&mm::pgdb_base[pfn].ref_count, pref);
            }
        
            base += PAGE_SIZE;
        }
    }
    
    return 0;
}

auto boot_mm_init(multiboot_uint8_t *mbi) -> int
{
    struct multiboot_tag *tag;
    static struct multiboot_mmap_entry *mmap_entry;
    int ret;

    for (tag = (struct multiboot_tag *)(mbi + 8);
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag + ((tag->size + 7) & ~7)))
    {
        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP)
        {
            mmap_tag = (struct multiboot_tag_mmap *)tag;
        }
        else if (tag->type == MULTIBOOT_TAG_TYPE_ELF_SECTIONS)
        {
            elf_info_tag = (struct multiboot_tag_elf_sections *)tag;
        }
    }

    if (!mmap_tag)
    {
        boot_puts("[x] FAILED to find MMAP tag in multiboot info.");
        return -1;
    }
    else if (!elf_info_tag)
    {
        boot_puts("[x] FAILED to find ELF tag in multiboot info.");
        return -1;
    }

    mmap_entry_nr = (mmap_tag->size - sizeof(*mmap_tag)) / sizeof(multiboot_mmap_entry);

    multiboot_tag_start = (mm::phys_addr_t)mbi & PAGE_MASK;

    for (tag = (struct multiboot_tag *)(mbi + 8);
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag + ((tag->size + 7) & ~7)))
        ;

    multiboot_tag_end = PAGE_ALIGN((page_attr_t)tag);

    /* Build sorted used-range cache once; all subsequent addr_in_used_range()
     * calls during page-table setup and page-database init use O(log N)
     * binary search instead of scanning ELF sections on every call. */
    boot_mm_build_used_ranges();

    if ((ret = boot_mm_pgtable_init()) < 0)
    {
        boot_puts("[x] FAILED to initialize page table, errno: ");
        boot_puti(ret);
        boot_putchar('\n');
    }

    if ((ret = boot_mm_page_database_init()) < 0)
    {
        boot_puts("[x] FAILED to initialize page database, errno: ");
        boot_puti(ret);
        boot_putchar('\n');
    }

    auto val = mm::KERN_DIRECT_MAP_REGION_BASE;

    return val;
}