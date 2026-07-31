.section .text.boot
.globl _start
_start:
    la sp, boot_stack_top
    call lab17_main
1:  wfi
    j 1b
.section .bss
.balign 16
boot_stack:
    .skip 8192
boot_stack_top:
