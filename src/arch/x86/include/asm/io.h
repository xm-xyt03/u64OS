#include <stdtypes.h>
#include <u64OS/compiler.h>

static __always_inline void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %w1" : : "a" (value), "d" (port));
}

static __always_inline void outw(uint16_t port, uint16_t value) {
    asm volatile ("outw %0, %w1" : : "a" (value), "d" (port));
}

static __always_inline void outl(uint16_t port, uint32_t value) {
    asm volatile ("outl %0, %w1" : : "a" (value), "d" (port));
}

static __always_inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile ("inb %w1, %0" : "=a" (value) : "d" (port));
    return value;
}

static __always_inline uint16_t inw(uint16_t port) {
    uint16_t value;
    asm volatile ("inw %w1, %0" : "=a" (value) : "d" (port));
    return value;
}

static __always_inline uint32_t inl(uint16_t port) {
    uint32_t value;
    asm volatile ("inl %w1, %0" : "=a" (value) : "d" (port));
    return value;
}