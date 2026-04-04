#ifndef KERNEL_MM_IOREMAP_HPP
#define KERNEL_MM_IOREMAP_HPP

#include <kernel/mm/types.hpp>
#include <kernel/base.hpp>
#include <kernel/lib.rbtree.hpp>
#include <kernel/lib.hpp>

namespace mm
{
    /*
     * ioremap - Map a physical I/O address range into kernel virtual space.
     *
     * @paddr : Physical base address (need not be page-aligned; any sub-page
     *          offset is preserved in the returned pointer).
     * @size  : Number of bytes to map (must be > 0).
     *
     * Returns the kernel virtual address corresponding to paddr, or nullptr
     * on failure.  The mapping uses cache-disabled (PCD+PWT) attributes,
     * which is appropriate for memory-mapped I/O registers.
     *
     * The mapping persists until iounmap() is called on the returned address.
     */
    auto ioremap(phys_addr_t paddr, base::size_t size) -> virt_addr_t;

    /*
     * iounmap - Release a mapping previously created by ioremap().
     *
     * @vaddr : The virtual address returned by ioremap().
     *
     * Returns 0 on success, -1 if vaddr was not found in the mapping table.
     */
    auto iounmap(virt_addr_t vaddr) -> int;

    /*
     * ioremap_init - Initialise the ioremap subsystem.
     *
     * Must be called after mm_core_init() completes.  Sets up the virtual
     * address allocator for the dynamic-map region.
     */
    auto ioremap_init(void) -> void;

    /* ------------------------------------------------------------------ */
    /* Internal data structures (exposed for unit-testing / diagnostics)  */
    /* ------------------------------------------------------------------ */

    /*
     * IoMapEntry - One active ioremap mapping.
     *
     * Embedded in the IoMapTree red-black tree, keyed by vaddr.
     */
    struct IoMapEntry
    {
        virt_addr_t vaddr;       /* page-aligned virtual base              */
        phys_addr_t paddr;       /* page-aligned physical base             */
        base::size_t page_count; /* number of pages mapped                 */
        base::size_t orig_offset;/* byte offset within the first page      */
        lib::RbNode  rb_node;    /* intrusive RB-tree node                 */
    };

    /*
     * VmaRegion - One free region in the dynamic-map virtual address space.
     *
     * Kept on a singly linked list sorted by ascending base address.
     */
    struct VmaRegion
    {
        virt_addr_t  base;
        base::size_t page_count;
        lib::ListHead list;
    };

} /* namespace mm */

#endif /* !KERNEL_MM_IOREMAP_HPP */
