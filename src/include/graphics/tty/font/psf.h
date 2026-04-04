#ifndef GRAPHICS_TTY_FONT_PSF_H
#define GRAPHICS_TTY_FONT_PSF_H

#include <stdtypes.h>

#define PSF1_FONT_MAGIC 0x0436

struct psf1_header {
    uint16_t magic;
    uint8_t font_mode;
    uint8_t char_size;
};

#define PSF2_FONT_MAGIC 0x864ab572

struct psf2_header {
    uint32_t magic;
    uint32_t version;
    uint32_t header_size;
    uint32_t flags;
    uint32_t glyph_nr;
    uint32_t bytes_per_glyph;
    uint32_t height;
    uint32_t width;
};

#endif // !GRAPHICS_TTY_FONT_PSF_H 