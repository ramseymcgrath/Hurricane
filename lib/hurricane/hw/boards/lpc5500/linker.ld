/**
 * @file linker.ld
 * @brief Linker script for LPC55S69
 */

MEMORY
{
  /* Define memory regions for LPC55S69 */
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 640K   /* 640KB Flash */
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 320K   /* 320KB SRAM */
}

ENTRY(Reset_Handler)

SECTIONS
{
  .text :
  {
    KEEP(*(.vectors))
    *(.text*)
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > FLASH

  .ARM.exidx :
  {
    __exidx_start = .;
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    __exidx_end = .;
  } > FLASH

  .data :
  {
    __data_start__ = .;
    *(vtable)
    *(.data*)
    . = ALIGN(4);
    __data_end__ = .;
  } > RAM AT > FLASH

  .bss :
  {
    __bss_start__ = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
  } > RAM

  .heap :
  {
    __heap_start__ = .;
    . += 0x8000;  /* 32KB heap */
    __heap_end__ = .;
  } > RAM

  .stack :
  {
    . += 0x2000;  /* 8KB stack */
    __stack_top__ = .;
  } > RAM
}