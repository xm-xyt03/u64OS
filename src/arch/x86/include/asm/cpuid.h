#include <stdtypes.h>
#include <u64OS/compiler.h>

#ifndef CPUID_FEAT_EDX_APIC
#define CPUID_FEAT_EDX_APIC (1 << 9)
#endif

struct CpuidResult
{
    uint32_t eax, ebx, ecx, edx;
};

static __always_inline CpuidResult cpuid(uint32_t eax)
{
    CpuidResult ret;
    asm volatile("cpuid"
                 : "=a"(ret.eax), "=b"(ret.ebx), "=c"(ret.ecx), "=d"(ret.edx)
                 : "a"(eax));
    return ret;
}