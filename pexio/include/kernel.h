#ifndef KERNEL_H
#define KERNEL_H

#include "keyboard.h"

typedef unsigned long size_t;

/* kernel.c */
static __inline__ void outportb(unsigned short port, unsigned char val)
{
	asm volatile("outb %0,%1"::"a"(val), "Nd"(port));
}

static __inline__ void outportw(unsigned short port, unsigned short val)
{
	asm volatile("outw %0,%1"::"a"(val), "Nd" (port));
}

static __inline__ unsigned char inportb(unsigned short port)
{
	unsigned char ret;
	asm volatile("inb %1,%0":"=a"(ret):"Nd"(port));
	return ret;
}

static __inline__ void HaltProcessor()
{
	asm volatile("hlt");
}

extern int ticks;
static void outportb(unsigned short, unsigned char);
static void outportw(unsigned short, unsigned short);
static unsigned char inportb(unsigned short);
static void HaltProcessor();
unsigned char *memset(unsigned char*, unsigned char, size_t);
unsigned short *memsetw(unsigned short*, unsigned short, size_t);
void main(void*, unsigned int);
void SetTimerHz(int);
void wait();

/* descriptors.c */
struct IDTEncode
{
	unsigned long base;
	unsigned long segment;
	unsigned char flags;
};

struct IDTEntry
{
	unsigned short LowBase;
	unsigned short segment;
	unsigned char zero;
	unsigned char flags;
	unsigned short HighBase;
} __attribute__((packed));

struct IDTLoc
{
	unsigned short limit;
	unsigned int base;
} __attribute__((packed));

extern void reloadGDT();
void EncodeIDTEntry(int, struct IDTEncode*);
void setupIDT();
extern void loadIDT();

/* textmode.c */
struct Cursor
{
	unsigned char x;
	unsigned char y;
	unsigned short colour;
};

struct Cursor CursorPos;

void cls();
void setcur(unsigned char, unsigned char);
void putch(const char);
void puts(const char*);

/* textfunctions.c */
size_t strlen(const char*);
char *dtoc(int, char[]);
char *itoa(long, char[], int);

/* interrupts.c */
static __inline__ void EnableInterrupts()
{
	asm volatile("sti");
}

static __inline__ void DisableInterrupts()
{
	asm volatile("cli");
}

static void EnableInterrupts();
static void DisableInterrupts();
void remapPIC();
extern void isr0(); /* Division by zero exception */
void CISR0(unsigned int, unsigned int, unsigned int);
extern void isr1(); /* Debug exception */
void CISR1(unsigned int, unsigned int, unsigned int);
extern void isr2(); /* NMI exception */
void CISR2(unsigned int, unsigned int, unsigned int);
extern void isr3(); /* Breakpoint exception */
void CISR3(unsigned int, unsigned int, unsigned int);
extern void isr4(); /* Overflow exception */
void CISR4(unsigned int, unsigned int, unsigned int);
extern void isr5(); /* Out of bounds exception */
void CISR5(unsigned int, unsigned int, unsigned int);
extern void isr6(); /* Invalid opcode exception */
void CISR6(unsigned int, unsigned int, unsigned int);
extern void isr7(); /* No coprocessor exception */
void CISR7(unsigned int, unsigned int, unsigned int);
extern void isr8(); /* Double fault exception */
void CISR8(unsigned int, unsigned int, unsigned int, unsigned int);
extern void isr9(); /* Coprocessor segment overrun exception */
void CISR9(unsigned int, unsigned int, unsigned int);
extern void isr10(); /* Bad TSS exception */
void CISR10(unsigned int, unsigned int, unsigned int, unsigned int);
extern void isr11(); /* Segment not present exception */
void CISR11(unsigned int, unsigned int, unsigned int, unsigned int);
extern void isr12(); /* Stack fault exception */
void CISR12(unsigned int, unsigned int, unsigned int, unsigned int);
extern void isr13(); /* GPF exception */
void CISR13(unsigned int, unsigned int, unsigned int, unsigned int);
extern void isr14(); /* Page fault exception */
void CISR14(unsigned int, unsigned int, unsigned int, unsigned int);
extern void isr15(); /* Unknown interrupt exception */
void CISR15(unsigned int, unsigned int, unsigned int);
extern void isr16(); /* Coprocessor fault exception */
void CISR16(unsigned int, unsigned int, unsigned int);
extern void isr17(); /* Alignment check exception */
void CISR17(unsigned int, unsigned int, unsigned int);
extern void isr18(); /* Machine check exception */
void CISR18(unsigned int, unsigned int, unsigned int);
extern void isrReserved(); /* Reserved IRQ */
void CISRReserved(unsigned int, unsigned int, unsigned int);

/* irqs.c */
extern void irq0(); /* Programmable Interrupt Timer */
void CIRQ0();
extern void irq1(); /* Keyboard */
void CIRQ1();
extern void irq7(); /* IRn not raised high for sufficient time */
void CIRQ7();

#endif
