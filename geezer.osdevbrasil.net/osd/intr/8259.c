/* re-program the 8259 chips so
IRQ 0-7 -> INT 20h-27h, IRQ 8-15 -> INT 28h-2Fh

This code snippet is public domain (no copyright).
You can do whatever you want with it. */

	outportb(0x20, 0x11); /* ICW1 */
	outportb(0xA0, 0x11);

	outportb(0x21, 0x20); /* ICW2: route IRQs 0...7 to INTs 20h...27h */
	outportb(0xA1, 0x28); /* ...IRQs 8...15 to INTs 28h...2Fh */

	outportb(0x21, 0x04); /* ICW3 */
	outportb(0xA1, 0x02);

	outportb(0x21, 0x01); /* ICW4 */
	outportb(0xA1, 0x01);

