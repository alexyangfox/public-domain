#include "kernel.h"

struct IDTEntry IDT[256];
struct IDTLoc IDTPtr;

void EncodeIDTEntry(int num, struct IDTEncode *entry)
{
	IDT[num].LowBase = (entry->base & 0xFFFF);
	IDT[num].segment = 0x08;
	IDT[num].zero = 0x00;
	IDT[num].flags = entry->flags;
	IDT[num].HighBase = (entry->base >> 16) & 0xFFFF;
}

void setupIDT()
{
	IDTPtr.limit = (sizeof(struct IDTEntry) * 256) - 1;
	IDTPtr.base = (unsigned int) &IDT;

	memset((unsigned char *)&IDT, 0x00, sizeof(struct IDTEntry) * 256);
	struct IDTEncode *entry;
	
	entry->base = (unsigned) isr0;
	entry->segment = 0x08;
	entry->flags = 0x8E;
	EncodeIDTEntry(0, entry);

	entry->base = (unsigned) isr1;
	EncodeIDTEntry(1, entry);

	entry->base = (unsigned) isr2;
	EncodeIDTEntry(2, entry);

	entry->base = (unsigned) isr3;
	EncodeIDTEntry(3, entry);

	entry->base = (unsigned) isr4;
	EncodeIDTEntry(4, entry);

	entry->base = (unsigned) isr5;
	EncodeIDTEntry(5, entry);

	entry->base = (unsigned) isr6;
	EncodeIDTEntry(6, entry);

	entry->base = (unsigned) isr7;
	EncodeIDTEntry(7, entry);

	entry->base = (unsigned) isr8;
	EncodeIDTEntry(8, entry);

	entry->base = (unsigned) isr9;
	EncodeIDTEntry(9, entry);

	entry->base = (unsigned) isr10;
	EncodeIDTEntry(10, entry);

	entry->base = (unsigned) isr11;
	EncodeIDTEntry(11, entry);

	entry->base = (unsigned) isr12;
	EncodeIDTEntry(12, entry);

	entry->base = (unsigned) isr13;
	EncodeIDTEntry(13, entry);

	entry->base = (unsigned) isr14;
	EncodeIDTEntry(14, entry);

	entry->base = (unsigned) isrReserved;
	EncodeIDTEntry(15, entry);

	entry->base = (unsigned) isr16;
	EncodeIDTEntry(16, entry);

	entry->base = (unsigned) isr17;
	EncodeIDTEntry(17, entry);

	entry->base = (unsigned) isr18;
	EncodeIDTEntry(18, entry);

	entry->base = (unsigned) isrReserved;
	for(int index = 19; index < 32; index++)
	{
		EncodeIDTEntry(index, entry);
	}

	entry->base = (unsigned) irq0;
	EncodeIDTEntry(32, entry);

	entry->base = (unsigned) irq1;
	EncodeIDTEntry(33, entry);

	entry->base = (unsigned) irq7;
	EncodeIDTEntry(39, entry);

	loadIDT();
}
