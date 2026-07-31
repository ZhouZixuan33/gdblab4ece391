#ifndef LAB15_ELF_LOADER_H
#define LAB15_ELF_LOADER_H

typedef unsigned long u64;

struct loaded_user {
    u64 entry;
    u64 segment_start;
    u64 segment_end;
};

int load_user_elf(const unsigned char *image, u64 image_size,
                  struct loaded_user *result);

#endif
