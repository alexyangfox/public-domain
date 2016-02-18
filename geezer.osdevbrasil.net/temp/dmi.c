/*----------------------------------------------------------------------------
SMBIOS (DMI) demo code
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: June 2, 2007
This code is public domain (no copyright).
You can do whatever you want with it.

Compile this code with a 16-bit DOS compiler such as Turbo C.
----------------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include <dos.h>
#if 0
/* C99 fixed-width types */
#include <stdint.h>
#else
typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;
#endif

static uint16_t g_pnp_ds;
static int far (*g_pnp_entry)(uint16_t fn, ...);
/*****************************************************************************
*****************************************************************************/
static int pnp_detect(void)
{
	unsigned char far *adr, csum;
	unsigned offset, len, i;

	printf("16-bit PnP BIOS...");
/* the spec says it's in this range, aligned on a 16-byte boundary */
	for(offset = 0; offset < 0xFFF0; offset += 16)
	{
		adr = (unsigned char far *)MK_FP(0xF000, offset);
/* bytes 0-3: signature */
		if(adr[0] != '$' || adr[1] != 'P' ||
			adr[2] != 'n' || adr[3] != 'P')
				continue;
/* byte 5: structure len */
		len = adr[5];
		if(len == 0)
			continue;
/* byte 8: checksum (checksum of entire structure must be 0) */
		csum = 0;
		for(i = 0; i < len; i++)
			csum += adr[i];
		if(csum)
			continue;
/* found it! */
		goto FOUND;
	}
	printf("not found\n");
	return -1;
FOUND:
/* byte 4: PnP version */
	i = adr[4];
	printf("version %u.%u, ", i >> 4, i & 0x0F);
/* store entry point */
	g_pnp_entry = MK_FP(
		*(uint16_t far *)(adr + 15),
		*(uint16_t far *)(adr + 13));
/* store DS value */
	g_pnp_ds = *(uint16_t far *)(adr + 27);
	printf("entry point at %Fp, DS=%04X\n", g_pnp_entry, g_pnp_ds);
	return 0;
}
/*****************************************************************************
*****************************************************************************/
#define BPERL		16	/* bytes per line for dump */

