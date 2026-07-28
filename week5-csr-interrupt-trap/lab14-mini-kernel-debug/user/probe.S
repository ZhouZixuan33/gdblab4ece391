.equ SYS_PROBE, 1
.equ SYS_EXIT,  2
.equ PROBE_OK,  0x391

.section .text
.option norvc
.globl _user_start
.globl user_probe_ecall
.globl probe_returned
.globl user_exit_ecall
.globl user_fail

_user_start:
    li a7, SYS_PROBE

user_probe_ecall:
    ecall

probe_returned:
    li t0, PROBE_OK
    bne a0, t0, user_fail

    li a0, 0
    li a7, SYS_EXIT
user_exit_ecall:
    ecall

user_fail:
    li a0, 1
    li a7, SYS_EXIT
    ecall

1:
    j 1b
