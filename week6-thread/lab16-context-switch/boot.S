.section .text.boot
.globl _start
_start:
    la sp, boot_stack_top
    call lab16_main
1:  wfi
    j 1b

.section .bss
.balign 16
boot_stack:
    .skip 4096
boot_stack_top:
