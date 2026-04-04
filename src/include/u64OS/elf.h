#ifndef u64OS_ELF_H
#define u64OS_ELF_H

#include <stdtypes.h>

typedef uint16_t Elf32_Half;
typedef uint16_t Elf64_Half;

typedef uint32_t Elf32_Word;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;

typedef uint64_t Elf32_Xword;
typedef int64_t Elf32_Sxword;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;

typedef uint32_t Elf32_Addr;
typedef uint64_t Elf64_Addr;

typedef uint32_t Elf32_Off;
typedef uint64_t Elf64_Off;

typedef struct
{
    Elf32_Word p_type;
    Elf32_Off p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_file_size;
    Elf32_Word p_mem_size;
    Elf32_Word p_flags;
    Elf32_Word p_align;
} Elf32_Phdr;

typedef struct
{
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    Elf64_Xword p_file_size;
    Elf64_Xword p_mem_size;
    Elf64_Xword p_align;
} Elf64_Phdr;

enum
{
    PT_NULL,
    PT_LOAD,
    PT_DYNAMIC,
    PT_INTEPRE,
    PT_NOTE,
    PT_SHLIB,
    PT_PHDR
};

typedef struct
{
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addr_align;
    Elf32_Word sh_ent_size;
} Elf32_Shdr;

typedef struct
{
    Elf64_Word sh_name;
    Elf64_Word sh_type;
    Elf64_Xword sh_flags;
    Elf64_Addr sh_addr;
    Elf64_Off sh_offset;
    Elf64_Xword sh_size;
    Elf64_Word sh_link;
    Elf64_Word sh_info;
    Elf64_Xword sh_addr_align;
    Elf64_Xword sh_ent_size;
} Elf64_Shdr;

enum
{
    SHT_NULL,
    SHT_PROGBITS,
    SHT_SYMTAB,
    SHT_STRTAB,
    SHT_RELA,
    SHT_HASH,
    SHT_DYNAMIC,
    SHT_NOTE,
    SHT_NOBITS,
    SHT_REL,
    SHT_SHLIB,
    SHT_DYNSYM,
    SHT_LOPROC = 0x70000000,
    SHT_HIPROC = 0x7fffffff,
    SHT_LOUSER = 0x80000000,
    SHT_HIUSER = 0xffffffff
};

enum
{
    SHF_WRITE = 0x1,
    SHF_ALLOC = 0x2,
    SHF_EXECIINSTR = 0x4,
    SHF_MASKPROC = 0xf0000000
};

#endif // !u64OS_ELF_H