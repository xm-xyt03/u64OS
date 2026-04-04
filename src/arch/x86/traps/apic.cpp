extern "C"
{
#include <asm/msr.h>
#include <asm/cpuid.h>
}

#include <kernel/mm.hpp>
#include <kernel/base.hpp>

inline constexpr mm::phys_addr_t APIC_PHYS_BASE_ADDR = 0xFEE00000;
inline constexpr base::size_t LAPIC_MAP_SIZE = 0x1000; /* 4 KB */

/* LAPIC register indices (each register is at index * 0x10 bytes, 128-bit aligned,
   accessed as uint32_t[4] where [0] is the actual register) */
enum
{
    APIC_APICID = 0x02,
    APIC_APICVER = 0x03,
    APIC_TPR = 0x08,
    APIC_APR = 0x09,
    APIC_PPR = 0x0A,
    APIC_EOI = 0x0B,
    APIC_RRD = 0x0C,
    APIC_LDR = 0x0D,
    APIC_DFR = 0x0E,
    APIC_SIV = 0x0F,
    APIC_ISR = 0x10,
    APIC_TMR = 0x18,
    APIC_IRR = 0x20,
    APIC_ESR = 0x28,
    APIC_CMCI = 0x2F,
    APIC_ICRL = 0x30,
    APIC_ICRH = 0x31,
    APIC_LVT0 = 0x32,
    APIC_LVT1 = 0x33,
    APIC_LVT2 = 0x34,
    APIC_LVT3 = 0x35,
    APIC_LVT4 = 0x36,
    APIC_LVTE = 0x37,
    APIC_ICR = 0x38,
    APIC_CCR = 0x39,
    APIC_DCR = 0x3E
};

enum
{
    APIC_SOFTWARE_ENABLE = 0x100,
    APIC_SPURIOUS_VECTOR = 0xFF,
    APIC_LVT_NMI = (1 << 10),
    APIC_LVT_CLR = (1 << 16)
};

/* LAPIC MMIO register file pointer, set by apic_init() */
static base::uint32_t (*APIC_REGISTERS)[4];

/* Virtual address of the mapped LAPIC MMIO region */
static mm::virt_addr_t lapic_vbase = 0;

/* Tracks whether the LAPIC is in use; read by traps.cpp via apic_enabled() */
static bool apic_active = false;

static auto set_apic_address(mm::phys_addr_t addr) -> void
{
    wrmsr(IA32_APIC_BASE_MSR, addr);
}

auto apic_check() -> bool
{
    auto cpui = cpuid(1);
    return cpui.edx & CPUID_FEAT_EDX_APIC;
}

auto apic_init() -> void
{
    /* Map LAPIC MMIO into the kernel I/O remap region */
    lapic_vbase = mm::ioremap(APIC_PHYS_BASE_ADDR, LAPIC_MAP_SIZE);
    if (!lapic_vbase)
        return;

    /* Relocate IA32_APIC_BASE_MSR to the same physical address (no-op in practice) */
    set_apic_address(APIC_PHYS_BASE_ADDR);
    APIC_REGISTERS = (base::uint32_t (*)[4])lapic_vbase;

    /* Configure task priority, destination format, and logical destination */
    APIC_REGISTERS[APIC_TPR][0] = 0;
    APIC_REGISTERS[APIC_DFR][0] = 0xFFFFFFFF;
    APIC_REGISTERS[APIC_LDR][0] = (APIC_REGISTERS[APIC_LDR][0] & 0x00FFFFFF) | 1;

    /* Mask all local vector table entries; LVT2 (LINT1) carries NMI */
    APIC_REGISTERS[APIC_LVT0][0] = APIC_LVT_CLR;
    APIC_REGISTERS[APIC_LVT1][0] = APIC_LVT_CLR;
    APIC_REGISTERS[APIC_LVT2][0] = APIC_LVT_NMI;
    APIC_REGISTERS[APIC_LVT3][0] = APIC_LVT_CLR;
    APIC_REGISTERS[APIC_LVT4][0] = APIC_LVT_CLR;

    /* Enable the APIC via the MSR global enable bit */
    wrmsr(IA32_APIC_BASE_MSR, rdmsr(IA32_APIC_BASE_MSR) | IA32_APIC_BASE_MSR_ENABLE);

    /* Software-enable the APIC and set the spurious interrupt vector */
    APIC_REGISTERS[APIC_SIV][0] = APIC_SPURIOUS_VECTOR | APIC_SOFTWARE_ENABLE;

    apic_active = true;
}

auto apic_enabled() -> bool
{
    return apic_active;
}

auto apic_eoi() -> void
{
    /* Writing 0 to the EOI register signals end-of-interrupt to the LAPIC */
    APIC_REGISTERS[APIC_EOI][0] = 0;
}
