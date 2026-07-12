    .section .text
    .globl check_preserves_s1
    .type check_preserves_s1, @function
check_preserves_s1:
    addi sp, sp, -16
    sw ra, 12(sp)
    sw s0, 8(sp)
    sw s1, 4(sp)
    addi s0, sp, 16

    li s1, 0x39139139
    call broken_helper

    li t0, 0x39139139
    bne s1, t0, bad_preserve

    li a0, 0
    j done_preserve

bad_preserve:
    li a0, 1

done_preserve:
    lw s1, 4(sp)
    lw s0, 8(sp)
    lw ra, 12(sp)
    addi sp, sp, 16
    ret

    .globl broken_helper
    .type broken_helper, @function
broken_helper:
    li s1, 0x0badc0de
    li a0, 7
    ret
