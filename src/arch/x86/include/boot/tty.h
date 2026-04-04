#ifndef X86_ASM_TTY_H
#define X86_ASM_TTY_H

#include <stdtypes.h>
#include <boot/multiboot2.h>

extern uint32_t *boot_fb_base, *boot_fb_end;
extern uint32_t boot_fb_width, boot_fb_height;

extern bool boot_tty_has_fb(void);
extern bool boot_tty_has_com(void);

extern size_t boot_tty_fb_sz(void);

extern void boot_puth(uint64_t);
extern void boot_puti(int64_t);
extern void boot_putchar(uint8_t);
extern void boot_puts(const char *);

extern int boot_tty_init(multiboot_uint8_t *mbi);

#endif // !X86_ASM_TTY_H