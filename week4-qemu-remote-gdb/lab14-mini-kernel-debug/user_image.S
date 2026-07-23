#ifndef USER_ELF_PATH
#define USER_ELF_PATH "build/solution/user.elf"
#endif

.section .user_elf, "a"
.balign 8
.globl embedded_user_elf_start
.globl embedded_user_elf_end
embedded_user_elf_start:
    .incbin USER_ELF_PATH
embedded_user_elf_end:
