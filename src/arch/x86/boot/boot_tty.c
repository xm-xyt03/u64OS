#include <stdtypes.h>
#include <asm/io.h>
#include <asm/com.h>
#include <boot/multiboot2.h>
#include <graphics/tty/default.h>
#include <graphics/tty/font/psf.h>

static enum {
    NO_TTY,
    TTY_FB,
    TTY_COM,
    TTY_ALL,
} tty_type = NO_TTY;

extern char _binary_font_psf_start[];
extern char _binary_font_psf_end[];

static uint32_t fb_cursor_x, fb_cursor_y;
static uint32_t max_ch_nr_x, max_ch_nr_y;

static struct psf2_header *boot_font;
static uint32_t font_width, font_height;
static uint8_t *glyph_table;
static uint32_t bytes_per_glyph, glyph_nr;

uint32_t *boot_fb_base, *boot_fb_end;
uint32_t boot_fb_width, boot_fb_height;
size_t boot_fb_sz;

static size_t com_base = 0;

static void itoa(uint64_t value, char **buf_ptr_addr, uint8_t base)
{
    uint8_t m = value % base;
    uint32_t i = value / base;
    if (i)
        itoa(i, buf_ptr_addr, base);

    if (m < 10)
        *((*buf_ptr_addr)++) = m + '0';
    else
        *((*buf_ptr_addr)++) = m - 10 + 'A';
}

static void boot_clear_screen_fb(void)
{
    for (uint32_t y = 0; y < boot_fb_height; y++)
    {
        for (uint32_t x = 0; x < boot_fb_width; x++)
        {
            boot_fb_base[y * boot_fb_width + x] = 0;
        }
    }
}

static void boot_cursor_back_fb(void)
{
    fb_cursor_x = 0;
}

static int boot_get_frame_buffer(multiboot_uint8_t *mbi)
{
    struct multiboot_tag_framebuffer *fb_info = 0;
    struct multiboot_tag *tag = (struct multiboot_tag *)(mbi + 8);

    if (tag == NULL)
        return -1;

    while (tag->type != MULTIBOOT_TAG_TYPE_END)
    {
        if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER)
        {
            fb_info = (struct multiboot_tag_framebuffer *)tag;
            break;
        }

        tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag + ((tag->size + 7) & ~7));
    }

    if (fb_info == NULL)
        return -1;

    boot_fb_base = (uint32_t *)fb_info->common.framebuffer_addr;
    boot_fb_height = fb_info->common.framebuffer_height;
    boot_fb_width = fb_info->common.framebuffer_width;
    boot_fb_sz = boot_fb_width * boot_fb_height * sizeof(uint32_t);
    boot_fb_end = (uint32_t *)((size_t)boot_fb_base + boot_fb_sz);

    return 0;
}

static void boot_putchar_new_line_fb(uint32_t bg)
{
    fb_cursor_x = 0;
    fb_cursor_y++;

    if (fb_cursor_y >= max_ch_nr_y)
    {
        for (uint32_t y = 0; y < ((max_ch_nr_y - 1) * font_height); y++)
        {
            for (uint32_t x = 0; x < boot_fb_width; x++)
            {
                boot_fb_base[x + y * boot_fb_width] = boot_fb_base[x + (y + font_height) * boot_fb_width];
            }
        }

        for (uint32_t y = 0; y < font_height; y++)
        {
            for (uint32_t x = 0; x < boot_fb_width; x++)
            {
                size_t lines = y + (max_ch_nr_y - 1) * font_height;
                size_t loc = lines * boot_fb_width + x;
                boot_fb_base[loc] = bg;
            }
        }

        fb_cursor_y--;
    }
}

static void boot_putchar_fb(uint8_t ch, uint32_t fg, uint32_t bg)
{
    uint8_t *glyph = glyph_table;
    size_t loc;

    if (ch < glyph_nr)
        glyph += ch * bytes_per_glyph;

    loc = fb_cursor_y * font_height * boot_fb_width;
    loc += fb_cursor_x * font_width;

    for (uint32_t ch_y = 0; ch_y < font_height; ch_y++)
    {
        uint8_t mask = 1 << 7;
        for (uint32_t ch_x = 0; ch_x < font_width; ch_x++)
        {
            if ((*(glyph + ch_x / 8) & mask) != 0)
                boot_fb_base[loc + ch_y * boot_fb_width + ch_x] = fg;
            else
                boot_fb_base[loc + ch_y * boot_fb_width + ch_x] = bg;

            mask >>= 1;
            if (ch_x % 8 == 0)
                mask = 1 << 7;
        }

        glyph += font_width / 8;
    }

    fb_cursor_x++;

    if (fb_cursor_x >= max_ch_nr_x)
        boot_putchar_new_line_fb(bg);
}

static void boot_putchar_tab_fb(void)
{
    int left_ch_nr, space_nr;

    left_ch_nr = max_ch_nr_x - fb_cursor_x - 1;
    space_nr = (left_ch_nr > DEFAULT_TAB_SIZE) ? DEFAULT_TAB_SIZE : left_ch_nr;

    for (int i = 0; i < space_nr; i++)
    {
        boot_putchar_fb(' ', 0xffffff, 0);
    }
}

static void boot_back_space_fb(void)
{
    if (fb_cursor_x > 0)
    {
        fb_cursor_x--;
    }
}

static void boot_init_font(void)
{
    boot_font = (struct psf2_header *)_binary_font_psf_start;

    font_width = ((boot_font->width + 7) / 8) * 8;
    font_height = boot_font->height;

    glyph_table = (uint8_t *)_binary_font_psf_start + boot_font->header_size;
    glyph_nr = boot_font->glyph_nr;
    bytes_per_glyph = boot_font->bytes_per_glyph;

    fb_cursor_x = fb_cursor_y = 0;
    max_ch_nr_x = boot_fb_width / font_width;
    max_ch_nr_y = boot_fb_height / font_height;
}

size_t boot_tty_fb_sz(void)
{
    return boot_fb_sz;
}

static int boot_init_com_internal(size_t base_port)
{
    outb(base_port + COM_REG_IER, 0x00);

    outb(base_port + COM_REG_LCR, 0x80);

    outb(base_port + COM_REG_DLL, 0x03);
    outb(base_port + COM_REG_DLM, 0x00);

    outb(base_port + COM_REG_LCR, 0x03);

    outb(base_port + COM_REG_FCR, 0xc7);

    outb(base_port + COM_REG_MCR, 0x0b);

    outb(base_port + COM_REG_MCR, 0x1e);

    outb(base_port + COM_REG_TX, "u64OS"[0]);
    if (inb(base_port + COM_REG_RX) != "u64OS"[0])
        return -1;

    outb(base_port + COM_REG_MCR, 0x0f);
    return 0;
}

static int boot_init_com(void)
{
    if ((boot_init_com_internal((com_base = COM1_BASE)) == 0) || (boot_init_com_internal((com_base = COM2_BASE)) == 0))
        return 0;
    return -1;
}

static void boot_putchar_com(uint8_t ch)
{
    uint8_t res;
    do
    {
        res = inb(com_base + COM_REG_LSR);
        res &= 0x20;
    } while (res == 0);

    outb(com_base, ch);
}

static void boot_clear_screen_com(void)
{
    boot_putchar_com('\x1B');
    boot_putchar_com('[');
    boot_putchar_com('2');
    boot_putchar_com('J');

    boot_putchar_com('\x1B');
    boot_putchar_com('[');
    boot_putchar_com('2');
    boot_putchar_com('J');

    boot_putchar_com('\x1B');
    boot_putchar_com('[');
    boot_putchar_com('0');
    boot_putchar_com('m');
}

bool boot_tty_has_fb(void)
{
    return (tty_type & TTY_FB) == TTY_FB;
}

bool boot_tty_has_com(void)
{
    return (tty_type & TTY_COM) == TTY_COM;
}

void boot_clear_screen(void)
{
    if (boot_tty_has_fb())
        boot_clear_screen_fb();
    if (boot_tty_has_com())
        boot_clear_screen_com();
}

void boot_cursor_back(void)
{
    if (boot_tty_has_fb())
        boot_cursor_back_fb();
    if (boot_tty_has_com())
        boot_putchar_com('\r');
}

void boot_back_space(void)
{
    if (boot_tty_has_fb())
        boot_back_space_fb();
    if (boot_tty_has_com())
        boot_putchar_com('\b');
}

void boot_putchar_tab(void)
{
    if (boot_tty_has_fb())
        boot_putchar_tab_fb();
    if (boot_tty_has_com())
        boot_putchar_com('\t');
}

void boot_put_new_line(void)
{
    if (boot_tty_has_fb())
        boot_putchar_new_line_fb(0);
    if (boot_tty_has_com())
        boot_putchar_com('\r');
    boot_putchar_com('\n');
}

void boot_putchar(uint8_t ch)
{
    switch (ch)
    {
    case '\r':
        boot_cursor_back();
        break;
    case '\b':
        boot_back_space();
        break;
    case '\t':
        boot_putchar_tab();
        break;
    case '\n':
        boot_put_new_line();
        break;
    default:
        if (boot_tty_has_fb())
            boot_putchar_fb(ch, 0xffffff, 0);
        if (boot_tty_has_com())
            boot_putchar_com(ch);
    }
}

void boot_puts(const char *str)
{
    while (*str != '\0')
    {
        boot_putchar(*str);
        str++;
    }
}

void boot_puti(int64_t num)
{
    char buf[32];
    char *ptr = buf;
    if (num < 0)
    {
        num = 0 - num;
        *ptr++ = '-';
    }
    itoa(num, &ptr, 10);
    *ptr++ = 0;
    boot_puts(buf);
}

void boot_puth(uint64_t num)
{
    char buf[32];
    char *ptr = buf;
    itoa(num, &ptr, 16);
    *ptr++ = 0;
    boot_puts(buf);
}

int boot_tty_init(multiboot_uint8_t *mbi)
{
    if (boot_get_frame_buffer(mbi) < 0)
    {
        max_ch_nr_x = DEFAULT_TTY_WIDTH;
        max_ch_nr_y = DEFAULT_TTY_HEIGHT;
    }
    else
    {
        tty_type |= TTY_FB;
        boot_init_font();
    }

    if (boot_init_com() == 0)
        tty_type |= TTY_COM;

    boot_clear_screen();

    if (tty_type == NO_TTY)
        return -1;
    if (!boot_tty_has_fb())
        boot_puts("[!] Warning: running under no-graphic mode.\n");
    else if (!boot_tty_has_com())
        boot_puts("[!] Warning: no serial ports for boot tty.\n");

    boot_puts("Hello u64OS!\n\n");
    boot_puts("Version 0.0.1\n");
    boot_puts("Copyright (c) 2025-2026 xm-xyt03. All rights reserved.\n\n");
    return 0;
}