void dump(void *data_p, unsigned count)
{
	unsigned char *data = (unsigned char *)data_p;
	unsigned i, j;

	while(count != 0)
	{
		for(i = 0; i < BPERL; i++)
		{
			if(count == 0)
				break;
			printf("%02X ", data[i]);
			count--;
		}
		printf("\t");
		for(j = 0; j < i; j++)
		{
			if(data[j] < ' ')
				printf(".");
			else
				printf("%c", data[j]);
		}
		printf("\n");
		data += BPERL;
	}
}
/*****************************************************************************
*****************************************************************************/
static unsigned read_le16(void *buf_ptr)
{
	unsigned char *buf = buf_ptr;

	return buf[0] + buf[1] * 0x100;
}
/*****************************************************************************
*****************************************************************************/
static void display_string(uint8_t *buf, unsigned string_index, char *name)
{
	unsigned hdr_len, string_num, i;

	hdr_len = buf[1];
	if(string_index >= hdr_len)
		return;
	string_num = buf[string_index];
	if(string_num == 0)
		return;
	i = hdr_len;
	for(string_num--; string_num != 0; string_num--)
	{
/* find zero byte at end of string */
		while(buf[i] != '\0')
			i++;
		i++;
	}
/* two zero bytes means the end of the strings */
	if(buf[i] == '\0')
		return;
	printf("%s: `%s'\n", name, &buf[i]);
}
/*****************************************************************************
*****************************************************************************/
static void dump_bios(uint8_t *buf)
{
	unsigned hdr_len;

	hdr_len = buf[1];
	printf("*** BIOS ***\n");
	display_string(buf, 4, "Vendor");
	display_string(buf, 5, "Version");
	printf("Segment: 0x%04X\n", read_le16(&buf[6]));
	display_string(buf, 8, "Release date");
	printf("Size: %uK\n", (buf[9] + 1) * 64);
	if((buf[10] & 0x08) == 0)
	{
		printf("Features:");

		if(buf[10] & 0x10)
			printf(" ISA");
		if(buf[10] & 0x20)
			printf(" MCA");
		if(buf[10] & 0x40)
			printf(" EISA");
		if(buf[10] & 0x80)
			printf(" PCI");

		if(buf[11] & 0x01)
			printf(" PCMCIA");
		if(buf[11] & 0x02)
			printf(" PnP");
		if(buf[11] & 0x04)
			printf(" APM");
		if(buf[11] & 0x08)
			printf(" FlashBIOS");

		if(buf[11] & 0x10)
			printf(" ShadowBIOS");
		if(buf[11] & 0x20)
			printf(" VL-VESA");
		if(buf[11] & 0x40)
			printf(" ESCD");
		if(buf[11] & 0x80)
			printf(" BootFromCD");

		if(buf[12] & 0x01)
			printf(" SelectableBoot");
		if(buf[12] & 0x02)
			printf(" SocketedBIOS");
		if(buf[12] & 0x04)
			printf(" BootFromPCMCIA");
		if(buf[12] & 0x08)
			printf(" EDD");
		putchar('\n');
	}
	if(hdr_len >= 22)
		printf("Version: %u.%u\n", buf[hdr_len - 2], buf[hdr_len - 1]);
}
/*****************************************************************************
*****************************************************************************/
static void dump_system(uint8_t *buf)
{
	static const char *wake_type[] =
	{
		"Reserved", "Other", "Unknown", "APM Timer",
		"Modem Ring", "LAN Remote", "Power Switch", "PCI PME#",
		"AC Power Restored"
	};
/**/
	unsigned hdr_len;

	hdr_len = buf[1];
	printf("*** system ***\n");
	display_string(buf, 4, "Manufacturer");
	display_string(buf, 5, "Product Name");
	display_string(buf, 6, "Version");
	display_string(buf, 7, "Serial Number");
	if(hdr_len >= 24)
	{
		printf("UUID:\n");
		dump(&buf[8], 16);
	}
	if(hdr_len > 24)
	{
		printf("Wake-up type: ");
		if(buf[24] < sizeof(wake_type) / sizeof(wake_type[0]))
			printf("%s\n", wake_type[buf[24]]);
		else
			printf("unknown\n");
	}
	display_string(buf, 25, "SKU Number");
	display_string(buf, 26, "Family");
}
/*****************************************************************************
*****************************************************************************/
static void dump_board(uint8_t *buf)
{
	static const char *board_type[] =
	{
		"UNDEFINED", "Unknown", "Other", "Server Blade",
		"Connectivity Switch", "System Management Module",
			"Processor Module", "I/O Module",
		"Memory Module", "Daughter board",
			"Motherboard (includes processor, memory, and I/O)",
			"Processor/Memory Module",
		"Processor/IO Module", "Interconnect Board"
	};
/**/
	unsigned hdr_len;

	hdr_len = buf[1];
	printf("*** baseboard ***\n");
	display_string(buf, 4, "Manufacturer");
	display_string(buf, 5, "Product");
	display_string(buf, 6, "Version");
	display_string(buf, 7, "Serial Number");
	display_string(buf, 8, "Asset tag");
	if(hdr_len >= 10)
	{
		printf("Features:");
		if(buf[9] & 0x01)
			printf(" Motherboard");
		if(buf[9] & 0x02)
			printf(" RequiresDaughterboard");
		if(buf[9] & 0x04)
			printf(" Removable");
		if(buf[9] & 0x08)
			printf(" Replaceable");
		if(buf[9] & 0x10)
			printf(" HotSwappable");
	}
	display_string(buf, 10, "Location in chassis");
	if(hdr_len >= 13)
		printf("Chassis/enclosure handle=0x%04X\n",
			read_le16(&buf[11]));
	if(hdr_len > 13)
	{
		printf("Board type: ");
		if(buf[13] < sizeof(board_type) / sizeof(board_type[0]))
			printf("%s\n", board_type[buf[13]]);
		else
			printf("unknown\n");
	}
	if(hdr_len > 14)
		printf("%u Contained Objects\n", buf[14]);
// xxx - list of handles follows
}
/*****************************************************************************
*****************************************************************************/
static void dump_chassis(uint8_t *buf)
{
	unsigned hdr_len;

	hdr_len = buf[1];
	printf("*** chassis/enclosure ***\n");
	display_string(buf, 4, "Manufacturer");
//type	display_string(buf, 5, "Product");
	display_string(buf, 6, "Version");
	display_string(buf, 7, "Serial Number");
	display_string(buf, 8, "Asset Tag");


#if 0
 09h         2.1+    Boot-up State        BYTE       ENUM       Identifies the state of the enclosure when it was last
								booted. See 3.3.4.2 for definitions.
 0Ah         2.1+    Power Supply         BYTE       ENUM       Identifies the state of the enclosure's power supply (or
		     State                                      supplies) when last booted. See 3.3.4.2 for definitions.
 0Bh         2.1+    Thermal State        BYTE       ENUM       Identifies the enclosure's thermal state when last booted.
								See 3.3.4.2 for definitions.
 0Ch         2.1+    Security Status      BYTE       ENUM       Identifies the enclosure's physical security status when
								last booted. See 3.3.4.3 for definitions.
 0Dh         2.3+    OEM-defined         DWORD       Varies     Contains OEM- or BIOS vendor-specific information.
 11h         2.3+    Height               BYTE       Varies     The height of the enclosure, in 'U's. A U is a standard
								unit of measure for the height of a rack or rack-mountable
								component and is equal to 1.75 inches or 4.445 cm. A
								value of 00h indicates that the enclosure height is
								unspecified.
 12h         2.3+    Number of Power      BYTE       Varies     Identifies the number of power cords associated with the
		     Cords                                      enclosure or chassis. A value of 00h indicates that the
								number is unspecified.
 13h         2.3+    Contained            BYTE       Varies     Identifies the number of Contained Element records that
		     Element Count (n)                          follow, in the range 0 to 255. Each Contained Element
								group comprises m bytes, as specified by the Contained
								Element Record Length field that follows. If no Contained
								Elements are included, this field is set to 0.
					  BYTE       Varies     Identifies the byte length of each Contained Element
 14h         2.3+    Contained
								record that follow, in the range 0 to 255. If no Contained
		     Element Record
								Elements are included, this field is set to 0. For v2.3.2
		     Length (m)
								and later of this specification, this field is set to at least
								03h when Contained Elements are specified.
 15h         2.3+    Contained             n*m       Varies     Identifies the elements, possibly defined by other
		     Elements             BYTEs                 SMBIOS structures, present in this chassis. See 3.3.4.4
								for definitions.
#endif
}
/*****************************************************************************
*****************************************************************************/
static void dump_cpu(uint8_t *buf)
{
	static const char *type[] =
	{
		"UNDEFINED", "Other", "Unknown", "Central Processor",
		"Math Processor", "DSP Processor", "Video Processor"
	};
	static const char *family[] =
	{
		"UNDEFINED", "Other", "Unknown", "8086",
		"80286", "Intel386TM processor", "Intel486TM processor",
			"8087",
		"80287", "80387", "80487", "Pentium® processor Family",
		"Pentium® Pro processor", "Pentium® II processor",
			"Pentium® processor with MMXTM technology",
			"CeleronTM processor",
		"Pentium® II XeonTM processor", "Pentium® III processor",
			"M1 Family", "M2 Family",
/* 0x14-0x17 */	"Unknown", "Unknown", "Unknown", "Unknown",
		"AMD DuronTM Processor Family", "K5 Family", "K6 Family",
			"K6-2",
		"K6-3", "AMD AthlonTM Processor Family", "AMD2900 Family",
			"K6-2+",
		"Power PC Family", "Power PC 601", "Power PC 603",
			"Power PC 603+",
		"Power PC 604", "Power PC 620", "Power PC x704",
			"Power PC 750",
/* 0x28-0x2F */	"Unknown", "Unknown", "Unknown", "Unknown",
		"Unknown", "Unknown", "Unknown", "Unknown",
		"Alpha Family3", "Alpha 21064", "Alpha 21066", "Alpha 21164",
		"Alpha 21164PC", "Alpha 21164a", "Alpha 21264", "Alpha 21364",
/* 0x38-0x3F */	"Unknown", "Unknown", "Unknown", "Unknown",
		"Unknown", "Unknown", "Unknown", "Unknown",
		"MIPS Family", "MIPS R4000", "MIPS R4200", "MIPS R4400",
		"MIPS R4600", "MIPS R10000",
/* 0x46-0x4F */		"Unknown", "Unknown",
		"Unknown", "Unknown", "Unknown", "Unknown",
		"Unknown", "Unknown", "Unknown", "Unknown",
		"SPARC Family", "SuperSPARC", "microSPARC II",
			"microSPARC IIep",
		"UltraSPARC", "UltraSPARC II", "UltraSPARC IIi",
			"UltraSPARC III",
		"UltraSPARC IIIi",
/* 0x59-0x5F */
			"Unknown", "Unknown", "Unknown",
		"Unknown", "Unknown", "Unknown", "Unknown",
		"68040 Family", "68xxx", "68000", "68010",
		"68020", "68030",
/* 0x66-0x6F */
			"Unknown", "Unknown",
		"Unknown", "Unknown", "Unknown", "Unknown",
		"Hobbit Family",
/* 0x71-0x77 */
		"Unknown", "Unknown", "Unknown",
		"Unknown", "Unknown", "Unknown", "Unknown",
		"CrusoeTM TM5000 Family", "CrusoeTM TM3000 Family",
			"EfficeonTM TM8000 Family",
/* 0x7B-0x7F */
			"Unknown",
		"Unknown", "Unknown", "Unknown", "Unknown",
		"Weitek",
/* 0x81 */		"Unknown",

			"ItaniumTM processor",
			"AMD AthlonTM 64 Processor Family",
		"AMD OpteronTM Processor Family",
			"AMD SempronTM Processor Family",
			"AMD TurionTM 64 Mobile Technology",
			"Dual-Core AMD OpteronTM Processor Family",
		"AMD AthlonTM 64 X2 Dual-Core Processor Family",
/* 0x89-0x8F */
			"Unknown", "Unknown", "Unknown",
		"Unknown", "Unknown", "Unknown", "Unknown",
		"PA-RISC Family", "PA-RISC 8500", "PA-RISC 8000",
			"PA-RISC 7300LC",
		"PA-RISC 7200", "PA-RISC 7100LC", "PA-RISC 7100",
/* 0x97-0x9F */
			"Unknown",
		"Unknown", "Unknown", "Unknown", "Unknown",
		"Unknown", "Unknown", "Unknown", "Unknown",
		"V30 Family",
/* 0xA1-0xAF */
			"Unknown", "Unknown", "Unknown",
		"Unknown", "Unknown", "Unknown", "Unknown",
		"Unknown", "Unknown", "Unknown", "Unknown",
		"Unknown", "Unknown", "Unknown", "Unknown",
		"Pentium® III XeonTM processor",
			"Pentium® III Processor with Intel ® SpeedStepTM Technology",
			"Pentium® 4 Processor",
			"Intel® XeonTM",
		"AS400 Family", "Intel® XeonTM processor MP",
			"AMD AthlonTM XP Processor Family",
			"AMD AthlonTM MP Processor Family",
		"Intel® Itanium® 2 processor", "Intel® Pentium® M processor",
			"Intel® Celeron® D processor",
			"Intel® Pentium® D processor",
		"Intel® Pentium® Processor Extreme Edition",
/* 0xBD-0xC7 */
			"Unknown", "Unknown", "Unknown",
		"Unknown", "Unknown", "Unknown", "Unknown",
		"Unknown", "Unknown", "Unknown", "Unknown",
		"IBM390 Family", "G4", "G5"
	};
	static const char *status[] =
	{
		"Unknown", "Enabled", "Disabled in BIOS by user",
			"Disabled in BIOS by POST error",
		"Idle", "Reserved", "Reserved", "Reserved"
	};
	static const char *upgrade[] =
	{
		"UNDEFINED", "Other", "Unknown", "Daughter Board",
		"ZIF Socket", "Replaceable Piggy Back", "None", "LIF Socket",
		"Slot 1", "Slot 2", "370-pin socket", "Slot A",
		"Slot M", "Socket 423", "Socket A (Socket 462)", "Socket 478",
		"Socket 754", "Socket 940", "Socket 939", "Socket mPGA604",
		"Socket LGA771", "Socket LGA775"
	};
/**/
	unsigned hdr_len;

	hdr_len = buf[1];
	printf("*** processor (CPU) ***\n");
	display_string(buf, 4, "Socket");
	if(hdr_len > 5)
	{
		printf("Processor type: ");
		if(buf[5] < sizeof(type) / sizeof(type[0]))
			printf("%s\n", type[buf[5]]);
		else
			printf("unknown\n");
	}
	if(hdr_len > 6)
	{
		printf("Processor family: ");
		if(buf[6] < sizeof(family) / sizeof(family[0]))
			printf("%s\n", family[buf[6]]);
		else
			printf("unknown\n");
	}
	display_string(buf, 7, "Manufacturer");
	if(hdr_len >= 16)
	{
		printf("ID:\n");
		dump(&buf[8], 8);
	}
	display_string(buf, 16, "Version");
	if(hdr_len >= 17)
	{
		printf("Voltage:");
		if(buf[17] & 0x80)
			printf(" %4.2f\n", (buf[17] & 0x7F) / 10.0);
		else
		{
			if(buf[17] & 0x04)
				printf(" 2.9V");
			if(buf[17] & 0x02)
				printf(" 3.3V");
			if(buf[17] & 0x01)
				printf(" 5.0V");
			putchar('\n');
		}
	}
	if(hdr_len >= 20)
	{
		printf("External clock frequency: %u MHz\n",
			read_le16(&buf[18]));
	}
	if(hdr_len >= 22)
	{
		printf("Max. clock frequency: %u MHz\n",
			read_le16(&buf[20]));
	}
	if(hdr_len >= 24)
	{
		printf("Current. clock frequency: %u MHz\n",
			read_le16(&buf[22]));
	}
	if(hdr_len > 24)
	{
		if(buf[24] & 0x40)
			printf("Socket is populated (CPU present)\n");
		printf("CPU status: %s\n", status[buf[24] & 7]);
	}
	if(hdr_len > 25)
	{
		printf("Upgrade: ");
		if(buf[25] < sizeof(upgrade) / sizeof(upgrade[0]))
			printf("%s\n", upgrade[buf[25]]);
		else
			printf("unknown\n");
	}
	if(hdr_len >= 28)
		printf("L1 cache handle=0x%04X\n", read_le16(&buf[26]));
	if(hdr_len >= 30)
		printf("L2 cache handle=0x%04X\n", read_le16(&buf[28]));
	if(hdr_len >= 32)
		printf("L3 cache handle=0x%04X\n", read_le16(&buf[30]));
	display_string(buf, 32, "Serial Number");
	display_string(buf, 33, "Asset Tag");
	display_string(buf, 32, "Part Number");
	if(hdr_len >= 35)
		printf("%u cores\n", buf[35]);
	if(hdr_len >= 36)
		printf("%u enabled cores\n", buf[36]);
	if(hdr_len >= 37)
		printf("%u threads\n", buf[37]);
	if(hdr_len >= 38)
	{
		if(buf[38] & 0x04)
			printf("64-bit-capable\n");
	}
}
/*****************************************************************************
*****************************************************************************/
static void dump_mem(uint8_t *buf)
{
	static const char *type[] =
	{
		"Other", "Unknown", "Standard", "Fast Page Mode",
		"EDO", "Parity", "ECC", "SIMM",
		"DIMM", "Burst EDO", "SDRAM"
	};
/**/
	unsigned hdr_len, i, j, k;

	hdr_len = buf[1];
	printf("*** memory module ***\n");
	display_string(buf, 4, "Socket");
	if(hdr_len > 5)
	{
		printf("Connected bank(s):");
		if((buf[5] & 0xF0) != 0xF0)
			printf(" %u", buf[5] >> 4);
		if((buf[5] & 0x0F) != 0x0F)
			printf(" %u", buf[5] & 0x0F);
		putchar('\n');
	}
	if(hdr_len > 6)
	{
		if(buf[6] == 0)
			printf("Speed: unknown\n");
		else
			printf("Speed: %u ns\n", buf[6]);
	}
	if(hdr_len >= 9)
	{
		printf("Memory type:");
		i = read_le16(&buf[7]);
		j = 0x0001;
		for(k = 0; k < sizeof(type) / sizeof(type[0]); k++)
		{
			if(i & j)
				printf(" %s", type[k]);
			j <<= 1;
		}
		putchar('\n');
	}
	if(hdr_len > 10)
	{
		printf("Installed size: ");
		i = buf[9] & 0x7F;
		if(i == 0x7F)
			printf("not installed, ");
		else if(i == 0x7E)
			printf("not enabled, ");
		else if(i == 0x7D)
			printf("can not be determined, ");
		else
			printf("%u Mbytes, ", 1 << i);
		printf("%s-bank connection\n", (buf[9] & 0x80) ?
			"double" : "single");
	}
	if(hdr_len > 11)
	{
		printf("Enabled size: ");
		i = buf[10] & 0x7F;
		if(i == 0x7F)
			printf("not installed, ");
		else if(i == 0x7E)
			printf("not enabled, ");
		else if(i == 0x7D)
			printf("can not be determined, ");
		else
			printf("%u Mbytes, ", 1 << i);
		printf("%s-bank connection\n", (buf[10] & 0x80) ?
			"double" : "single");
	}
	if(hdr_len >= 11)
	{
		if(buf[11] & 0x04)
			printf("Error status in event log\n");
		else
		{
			if(buf[11] & 0x02)
				printf("Module had correctable errors\n");
			if(buf[11] & 0x01)
				printf("Module had uncorrectable errors\n");
		}
	}
}
/*****************************************************************************
*****************************************************************************/
static void dump_cache(uint8_t *buf)
{
	static const char *mode[] =
	{
		"write-through", "write-back", "address-dependent",
			"unknown"
	};
	static const char *location[] =
	{
		"internal (L1)", "external (L2)", "reserved", "unknown"
	};
	static const char *type[] =
	{
		"other", "unknown", "non-burst", "burst",
		"pipeline burst", "synchronous", "asynchronous"
	};
	static const char *ecc[] =
	{
		"UNDEFINED", "other", "unknown", "none",
		"parity", "single-bit ECC", "multi-bit ECC"
	};
	static const char *use[] =
	{
		"UNDEFINED", "other", "unknown", "instruction",
		"data", "unified"
	};
	static const char *assoc[] =
	{
		"UNDEFINED", "other", "unknown", "direct-mapped",
		"2-way set-associative", "4-way set-associative",
			"fully associative", "8-way set-associative",
		"16-way set-associative"
	};
/**/
	unsigned hdr_len, i, j, k;

	hdr_len = buf[1];
	printf("*** cache ***\n");
	display_string(buf, 4, "Socket");
	if(hdr_len >= 7)
	{
		printf("Mode: %s, ", mode[buf[6] & 0x03]);
		printf("%s, ", (buf[5] & 0x80) ? "enabled" : "disabled");
		printf("location: %s, ", location[(buf[5] >> 5) & 3]);
		if((buf[5] & 0x08) == 0)
			printf("not ");
		printf("socketed, ");
		printf("level %u\n", buf[5] & 7);
	}
	if(hdr_len >= 9)
	{
		i = (buf[7] & 0x80) ? 64 : 1;
		printf("Max cache size: %u K", (buf[7] & 0x7FFF) * i);
		printf(" (%u K granule size)\n", i);
	}
	if(hdr_len >= 11)
	{
		i = (buf[9] & 0x80) ? 64 : 1;
		printf("Installed cache size: %u K", (buf[9] & 0x7FFF) * i);
		printf(" (%u K granule size)\n", i);
	}
	if(hdr_len >= 13)
	{
		printf("Supported SRAM type:");
		i = read_le16(&buf[11]);
		j = 0x0001;
		for(k = 0; k < sizeof(type) / sizeof(type[0]); k++)
		{
			if(i & j)
				printf(" %s", type[k]);
			j <<= 1;
		}
		putchar('\n');
	}
	if(hdr_len >= 15)
	{
		printf("Current SRAM type:");
		i = read_le16(&buf[13]);
		j = 0x0001;
		for(k = 0; k < sizeof(type) / sizeof(type[0]); k++)
		{
			if(i & j)
				printf(" %s", type[k]);
			j <<= 1;
		}
		putchar('\n');
	}
	if(hdr_len >= 15)
	{
		if(buf[15] == 0)
			printf("Speed: unknown\n");
		else
			printf("Speed: %u ns\n", buf[15]);
	}
	if(hdr_len > 16)
	{
		printf("Error-correction:");
		j = 0x0001;
		for(k = 0; k < sizeof(ecc) / sizeof(ecc[0]); k++)
		{
			if(buf[16] & j)
				printf(" %s", ecc[k]);
			j <<= 1;
		}
		putchar('\n');
	}
	if(hdr_len > 17)
	{
		printf("Cache type:");
		j = 0x0001;
		for(k = 0; k < sizeof(use) / sizeof(use[0]); k++)
		{
			if(buf[17] & j)
				printf(" %s", use[k]);
			j <<= 1;
		}
		putchar('\n');
	}
	if(hdr_len > 18)
	{
		printf("Associativity:");
		j = 0x0001;
		for(k = 0; k < sizeof(assoc) / sizeof(assoc[0]); k++)
		{
			if(buf[18] & j)
				printf(" %s", assoc[k]);
			j <<= 1;
		}
		putchar('\n');
	}
}
/*****************************************************************************
*****************************************************************************/
static void dump_port(uint8_t *buf)
{
	static const char *conn[] =
	{
		"None", "Centronics", "Mini Centronics", "Proprietary",
		"DB-25 pin male", "DB-25 pin female", "DB-15 pin male",
			"DB-15 pin female",
		"DB-9 pin male", "DB-9 pin female", "RJ-11", "RJ-45",
		"50 Pin MiniSCSI", "Mini-DIN", "Micro-DIN", "PS/2",
		"Infrared", "HP-HIL", "Access Bus (USB)", "SSA SCSI",
		"Circular DIN-8 male", "Circular DIN-8 female",
			"On Board IDE", "On Board Floppy",
		"9 Pin Dual Inline (pin 10 cut)",
			"25 Pin Dual Inline (pin 26 cut)",
			"50 Pin Dual Inline", "68 Pin Dual Inline",
		"On Board Sound Input from CD-ROM",
			"Mini-Centronics Type-14",
			"Mini-Centronics Type-26", "Mini-jack (headphones)",
		"BNC", "1394", "SAS/SATA Plug Receptacle"
	};
	static const char *type[] =
	{
		"None", "Parallel Port XT/AT Compatible",
			"Parallel Port PS/2", "Parallel Port ECP",
		"Parallel Port EPP", "Parallel Port ECP/EPP",
			"Serial Port XT/AT Compatible",
			"Serial Port 16450 Compatible",
		"Serial Port 16550 Compatible",
			"Serial Port 16550A Compatible",
			"SCSI Port", "MIDI Port",
		"Joy Stick Port", "Keyboard Port", "Mouse Port", "SSA SCSI",
		"USB", "FireWire (IEEE P1394)", "PCMCIA Type II",
			"PCMCIA Type II",
		"PCMCIA Type III", "Cardbus", "Access Bus Port", "SCSI II",
		"SCSI Wide", "PC-98", "PC-98-Hireso", "PC-H98",
		"Video Port", "Audio Port", "Modem Port", "Network Port",
		"SATA", "SAS"
	};
/**/
	unsigned hdr_len;

	hdr_len = buf[1];
	printf("*** port ***\n");
	display_string(buf, 4, "Internal reference designator");
	if(hdr_len > 5)
	{
		printf("Internal connector type: ");
		if(buf[5] < sizeof(conn) / sizeof(conn[0]))
			printf("%s\n", conn[buf[5]]);
		else
			printf("unknown\n");
	}
	display_string(buf, 6, "External reference designator");
	if(hdr_len > 7)
	{
		printf("External connector type: ");
		if(buf[7] < sizeof(conn) / sizeof(conn[0]))
			printf("%s\n", conn[buf[7]]);
		else
			printf("unknown\n");
	}
	if(hdr_len > 8)
	{
		printf("Port type: ");
		if(buf[8] == 0xA0)
			printf("8251-compatible serial\n");
		else if(buf[8] == 0xA1)
			printf("8251-compatible serial with FIFO\n");
		else if(buf[8] < sizeof(type) / sizeof(type[0]))
			printf("%s\n", type[buf[8]]);
		else
			printf("unknown\n");
	}
}
/*****************************************************************************
*****************************************************************************/
static void dump_slot(uint8_t *buf)
{
	static const char *type1[] =
	{
		"UNDEFINED", "Other", "Unknown", "ISA",
		"MCA", "EISA", "PCI", "PC Card (PCMCIA)",
		"VL-VESA", "Proprietary", "Processor Card Slot",
			"Proprietary Memory Card Slot",
		"I/O Riser Card Slot", "NuBus", "PCI - 66MHz Capable", "AGP",
		"AGP 2X", "AGP 4X", "PCI-X", "AGP 8X"
	};
/* these start with 0xA0 */
	static const char *type2[] =
	{
		"PC-98/C20", "PC-98/C24", "PC-98/E", "PC-98/Local Bus",
		"PC-98/Card", "PCI Express"
	};
	static const char *width[] =
	{
		"UNDEFINED", "Other", "Unknown", "8 bit",
		"16 bit", "32 bit", "64 bit", "128 bit",
		"1x or x1", "2x or x2", "4x or x4", "8x or x8",
		"12x or x12", "16x or x16", "32x or x32"
	};
	static const char *usage[] =
	{
		"UNDEFINED", "other", "unknown", "available", "in use"
	};
	static const char *length[] =
	{
		"UNDEFINED", "other", "unknown", "short", "long"
	};
/**/
	unsigned hdr_len;

	hdr_len = buf[1];
	printf("*** slot ***\n");
	display_string(buf, 4, "Designation");
	if(hdr_len > 5)
	{
		printf("Type: ");
		if(buf[5] < sizeof(type1) / sizeof(type1[0]))
			printf("%s\n", type1[buf[5]]);
		else if(buf[5] >= 0xA0 &&
			buf[5] < 0xA0 + sizeof(type2) / sizeof(type2[0]))
				printf("%s\n", type2[buf[5] - 0xA0]);
		else
			printf("unknown\n");
	}
	if(hdr_len > 6)
	{
		printf("Width: ");
		if(buf[6] < sizeof(width) / sizeof(width[0]))
			printf("%s\n", width[buf[6]]);
		else
			printf("unknown\n");
	}
	if(hdr_len > 7)
	{
		printf("Usage: ");
		if(buf[7] < sizeof(usage) / sizeof(usage[0]))
			printf("%s\n", usage[buf[7]]);
		else
			printf("unknown\n");
	}
	if(hdr_len > 8)
	{
		printf("Length: ");
		if(buf[8] < sizeof(length) / sizeof(length[0]))
			printf("%s\n", length[buf[8]]);
		else
			printf("unknown\n");
	}
	if(hdr_len > 9)
		printf("Slot ID=%u\n", buf[9]);

// buf[10]
//     Bit 0        Characteristics Unknown
//     Bit 1        Provides 5.0 Volts
//     Bit 2        Provides 3.3 Volts
//     Bit 3        Slot's opening is shared with another slot, e.g. PCI/EISA shared slot.
//     Bit 4        PC Card slot supports PC Card-16
//     Bit 5        PC Card slot supports CardBus
//     Bit 6        PC Card slot supports Zoom Video
//     Bit 7        PC Card slot supports Modem Ring Resume
}
/*****************************************************************************
*****************************************************************************/
int main(void)
{
//	static const char *foo[] =
//	{
//		"BIOS", "PC", "motherboard", "case",
//		"CPU", "memory controller", "memory module", "cache",
//		"ports", "slots", "on-board devices"
//	};
/**/
	uint16_t fn, dmi_count, dmi_largest, dmi_size, j;
	uint8_t rev, buf[128];
	uint32_t dmi_adr;
	int i;

#if 0
/* my system is too old -- this doesn't work. The PnP interface to
SMBIOS is deprecated, so there are probably systems where this code
works, and the PnP code below does not. */
unsigned long linear;
char far *foo;
for(linear = 0xF0000L; linear < 0xFFFFFL; linear += 16)
{
 foo = MK_FP(linear >> 4, linear & 0x0F);
 if(foo[0] == '_' && foo[1] == 'S' && foo[2] == 'M' && foo[3] == '_')
 {
  printf("_SM_ signature found at linear address 0x%lX\n", linear);
  return 0;
 }
}
printf("_SM_ signature not found\n");
return 1;
#endif
/* detect 16-bit PnP BIOS */
	if(pnp_detect())
		return 1;
/* get DMI info */
	fn = 0x50;
	i = g_pnp_entry(fn, (char far *)&rev, (uint16_t far *)&dmi_count,
		(uint16_t far *)&dmi_largest, (uint32_t far *)&dmi_adr,
		(uint16_t far *)&dmi_size, g_pnp_ds);
	if(i != 0)
ERR:	{
		printf("PnP BIOS call 0x%02X returned 0x%02X\n", fn, i);
		return 1;
	}
	printf("DMI ver %u.%u: %u DMI structures; largest is %u bytes\n",
		rev >> 4, rev & 15, dmi_count, dmi_largest);
	printf("%u bytes total storage at address 0x%lX\n",
		dmi_size, dmi_adr);
/* get DMI struct */
	fn = 0x51;
	j = 0;
	do
	{
		i = g_pnp_entry(fn, (uint16_t far *)&j, (uint8_t far *)buf,
			_DS, g_pnp_ds);
		if(i != 0)
			goto ERR;
/* buf[0]=structure type, buf[1]=length of structure header,
read_le16(&buf[2])=structure 'handle'. Length excludes strings that
follow structure and xxx */
		putchar('\n');
//		printf("--Type=%u, size=%2u bytes, handle=0x%X\n",
//			buf[0], buf[1], read_le16(&buf[2]));
//		if(buf[0] < sizeof(foo) / sizeof(foo[0]))
//			printf("; info about %s", foo[buf[0]]);
//		putchar('\n');
//		dump(buf, buf[1]);
//		dump(buf, 64);

if(buf[0] == 0) dump_bios(buf);
if(buf[0] == 1) dump_system(buf);
if(buf[0] == 2) dump_board(buf);
if(buf[0] == 3) dump_chassis(buf);
if(buf[0] == 4) dump_cpu(buf);
if(buf[0] == 6) dump_mem(buf);
if(buf[0] == 7) dump_cache(buf);
if(buf[0] == 8) dump_port(buf);
if(buf[0] == 9) dump_slot(buf);
	} while(j != 0xFFFF);

	return 0;
}
