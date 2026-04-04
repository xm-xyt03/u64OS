extern "C"
{

#include <boot/multiboot2.h>
#include <boot/tty.h>
#include <asm/page_types.h>
#include <asm/io.h>
#include <boot/tty.h>

    extern uint64_t boot_pud[512];
}

static auto boot_init_pgtable() -> void
{
    for (size_t i = 1; i < 512; i++)
    {
        boot_pud[i] = (i * 0x40000000) | (PDE_ATTR_P | PDE_ATTR_RW | PDE_ATTR_PS);
    }
}

extern auto boot_mm_init(multiboot_uint8_t *mbi) -> int;
extern auto main(multiboot_uint8_t *mbi) -> void;

extern "C" void boot_main(unsigned int magic, multiboot_uint8_t *mbi)
{
    int ret;

    boot_init_pgtable();
    if (boot_tty_init(mbi) < 0)
        asm volatile("hlt");

    boot_puts("[+] Boot-state tty initialization done.\n");

    if ((ret = boot_mm_init(mbi)) < 0)
    {
        boot_puts("[x] FAILED to initialize memory unit, errno: ");
        boot_puti(ret);
        boot_puts("\nAbort.");
        asm volatile("hlt");
    }

    boot_puts("[+] Boot-state memory initialization done.\n");

    main(mbi);
}