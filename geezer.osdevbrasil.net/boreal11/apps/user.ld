/* let the linker use its 'native' format (ELF/COFF/PE)
OUTPUT_FORMAT("coff-go32") */
/* no leading underscores for symbols handled in asm: */
ENTRY(uentry)
SECTIONS
{
    .text 0x400000 :
    {
/* task code */
	g_code = .; _g_code = .;
	*(.text)
/* ELF constant data section(s) */
	*(.rodata*)
	. = ALIGN(4096);
/* fill empty space at end of section with INT 3 (breakpoint) */
    } = 0xCCCC
    .data :
    {
/* task data */
	g_data = .; _g_data = .;
	*(.data)
	. = ALIGN(4096);
    } = 0xCCCC
    .bss :
    {
/* task BSS */
	g_bss = .; _g_bss = .;
	*(.bss)
	*(COMMON) /* "common" variables */
	. = ALIGN(4096);
    } = 0xCCCC
/* end of task data segment */
    g_end = .; _g_end = .;
/* MinGW debug sections
Omit these and you may get an invalid executable file */
    .stab :
    {
	*(.stab)
    }
    .stabstr :
    {
	*(.stabstr)
    }
}
