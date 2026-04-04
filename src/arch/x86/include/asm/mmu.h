#ifndef X86_ASM_MMU_H
#define X86_ASM_MMU_H

#ifndef ASM_FILE

#include <stdtypes.h>

/* Flush a single TLB entry for the given virtual address (invlpg) */
static inline void flush_tlb_single(uint64_t vaddr)
{
    asm volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

/* Read the current CR3 value (physical address of PGD) */
static inline uint64_t read_cr3(void)
{
    uint64_t val;
    asm volatile("mov %%cr3, %0" : "=r"(val));
    return val;
}

/* Load a new PGD physical address into CR3 */
static inline void load_cr3(uint64_t pgd_phys)
{
    asm volatile("mov %0, %%cr3" : : "r"(pgd_phys) : "memory");
}

/* Flush the entire TLB (by reloading CR3) */
static inline void flush_tlb_all(void)
{
    load_cr3(read_cr3());
}

#endif /* !ASM_FILE */
#endif /* !X86_ASM_MMU_H */
