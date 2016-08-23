OUTPUT_ARCH(mips)
ENTRY(startup)
SECTIONS {
	. = 0x80500000                                                   ;
	.text : {
		text_start = .;
		*(.text)
		*(.rodata)
		text_end = .;
	}

	.data : {
		data_start = .;
		*(.data)
		data_end = .;
	}

	.bss : {
		bss_start = .;
		*(.bss)
		bss_end = .;
	}
}
