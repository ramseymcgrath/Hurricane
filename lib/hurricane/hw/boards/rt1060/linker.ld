MEMORY
{
  FLASH (rx) : ORIGIN = 0x60000000, LENGTH = 4M
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 1M
}

SECTIONS
{
  .text : {
    KEEP(*(.isr_vector))
    *(.text*)
    *(.rodata*)
  } > FLASH

  .data : {
    *(.data*)
  } > RAM AT> FLASH

  .bss : {
    *(.bss*)
    *(COMMON)
  } > RAM
}
