/*
*****************************************************************************
**
**  File        : AT32F421x8_FLASH.ld
**
**  Abstract    : Linker script for AT32F421K8U7 Device with
**                64KByte FLASH (62KB firmware + 2KB KVDB), 16KByte RAM
**
**                Flash layout:
**                  0x08000000 - 0x0800F7FF : Firmware (62KB)
**                  0x0800F800 - 0x0800FFFF : KVDB data (2KB, managed by FlashDB)
**
**  Target      : Artery Tek AT32F421
**
**  Environment : Arm gcc toolchain
**
*****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Stack and Heap sizes */
_Min_Stack_Size = 0x1000;    /* 4KB stack */
_Min_Heap_Size  = 0x40;      /* 64 bytes heap */

/* Specify the memory areas */
MEMORY
{
FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 62K    /* 62KB for firmware, last 2KB reserved for KVDB */
RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 16K
}

/* Symbols for cm_backtrace */
_stext = ORIGIN(FLASH);

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >FLASH

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM AT> FLASH

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM

  /* User_heap_stack section */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    _sstack = .;       /* stack bottom */
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /* Set stack top to end of RAM */
  _estack = ORIGIN(RAM) + LENGTH(RAM);

  /* Verify RAM usage */
  ASSERT(_estack <= ORIGIN(RAM) + LENGTH(RAM), "RAM overflow")

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
