#include "elf_loader.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

enum {
    EI_NIDENT = 16,
    PT_LOAD = 1,
    EM_RISCV = 243,
};

static const u64 USER_LOAD_MIN = 0x80400000UL;
static const u64 USER_LOAD_MAX = 0x80410000UL;

struct elf64_header {
    u8 ident[EI_NIDENT];
    u16 type;
    u16 machine;
    u32 version;
    u64 entry;
    u64 phoff;
    u64 shoff;
    u32 flags;
    u16 ehsize;
    u16 phentsize;
    u16 phnum;
    u16 shentsize;
    u16 shnum;
    u16 shstrndx;
};

struct elf64_program_header {
    u32 type;
    u32 flags;
    u64 offset;
    u64 vaddr;
    u64 paddr;
    u64 filesz;
    u64 memsz;
    u64 align;
};

static int range_inside(u64 start, u64 size, u64 lower, u64 upper) {
    return start >= lower && start <= upper && size <= upper - start;
}

static void copy_bytes(u8 *dst, const u8 *src, u64 count) {
    while (count-- != 0)
        *dst++ = *src++;
}

static void zero_bytes(u8 *dst, u64 count) {
    while (count-- != 0)
        *dst++ = 0;
}

int load_user_elf(const unsigned char *image, u64 image_size,
                  struct loaded_user *result) {
    const struct elf64_header *header;
    u64 i;
    int loaded = 0;

    if (image_size < sizeof(struct elf64_header))
        return -1;

    header = (const struct elf64_header *)image;
    if (header->ident[0] != 0x7f || header->ident[1] != 'E' ||
        header->ident[2] != 'L' || header->ident[3] != 'F')
        return -2;
    if (header->ident[4] != 2 || header->ident[5] != 1)
        return -3;
    if (header->machine != EM_RISCV ||
        header->phentsize != sizeof(struct elf64_program_header))
        return -4;
    if (!range_inside(header->phoff,
                      (u64)header->phnum * header->phentsize,
                      0, image_size))
        return -5;

    result->entry = header->entry;
    result->segment_start = USER_LOAD_MAX;
    result->segment_end = USER_LOAD_MIN;

    for (i = 0; i < header->phnum; ++i) {
        const struct elf64_program_header *program =
            (const struct elf64_program_header *)(image + header->phoff +
                                                   i * header->phentsize);
        u64 destination;

        if (program->type != PT_LOAD)
            continue;
        if (program->filesz > program->memsz ||
            !range_inside(program->offset, program->filesz, 0, image_size))
            return -6;

        destination = program->paddr != 0 ? program->paddr : program->vaddr;
        if (!range_inside(destination, program->memsz,
                          USER_LOAD_MIN, USER_LOAD_MAX))
            return -7;

        copy_bytes((u8 *)destination, image + program->offset, program->filesz);
        zero_bytes((u8 *)(destination + program->filesz),
                   program->memsz - program->filesz);

        if (destination < result->segment_start)
            result->segment_start = destination;
        if (destination + program->memsz > result->segment_end)
            result->segment_end = destination + program->memsz;
        loaded = 1;
    }

    if (!loaded || result->entry < result->segment_start ||
        result->entry >= result->segment_end)
        return -8;

    return 0;
}
