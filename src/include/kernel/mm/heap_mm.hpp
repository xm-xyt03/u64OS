#ifndef KERNEL_MM_HEAP_MM_HPP
#define KERNEL_MM_HEAP_MM_HPP

#include <kernel/base.hpp>
#include <kernel/mm/page_mm.hpp>
#include <kernel/lib.hpp>

namespace mm
{
    enum
    {
        KOBJECT_16,
        KOBJECT_32,
        KOBJECT_64,
        KOBJECT_128,
        KOBJECT_192,
        KOBJECT_256,
        KOBJECT_512,
        KOBJECT_1K,
        KOBJECT_2K,
        KOBJECT_4K,
        KOBJECT_8K,
        KOBJECT_SIZE_NR,
    };

    extern base::size_t kobj_default_size[KOBJECT_SIZE_NR];

    inline constexpr base::size_t CACHE_POOL_MAX_NR = 0x10;

    class KMemCache
    {
    public:
        KMemCache();
        ~KMemCache();

        auto Malloc(void) -> void *;
        auto Free(Page *page, void *obj) -> void;

        auto AddPool(PagePool *pool) -> bool;
        auto RemovePool(base::size_t index) -> PagePool *;

        auto Init(base::size_t obj_sz) -> void;

    private:
        PagePool *pools[CACHE_POOL_MAX_NR];
        base::size_t pool_nr;
        base::size_t order;

        auto __internal_page_alloc(void) -> Page *;
        auto __page_obj_slicing(Page *page) -> void;

        base::size_t page_obj_nr;
        base::size_t obj_sz;
        Page *current;
        lib::ListHead partial;
        lib::ListHead full;
        void **freelist;

        auto __internal_obj_alloc(void) -> void *;
        auto __internal_obj_free(Page *page, void *obj) -> void;

        lib::atomic::SpinLock lock;
    };

    extern base::uint8_t GloblKMemCacheGroupMem[KOBJECT_SIZE_NR][sizeof(KMemCache)];
    extern KMemCache *GloblKMemCacheGroup[KOBJECT_SIZE_NR];

    class KHeapPool
    {
    public:
        KHeapPool(void);
        ~KHeapPool();

        auto Malloc(base::size_t size) -> void *;
        auto Free(void *obj) -> void;

        auto PageAlloc(base::size_t order) -> Page *;
        auto PageFree(Page *p) -> void;

        auto Init(void) -> void;
        auto SetKMemCaches(KMemCache **caches, base::size_t cache_nr, base::size_t *cache_obj_sizes) -> void;
        auto SetPagePools(PagePool **pools, base::size_t pool_nr) -> void;

    private:
        KMemCache **caches;
        base::size_t cache_nr;
        base::size_t *cache_obj_sizes;

        auto __malloc_caches(base::size_t size) -> void *;

        PagePool **pools;
        base::size_t pool_nr;

        auto __malloc_pools(base::size_t size) -> void *;
    };

    extern KHeapPool *GloblKHeapPool;
}

#endif // !KERNEL_MM_HEAP_MM_HPP