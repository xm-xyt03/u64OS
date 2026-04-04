#ifndef X86_BOOT_STRING_H
#define X86_BOOT_STRING_H

#include <stdtypes.h>

void *boot_memset(void *dst, uint8_t val, uint64_t sz)
{
    uint8_t *__dst = (uint8_t *)dst;

    for (uint64_t i = 0; i < sz; i++)
    {
        __dst[i] = val;
    }

    return dst;
}

#endif // X86_BOOT_STRING_H