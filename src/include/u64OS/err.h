#ifndef u64OS_ERR_H
#define u64OS_ERR_H

#include <stdtypes.h>
#include <u64OS/errno.h>
#include <u64OS/compiler.h>

static __always_inline bool IS_ERR_PTR(const void *ptr)
{
    return (size_t)ptr > (size_t)-MAX_ERRNO;
}

static __always_inline void *ERR_PTR(ssize_t errno)
{
    return (void *)errno;
}

static __always_inline ssize_t PTR_ERR(void *ptr)
{
    return (ssize_t)ptr;
}

#endif // !u64OS_ERR_H