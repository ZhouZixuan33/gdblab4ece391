.equ SYS_REGCHECK,  1
.equ SYS_ENABLE_IRQ, 2
.equ SYS_EXIT,       3
.equ USER_IRQ_FLAG,  0x80401000

.equ SENTINEL_S1, 0x151
.equ SENTINEL_S2, 0x252
.equ SENTINEL_S3, 0x353
.equ SENTINEL_T3, 0x454
.equ SENTINEL_A1, 0x515

.section .text
.option norvc
.globl _user_start
.globl user_regcheck_ecall
.globl user_wait_for_uart
.globl user_after_uart
.globl user_exit_ecall
.globl user_fail

_user_start:
    li s1, SENTINEL_S1
    li s2, SENTINEL_S2
    li s3, SENTINEL_S3
    li t3, SENTINEL_T3
    li a1, SENTINEL_A1

    # Part A: a synchronous trap checks save/restore without enabling UART.
    li a7, SYS_REGCHECK
user_regcheck_ecall:
    ecall
    call verify_sentinels

    # Part B: the kernel now enables UART, PLIC, and CPU interrupt delivery.
    li a7, SYS_ENABLE_IRQ
    ecall
    call verify_sentinels

user_wait_for_uart:
    li t0, USER_IRQ_FLAG
1:
    ld t1, 0(t0)
    beqz t1, 1b

user_after_uart:
    call verify_sentinels
    li a0, 0
    li a7, SYS_EXIT
user_exit_ecall:
    ecall

user_fail:
    li a0, 1
    li a7, SYS_EXIT
    ecall
2:
    j 2b

verify_sentinels:
    li t0, SENTINEL_S1
    bne s1, t0, user_fail
    li t0, SENTINEL_S2
    bne s2, t0, user_fail
    li t0, SENTINEL_S3
    bne s3, t0, user_fail
    li t0, SENTINEL_T3
    bne t3, t0, user_fail
    li t0, SENTINEL_A1
    bne a1, t0, user_fail
    ret

