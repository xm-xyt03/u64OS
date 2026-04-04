#include <config.h>
#include <boot/multiboot2.h>
#include <kernel/mm.hpp>
#include <kernel/init.hpp>
#ifdef KTEST_ENABLE
#include <kernel/test.hpp>
#endif
#include <u64OS/cpp_base.hpp>

extern "C"
{
#include <boot/tty.h>
}

auto main(multiboot_uint8_t *mbi) -> void
{
    boot_puts("This is the kernel!\n");
    init::kernel_init(mbi);

#ifdef KTEST_ENABLE
    auto ktest_result = ktest::run_all();
    if (ktest_result.failed > 0)
        boot_puts("[KTEST] WARNING: some tests FAILED!\n");
    else
        boot_puts("[KTEST] All tests passed.\n");
#endif

    while (1)
    {
        boot_puts("[x] No work to do. Halting...\n");
        asm volatile("hlt");
    }
}