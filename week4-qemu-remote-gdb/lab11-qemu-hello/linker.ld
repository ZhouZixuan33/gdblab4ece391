OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
  . = 0x80000000;

  .text : {
    KEEP(*(.text.start))
    *(.text*)
  }

  .rodata : {
    *(.srodata*)
    *(.rodata*)
  }

  .data : {
    *(.sdata*)
    *(.data*)
  }

  .bss : {
    *(.sbss*)
    *(.bss*)
    *(COMMON)
    . = ALIGN(16);
  }
}
