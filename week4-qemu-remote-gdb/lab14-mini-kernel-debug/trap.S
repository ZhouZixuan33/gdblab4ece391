.equ SCAUSE_ECALL_U, 8
.equ SYS_PROBE,      1
.equ SYS_EXIT,       2
.equ PROBE_OK,       0x391
.equ FRAME_SIZE,     64

.section .text
.balign 4
.globl supervisor_trap_entry
.globl before_probe_sret

supervisor_trap_entry:
    # Swap the user sp with the kernel sp saved in sscratch.
    csrrw sp, sscratch, sp

    # This intentionally small save area is sufficient only for probe.S.
    # Lab 15 replaces it with a complete trap frame.
    addi sp, sp, -FRAME_SIZE
    sd ra,  0(sp)
    sd t0,  8(sp)
    sd t1, 16(sp)
    sd t2, 24(sp)
    sd a0, 32(sp)
    sd a7, 40(sp)
    csrr t0, sepc
    sd t0, 48(sp)
    csrr t0, sscratch
    sd t0, 56(sp)

    csrr t0, scause
    li t1, SCAUSE_ECALL_U
    bne t0, t1, unexpected_supervisor_trap

    li t0, SYS_PROBE
    beq a7, t0, handle_probe
    li t0, SYS_EXIT
    beq a7, t0, handle_exit
    j unexpected_supervisor_trap

handle_probe:
#if LAB14_SOLUTION
    # Reference solution for LAB14 TODO 3.
    ld t0, 48(sp)
    addi t0, t0, 4
    csrw sepc, t0
    li a0, PROBE_OK
#else
    # LAB14 TODO 3 (exercise): advance sepc over the 32-bit ecall and return
    # PROBE_OK in a0. This placeholder deliberately repeats the ecall.
    li a0, 0
#endif

before_probe_sret:
    ld ra,  0(sp)
    ld t0,  8(sp)
    ld t1, 16(sp)
    ld t2, 24(sp)
    # Do not restore old a0: a0 carries the probe return value.
    ld a7, 40(sp)
    addi sp, sp, FRAME_SIZE
    csrrw sp, sscratch, sp
    sret

handle_exit:
    # SYS_EXIT never returns to U-mode, so calling C cannot corrupt a user
    # context that needs to be restored.
    call kernel_exit_from_user
1:
    wfi
    j 1b

unexpected_supervisor_trap:
    call kernel_unexpected_trap
2:
    wfi
    j 2b
