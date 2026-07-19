/*
 * Lab 11 reuses the Lab 10 calling-convention example at runtime.
 * GDB can stop inside these functions while QEMU is executing the target.
 */

.section .text

.globl asm_add_three
.type asm_add_three, @function
asm_add_three:
    add a0, a0, a1
    add a0, a0, a2
    ret

.globl check_preserves_s1
.type check_preserves_s1, @function
check_preserves_s1:
    addi sp, sp, -32
    sd ra, 24(sp)
    sd s0, 16(sp)
    sd s1, 8(sp)
    addi s0, sp, 32

    li s1, 0x39139139
    call broken_helper

    li t0, 0x39139139
    bne s1, t0, bad_preserve

    li a0, 0
    j done_preserve

.globl bad_preserve
bad_preserve:
    li a0, 1

.globl done_preserve
done_preserve:
    ld s1, 8(sp)
    ld s0, 16(sp)
    ld ra, 24(sp)
    addi sp, sp, 32
    ret

.globl broken_helper
.type broken_helper, @function
broken_helper:
    li s1, 0x0badc0de
    li a0, 7
    ret
