extern "C"
{
#include <asm/io.h>
}

#include <kernel/base.hpp>

/* 8259A PIC I/O port addresses */
inline constexpr base::uint16_t PIC_MASTER_CMD  = 0x20;
inline constexpr base::uint16_t PIC_MASTER_DATA = 0x21;
inline constexpr base::uint16_t PIC_SLAVE_CMD   = 0xA0;
inline constexpr base::uint16_t PIC_SLAVE_DATA  = 0xA1;

/* ICW1: initialise + ICW4 required */
inline constexpr base::uint8_t PIC_ICW1_INIT    = 0x11;
/* ICW4: 8086 mode */
inline constexpr base::uint8_t PIC_ICW4_8086    = 0x01;

/* EOI command */
inline constexpr base::uint8_t PIC_EOI          = 0x20;

/* IRQ base vectors after remapping */
inline constexpr base::uint8_t PIC_MASTER_OFFSET = 0x20; /* IRQ 0–7  → vectors 32–39 */
inline constexpr base::uint8_t PIC_SLAVE_OFFSET  = 0x28; /* IRQ 8–15 → vectors 40–47 */

auto pic_init(void) -> void
{
    /* ICW1: start initialisation sequence for both PICs */
    outb(PIC_MASTER_CMD,  PIC_ICW1_INIT);
    outb(PIC_SLAVE_CMD,   PIC_ICW1_INIT);

    /* ICW2: set interrupt vector offsets */
    outb(PIC_MASTER_DATA, PIC_MASTER_OFFSET);
    outb(PIC_SLAVE_DATA,  PIC_SLAVE_OFFSET);

    /* ICW3: master has slave on IRQ2; slave ID is 2 */
    outb(PIC_MASTER_DATA, 0x04);
    outb(PIC_SLAVE_DATA,  0x02);

    /* ICW4: 8086 mode for both */
    outb(PIC_MASTER_DATA, PIC_ICW4_8086);
    outb(PIC_SLAVE_DATA,  PIC_ICW4_8086);

    /* OCW1: mask all IRQs on both PICs */
    outb(PIC_MASTER_DATA, 0xFF);
    outb(PIC_SLAVE_DATA,  0xFF);
}

auto pic_eoi() -> void
{
    /* Always send EOI to master; also send to slave for IRQs 8–15 */
    outb(PIC_SLAVE_CMD,  PIC_EOI);
    outb(PIC_MASTER_CMD, PIC_EOI);
}
