OUTPUT_ARCH ( arm )
ENTRY ( _start )
SECTIONS
{
	.text 0x8000:
	{
		. = ALIGN(4);
		KEEP(*(.startup))
		*(.text)
		*(.text*)
	}

	__rodata_start = .;
	.rodata : { *(.rodata*) }
	. = ALIGN(4);
	__rodata_end = .;

	__data_start = . ;
	.data : { *(.data*) }
	. = ALIGN(4);
	__data_end = . ;

	__bss_start = . ;
	.bss : { *(.bss*) }
	. = ALIGN(4);
	__bss_end = . ;
}
