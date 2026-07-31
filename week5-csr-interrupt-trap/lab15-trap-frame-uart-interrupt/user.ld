OUTPUT_ARCH(riscv)
ENTRY(_user_start)

PHDRS
{
  text PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
}

SECTIONS
{
  . = 0x80400000;

  .text : {
    *(.text*)
    *(.rodata*)
  } :text

  . = 0x80401000;
  .user_irq (NOLOAD) : {
    . = . + 16;
  } :data

  /DISCARD/ : {
    *(.comment)
    *(.riscv.attributes)
  }
}
