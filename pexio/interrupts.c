#include "kernel.h"

void remapPIC()
{
	#define PIC1 0x20 /* The first PIC in the chain and */
	#define PIC2 0xA0 /* the second. Data is PIC+1. */
	
	#define ICW1 0x11
	/* Initialisation control word 1, explanation:
		0001 0001
		Bits 5-7:
			Are set to 0 in 8086/8088 mode
		Bit 4:
			ICW is indicated by a control word with D4 set.
		Bits 0-3:
			Bit 3 - LTIM - If set then PIC operates in level
				interrupt mode, else edge detect).
			Bit 2 - ADI - CALL address interval, if set then
				4, else 0.
			Bit 1 - SNGL - If set, only one PIC used, so no ICW3
				is issued.
			Bit 0 - IC4 - If set, PIC will read ICW4.
	*/

	/* Initialisation control word 2, explanation:
		This is the vector offsets for the indices into the IDT for each
		PIC.
	*/
	
	/* Initialisation control word 3, explanation:
		0000 0100 for master (PIC1)
		0000 0010 for slave (PIC2)
		For PIC1, the bit set is the IR input with a slave.
		For PIC2, the least three bits indicate the IR input to which
			it is slave (or alternatively, the master IR input 
	*/

	#define ICW4 0x01
	/* Initialisation control word 4, explanation:
		0000 0001
		Bits 5-7:
			Are set to 0 in 8086/8088 mode
		Bit 4-0:
			Bit 4 - SFNM - Set for special fully nested mode, else
				not special fully nested mode
			Bit 3 - BUF - Set for buffered mode
			Bit 2 - M/S - If BUF set then set for master, else slave
			Bit 1 - AEOI - Set for auto End of Interrupt
			Bit 0 - muPM - Set for 8086/8088 mode, else MCS-80/85
	*/


			
	unsigned char IRQmaskPIC1 = 0x00;
	unsigned char IRQmaskPIC2 = 0x00;

	/* Store the IRQ masks for each PIC */
	IRQmaskPIC1 = inportb(PIC1 + 1);
	IRQmaskPIC2 = inportb(PIC2 + 1);

	/* Send ICW1 to each PIC */
	outportb(PIC1,ICW1);
	wait();
	outportb(PIC2,ICW1);
	wait();

	/* Send ICW2 to each PIC */
	outportb(PIC1 + 1, 0x20);
	wait();
	outportb(PIC2 + 1, 0x28);
	wait();

	/* Send ICW3 to each PIC */
	outportb(PIC1 + 1, 0x04);
	wait();
	outportb(PIC2 + 1, 0x02);
	wait();

	/* Send ICW4 to each PIC */
	outportb(PIC1 + 1, ICW4);
	wait();
	outportb(PIC2 + 1, ICW4);
	wait();

	/* Restore interrupt masks */
	outportb(PIC1 + 1, IRQmaskPIC1);
	outportb(PIC2 + 1, IRQmaskPIC2);
}

void CISR0(unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* Divide by zero exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Divide by zero");
	
	char buf[10];
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR1(unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* Debug exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Debug");

	char buf[10];
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR2(unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* NMI exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Non maskable interrupt");

	char buf[10];
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR3(unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* Breakpoint exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Breakpoint");

	char buf[10];
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR4(unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* Overflow exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Overflow");
	
	char buf[10];
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR5(unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* Out of bounds exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Out of bounds");
	
	char buf[10];
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR6(unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* Invalid opcode exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Invalid opcode");

	char buf[10];
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR7(unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* No coprocessor exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: No coprocessor");

	char buf[10];
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR8(unsigned int errcode, unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* Double fault exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Double fault");
	
	char buf[10];
	puts("\n Error: 0x");
	puts(itoa(errcode, buf, 16));
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR9(unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* Coprocessor segment overrun exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Coprocessor segment overrun exception");

	char buf[10];
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR10(unsigned int errcode, unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* Bad TSS exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Bad TSS");

	char buf[10];
	puts("\n Error: 0x");
	puts(itoa(errcode, buf, 16));
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR11(unsigned int errcode, unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* Segment not present exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Segment not present");

	char buf[10];
	puts("\n Error: 0x");
	puts(itoa(errcode, buf, 16));
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR12(unsigned int errcode, unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* Stack fault exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Stack fault");

	char buf[10];
	puts("\n Error: 0x");
	puts(itoa(errcode, buf, 16));
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR13(unsigned int errcode, unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* GPF exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: General protection fault");

	char buf[10];
	puts("\n Error: 0x");
	puts(itoa(errcode, buf, 16));
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR14(unsigned int errcode, unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* Page fault exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Page fault");

	char buf[10];
	puts("\n Error: 0x");
	puts(itoa(errcode, buf, 16));
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR15(unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* Unknown interrupt exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Unknown interrupt");

	char buf[10];
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR16(unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* Coprocessor fault exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Coprocessor fault");

	char buf[10];
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR17(unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* Alignment check exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Alignment check");

	char buf[10];
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISR18(unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* Machine check exception */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Machine check");

	char buf[10];
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}

void CISRReserved(unsigned int eip, unsigned int cs, unsigned int eflags)
{
	/* Reserved IRQ */
	CursorPos.colour = 0xEC00;
	puts("\nEXCEPTION: Reserved IRQ");

	char buf[10];
	puts("\n EIP: 0x");
	puts(itoa(eip, buf, 16));
	puts("\n CS: 0x");
	puts(itoa(cs, buf, 16));
	puts("\n EFLAGS: 0x");
	puts(itoa(eflags, buf, 16));
	for(;;);
}
