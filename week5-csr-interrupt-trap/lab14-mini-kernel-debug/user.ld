OUTPUT_ARCH(riscv)
ENTRY(_user_start)

PHDRS
{
  text PT_LOAD FLAGS(5);
}

SECTIONS
{
  . = 0x80400000;

  .text : {
    *(.text*)
    *(.rodata*)
  } :text

  /DISCARD/ : {
    *(.comment)
    *(.riscv.attributes)
  }
}
