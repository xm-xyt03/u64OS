#include <kernel/test.hpp>
#include <kernel/mm.hpp>

// NOTE: These tests assume mm_core_init() has already been called in main()
//       before ktest::run_all() is invoked.

// ── Alignment helper ──────────────────────────────────────────────────────────

static auto is_aligned(void *ptr, base::size_t align) -> bool
{
    return ((reinterpret_cast<mm::virt_addr_t>(ptr)) & (align - 1)) == 0;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

KTEST(Heap, SmallAllocNonNull)
{
    void *p = mm::GloblKHeapPool->Malloc(16);
    EXPECT_NOT_NULL(p);
    mm::GloblKHeapPool->Free(p);
}

KTEST(Heap, AllocIsAligned16)
{
    void *p = mm::GloblKHeapPool->Malloc(16);
    EXPECT_NOT_NULL(p);
    EXPECT_TRUE(is_aligned(p, 16));
    mm::GloblKHeapPool->Free(p);
}

KTEST(Heap, SizeClass32)
{
    void *p = mm::GloblKHeapPool->Malloc(32);
    EXPECT_NOT_NULL(p);
    mm::GloblKHeapPool->Free(p);
}

KTEST(Heap, SizeClass64)
{
    void *p = mm::GloblKHeapPool->Malloc(64);
    EXPECT_NOT_NULL(p);
    mm::GloblKHeapPool->Free(p);
}

KTEST(Heap, SizeClass256)
{
    void *p = mm::GloblKHeapPool->Malloc(256);
    EXPECT_NOT_NULL(p);
    mm::GloblKHeapPool->Free(p);
}

KTEST(Heap, SizeClass1K)
{
    void *p = mm::GloblKHeapPool->Malloc(1024);
    EXPECT_NOT_NULL(p);
    mm::GloblKHeapPool->Free(p);
}

KTEST(Heap, SizeClass8K)
{
    void *p = mm::GloblKHeapPool->Malloc(8192);
    EXPECT_NOT_NULL(p);
    mm::GloblKHeapPool->Free(p);
}

KTEST(Heap, LargeAllocPagePath)
{
    // Requests larger than 8 KB should fall through to the page allocator
    void *p = mm::GloblKHeapPool->Malloc(16384);
    EXPECT_NOT_NULL(p);
    mm::GloblKHeapPool->Free(p);
}

KTEST(Heap, AllocFreeLoop)
{
    // Repeated alloc/free must not exhaust the pool or crash
    for (int i = 0; i < 32; i++)
    {
        void *p = mm::GloblKHeapPool->Malloc(64);
        EXPECT_NOT_NULL(p);
        mm::GloblKHeapPool->Free(p);
    }
}

KTEST(Heap, MultipleSimultaneous)
{
    // Hold several allocations at once then free them all
    void *ptrs[8];
    for (int i = 0; i < 8; i++)
    {
        ptrs[i] = mm::GloblKHeapPool->Malloc(128);
        EXPECT_NOT_NULL(ptrs[i]);
    }

    // All pointers must be distinct
    for (int i = 0; i < 8; i++)
    {
        for (int j = i + 1; j < 8; j++)
        {
            EXPECT_NE(ptrs[i], ptrs[j]);
        }
    }

    for (int i = 0; i < 8; i++)
        mm::GloblKHeapPool->Free(ptrs[i]);
}

KTEST(PagePool, AllocOrder0)
{
    mm::Page *p = mm::GloblPagePool->AllocPages(0);
    EXPECT_NOT_NULL(p);
    mm::GloblPagePool->FreePages(p, 0);
}

KTEST(PagePool, AllocOrder1)
{
    mm::Page *p = mm::GloblPagePool->AllocPages(1);
    EXPECT_NOT_NULL(p);
    mm::GloblPagePool->FreePages(p, 1);
}

KTEST(PagePool, VirtAddrInHighRange)
{
    // All kernel pages must be mapped in the high-canonical region
    mm::Page *p = mm::GloblPagePool->AllocPages(0);
    EXPECT_NOT_NULL(p);

    mm::virt_addr_t va = mm::page_to_virt(p);
    // High canonical on x86-64: top bit set (>= 0xffff800000000000)
    EXPECT_TRUE(va >= 0xffff800000000000UL);

    mm::GloblPagePool->FreePages(p, 0);
}
