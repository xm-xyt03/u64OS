#include <kernel/lib.atomic.hpp>

namespace lib::atomic
{
#define SPINLOCK_LOCKED 1
#define SPINLOCK_FREE 0

    SpinLock::SpinLock()
    {
        this->Reset();
    }

    SpinLock::~SpinLock() {}

    auto SpinLock::Lock(void) -> void
    {
        while (!this->TryLock())
            ;
    }

    auto SpinLock::TryLock(void) -> bool
    {
        return atomic_compare_and_swap(&this->counter, SPINLOCK_FREE, SPINLOCK_LOCKED);
    }

    auto SpinLock::UnLock(void) -> void
    {
        atomic_set(&this->counter, SPINLOCK_FREE);
    }

    auto SpinLock::Reset(void) -> void
    {
        this->UnLock();
    }
}