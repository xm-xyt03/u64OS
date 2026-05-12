#ifndef u64OS_CPP_BASE_HPP
#define u64OS_CPP_BASE_HPP

#include <stdtypes.h>

extern void *operator new(size_t sz);
extern void *operator new[](size_t sz);
extern void operator delete(void *p) noexcept;
extern void operator delete[](void *p) noexcept;
extern void operator delete(void *p, size_t sz) noexcept;
extern void operator delete[](void *p, size_t sz) noexcept;

struct dtor_info
{
    struct dtor_info *next;
    void (*destructor)(void *);
    void *arg;
    void *__dso_handle;
};

extern "C"
{
    extern void (*__init_array)(void);
    extern void (*__init_array_end)(void);
    extern int __cxa_atexit(void (*f)(void *), void *p, void *d);
    extern void __cxa_finalize(void *dso_handle);
    extern void __cxa_pure_virtual();
    extern void *__dso_handle __attribute__((visibility("hidden")));
}

#endif // !u64OS_CPP_BASE_HPP