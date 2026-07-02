    .text
    .globl check_preserves_ebx
    .type check_preserves_ebx, @function
check_preserves_ebx:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx

    movl $0x39139139, %ebx
    call broken_helper
    cmpl $0x39139139, %ebx
    jne .Lbad_preserve

    movl $0, %eax
    jmp .Ldone_preserve

.Lbad_preserve:
    movl $1, %eax

.Ldone_preserve:
    popl %ebx
    leave
    ret

    .globl broken_helper
    .type broken_helper, @function
broken_helper:
    movl $0x0badc0de, %ebx
    movl $7, %eax
    ret

    .section .note.GNU-stack,"",@progbits
