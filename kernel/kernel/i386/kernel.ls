OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386) 
SEARCH_DIR("/cross/lib/gcc-lib/i386-elf/3.3.3")

ENTRY(Entry)
SECTIONS 
{ 
	. = 0x00100000;
	__loader_start = .;
		
	.text :
	{ 
		_stext = .;
		kernel/i386/asm/entry.o(.text)
		*(.text)
	}	
  	
  	.rodata :
  	{
		_srodata = .;
		*(.rodata)
		
		*(.rodata.*)
		*(.rodata1)
		
		_etext = .;
  	}
  	
  	
	.data ALIGN (0x1000) :
	{ 
		_sdata = .;
		*(.data)
		_edata = .;
	}
	
	.bss :
	{ 
		_sbss = .;
		*(COMMON)
		*(.bss)
		_ebss = .;
	}
	
	__loader_end = .;
} 
