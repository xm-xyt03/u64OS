#include <stdtypes.h>
#include <u64OS/compiler.h>

#ifndef IA32_APIC_BASE_MSR
#define IA32_APIC_BASE_MSR 0x1B
#endif

#ifndef IA32_APIC_BASE_MSR_ENABLE
#define IA32_APIC_BASE_MSR_ENABLE 0x800
#endif

static __always_inline uint64_t rdmsr(uint32_t msr)
{
    uint32_t ret_hi;
    uint32_t ret_lo;
    asm volatile("rdmsr" : "=a"(ret_lo), "=d"(ret_hi) : "c"(msr));
    return ((uint64_t)ret_hi << 32) | ret_lo;
}

static __always_inline void wrmsr(uint32_t msr, uint64_t data)
{
    uint32_t data_hi = (data >> 32) & 0xFFFFFFFF;
    uint32_t data_lo = data & 0xFFFFFFFF;
    asm volatile("wrmsr" ::"a"(data_lo), "d"(data_hi), "c"(msr));
}