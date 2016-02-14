#include <kernel/types.h>
#include <kernel/vm.h>
#include <kernel/i386/i386.h>
#include <kernel/dbg.h>
#include <kernel/utility.h>
#include <kernel/proc.h>
#include <kernel/fs.h>
#include <kernel/kmalloc.h>
#include <kernel/config.h>
#include "v86.h"



struct BMPHeader
{
    char bfType[2];       /* "BM" */
    int bfSize;           /* Size of file in bytes */
    int bfReserved;       /* set to 0 */
    int bfOffBits;        /* Byte offset to actual bitmap data (= 54) */
    int biSize;           /* Size of BITMAPINFOHEADER, in bytes (= 40) */
    int biWidth;          /* Width of image, in pixels */
    int biHeight;         /* Height of images, in pixels */
    short biPlanes;       /* Number of planes in target device (set to 1) */
    short biBitCount;     /* Bits per pixel (24 in this case) */
    int biCompression;    /* Type of compression (0 if no compression) */
    int biSizeImage;      /* Image size, in bytes (0 if no compression) */
    int biXPelsPerMeter;  /* Resolution in pixels/meter of display device */
    int biYPelsPerMeter;  /* Resolution in pixels/meter of display device */
    int biClrUsed;        /* Number of colors in the color table (if 0, use 
                             maximum allowed by biBitCount) */
    int biClrImportant;   /* Number of important colors.  If 0, all colors 
                             are important */
} __attribute__ ((__packed__));
	
void ReadBMP (void);
void SwitchToGraphicsMode (void);
void SwitchToTextMode (void);
void Wave (uint8 *bitmap);



/*
 * Call TestV86() in the init/init.c code to display a fixed size bitmap.
 */

void TestV86 (void)
{
	ReadBMP();
}
 





void DrawToScreen (void)
{
	int x, y;
	uint8 *gfx = (uint8 *)0xa0000;


	for (y=0; y<200; y++)
	{
		for (x=0; x<320; x++)
			*(gfx + y*320 + x) = x % 256;
	}
}



void ReadBMP (void)
{
	int fd;
	struct BMPHeader bmp_hdr;
	uint8 *bmp_data;
	
	uint8 *src;
	uint8 *gfx;
	int c,x, y;
					uint8 r,g,b;
	
	uint32 palette[256];
	
	KPRINTF ("ReadBMP()");
	
	
	if ((fd = Open ("/sys/home/pic.bmp", 0, O_RDONLY)) != -1)
	{
		KPRINTF ("Bitmap opened!");
	
		if ((bmp_data = KMalloc (320 * 240)) != NULL)
		{
			KPRINTF ("Bitmap allocated!");
		
			Read (fd, &bmp_hdr, sizeof (struct BMPHeader));
			
			KPRINTF ("bftype = %c%c", bmp_hdr.bfType[0], bmp_hdr.bfType[1]);
			KPRINTF ("bfOffBits = %d", bmp_hdr.bfOffBits);

			Seek (fd, 54, SEEK_SET);
			Read (fd, &palette, 256*4);
			
			Seek (fd, bmp_hdr.bfOffBits, SEEK_SET);
			Read (fd, bmp_data, (320 * 240));
		
			SwitchToGraphicsMode();


			OutByte (0x03c8, 0);
			for (c=0; c<256;c++)
			{
				r = (palette[c] >>16) & 0xff;
				g = (palette[c] >>8) & 0xff;
				b = (palette[c] >>0) & 0xff;
				
				OutByte (0x03c9, r/4);
				OutByte (0x03c9, g/4);	
				OutByte (0x03c9, b/4);
			}
			
		
			src = bmp_data;
			gfx = (uint8 *)0xa0000;
		
			for (y = 199; y>=0; y--)
			{
				for (x = 0; x < 320; x++)
				{
					*(gfx + x + y * 320) = *(src + x + (199 - y)*320);
				}		
			}
			
			
			KSleep2(5,0);
			
			SwitchToTextMode();
			
			
			KFree (bmp_data);
		}
		else
		{
			KPANIC ("Bitmap malloc failed!");
		}
		
		Close (fd);
	}
	else
	{
		KPANIC ("Bitmap open failed!");
	}
}





void SwitchToGraphicsMode (void)
{
	struct ContextState vs;

	vs.eax = 19;
	vs.ebx = 0;
	vs.ecx = 0;
	vs.edx = 0;
	vs.esi = 0;    /* Has 3 */
	vs.edi = 0;    /* 4 */
	vs.ebp = 0;    /* Ebp = 5 */
	vs.gs = 0;
	vs.fs = 0;
	vs.es = 0;
	vs.ds = 0;
	
	V86BiosCall (0x10, &vs);
}	
	



void SwitchToTextMode(void)
{
	struct ContextState vs;

	
	vs.eax = 2;
	vs.ebx = 0;
	vs.ecx = 0;
	vs.edx = 0;
	vs.esi = 0;
	vs.edi = 0;
	vs.ebp = 0;
	vs.gs = 0;
	vs.fs = 0;
	vs.es = 0;
	vs.ds = 0;
	
	V86BiosCall (0x10, &vs);

}


// C 8-bit Sine Table
const unsigned char sinetable[256] = {
        128,131,134,137,140,143,146,149,152,156,159,162,165,168,171,174,
        176,179,182,185,188,191,193,196,199,201,204,206,209,211,213,216,
        218,220,222,224,226,228,230,232,234,236,237,239,240,242,243,245,
        246,247,248,249,250,251,252,252,253,254,254,255,255,255,255,255,
        255,255,255,255,255,255,254,254,253,252,252,251,250,249,248,247,
        246,245,243,242,240,239,237,236,234,232,230,228,226,224,222,220,
        218,216,213,211,209,206,204,201,199,196,193,191,188,185,182,179,
        176,174,171,168,165,162,159,156,152,149,146,143,140,137,134,131,
        128,124,121,118,115,112,109,106,103,99, 96, 93, 90, 87, 84, 81, 
        79, 76, 73, 70, 67, 64, 62, 59, 56, 54, 51, 49, 46, 44, 42, 39, 
        37, 35, 33, 31, 29, 27, 25, 23, 21, 19, 18, 16, 15, 13, 12, 10, 
        9,  8,  7,  6,  5,  4,  3,  3,  2,  1,  1,  0,  0,  0,  0,  0,  
        0,  0,  0,  0,  0,  0,  1,  1,  2,  3,  3,  4,  5,  6,  7,  8,  
        9,  10, 12, 13, 15, 16, 18, 19, 21, 23, 25, 27, 29, 31, 33, 35, 
        37, 39, 42, 44, 46, 49, 51, 54, 56, 59, 62, 64, 67, 70, 73, 76, 
        79, 81, 84, 87, 90, 93, 96, 99, 103,106,109,112,115,118,121,124
};


void Wave (uint8 *bitmap)
{
	uint8 *src, *gfx;
	uint32 s, x, y, src_x, src_y;

	return;
	
	src = bitmap;
	gfx = (uint8 *)0xa0000;
		
	while (1)
	{
		for (s = 0; s<256; s+=8)
		{		
			for (y = 199; y>=0; y--)
			{
				for (x = 0; x < 320; x++)
				{
					src_x = ((x + sinetable[((y)+256-s)%256]/4))%320;
					src_y = ((y + sinetable[((x)+256-s)%256]/4)-64)%200;
				
				
					*(gfx + x + y * 320) = *(src + src_x + (199 - src_y) * 320);
					
				}		
			}
		}
	}
}



