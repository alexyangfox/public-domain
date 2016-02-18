/* let the linker use its 'native' format (ELF/COFF/PE)
OUTPUT_FORMAT("coff-go32") */
/* no leading underscores for symbols handled in asm: */
ENTRY(kentry)
load_adr = 0x100000; /* 1 meg = physical address */
SECTIONS
{
    .text 0xC0000000 : AT(load_adr)
    {
/* discardable kernel code */
	g_d_code = .; _g_d_code = .;
	*(.dtext)
	. = ALIGN(4096);
/* kernel code */
	g_code = .; _g_code = .;
	*(.text*)
/* ELF constant data section(s)
Thanks to Jeroen van Bemmel for helping with this. */
	*(.rodata*)
	. = ALIGN(4096);
/* fill empty space at end of section with INT 3 (breakpoint) */
    } = 0xCCCC
    .data : AT(load_adr + (g_d_data - g_d_code))
    {
/* discardable kernel data */
	g_d_data = .; _g_d_data = .;
	*(.ddata)
	. = ALIGN(4096);
/* kernel data */
	g_data = .; _g_data = .;
	*(.data*)
	. = ALIGN(4096);
    } = 0xCCCC
    .bss : AT(load_adr + (g_d_bss - g_d_code))
    {
/* discardable kernel BSS */
	g_d_bss = .; _g_d_bss = .;
	*(.dbss)
	. = ALIGN(4096);
/* kernel BSS */
	g_bss = .; _g_bss = .;
	*(.bss*)
	*(COMMON) /* "common" variables */
	. = ALIGN(4096);
    } = 0xCCCC
/* end of kernel */
    g_end = .; _g_end = .;
/* MinGW debug sections
Omit these and you get an invalid executable file */
    .stab :
    {
	*(.stab)
    }
    .stabstr :
    {
	*(.stabstr)
    }
}
