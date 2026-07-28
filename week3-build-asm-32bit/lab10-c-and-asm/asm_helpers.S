    .section .text

    .globl asm_add_three
    .type asm_add_three, @function
asm_add_three:
    add a0, a0, a1          # Add the first argument (a0) and second argument (a1).
    add a0, a0, a2          # Add the third argument (a2); a0 carries the return value.
    ret                     # Return to C through ra.

    .globl check_preserves_s1
    .type check_preserves_s1, @function
check_preserves_s1:
    # On entry, ra contains the address used to return to C. The call below will
    # overwrite ra, so save that original return address and restore it before ret.
    addi sp, sp, -32        # Allocate a 32-byte, 16-byte-aligned stack frame.
    sd ra, 24(sp)           # Save all 64 bits of the return-to-C address.
    sd s0, 16(sp)           # Save all 64 bits of the old frame pointer.
    sd s1, 8(sp)            # Save all 64 bits of callee-saved s1.
    addi s0, sp, 32         # Set s0/fp to the stack pointer value on entry.

    li s1, 0x39139139       # Put a sentinel value in s1.
    call broken_helper      # ra now holds the address of the next instruction here.

    li t0, 0x39139139       # Load the expected sentinel into temporary register t0.
    bne s1, t0, bad_preserve  # If s1 changed, branch to the failure case.

    li a0, 0                # Return 0 for success.
    j done_preserve         # Skip the failure return value.

bad_preserve:
    li a0, 1                # Return 1 for failure.

done_preserve:
    ld s1, 8(sp)            # Restore all 64 bits of the caller's s1.
    ld s0, 16(sp)           # Restore all 64 bits of the caller's s0/fp.
    ld ra, 24(sp)           # Restore the 64-bit return-to-C address.
    addi sp, sp, 32         # Release the 32-byte stack frame.
    ret                     # Return to the address in ra.

    .globl broken_helper
    .type broken_helper, @function
broken_helper:
    addi sp, sp, -16
    sd s1, 8(sp)
    li s1, 0x0badc0de       # BUG: s1 is callee-saved, but this function does not restore it.
    li a0, 7                # Put return value 7 in a0.
    ld s1, 8(sp)
    addi sp, sp, 16
    ret                     # Return without fixing s1.
