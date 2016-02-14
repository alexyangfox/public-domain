/* libmaglo : MAG image format loader library  */
/*      This is a public domain software.      */
/* 2008 TOMARI, Hisanobu                       */

/* stdlib */
#define MAG_MAXSIZE 16*1024*1024 /* bytes */
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

/* system */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

/* local */
#include "maglo.h"

/* prototype for local functons */
static uint16_t MAGshortOfHeaderField(MagImage *mag, unsigned long pos);
static uint32_t MAGlongOfHeaderField(MagImage *mag, unsigned long pos);
static unsigned long MAGAFlagsOffset(MagImage *mag);
static unsigned long MAGBFlagsOffset(MagImage *mag);
static unsigned long MAGAFlagsLength(MagImage *mag);
static unsigned long MAGBFlagsLength(MagImage *mag);
static unsigned long MAGpixelsOffset(MagImage *mag);
static unsigned long MAGpixelsLength(MagImage *mag);
static int MAGoffsetIsInRange(MagImage *mag,long offset);
static int MAGnumPixelsInColumn(MagImage *mag);
static int MAGReaderNextFlag(MagReader *rd,int y,unsigned char* flagp);

/**
 * MAGversion - returns library interface version
 */
extern int MAGversion()
{
  return 99;
}

/**
 * MAGopen - open a MAG format image
 *  call MAGclose after use
 */
extern int MAGopen(MagImage *mag,const char *path)
{
  int fd;
  FILE *input;
  char buf[8];
  struct stat st;

  if((fd=open(path,O_RDONLY))<0)
    return MAGERR_IO_ERROR;
  if((input=fopen(path,"rb"))==NULL)
    { close(fd); return MAGERR_IO_ERROR; }
  /* check header */
  if(fread(&buf,8,1,input)<1)
    { fclose(input);  return MAGERR_IO_ERROR; }
  if(memcmp(&buf,"MAKI02  ",8)!=0)
    { fclose(input); return MAGERR_MAGIC_MISMATCH; }
  if(fseek(input,4,SEEK_CUR)<0) /* skip machine id */
    { fclose(input); return MAGERR_IO_ERROR; }
  if(fseek(input,18,SEEK_CUR)<0) /* skip user */
    { fclose(input); return MAGERR_IO_ERROR; }
  /* skip header */
  while(!feof(input))
    if(fgetc(input)==0x1a)
      goto header;
  fclose(input);
  return MAGERR_IO_ERROR;
 header:
  if((mag->head_off=ftello(input))==-1) /* set offset to header */
    { fclose(input); return MAGERR_IO_ERROR; }
  /* get size of file for mmap */
  if(fstat(fd,&st)<0)
    { fclose(input); return MAGERR_IO_ERROR; }
  mag->len=st.st_size;
  if(mag->len<(mag->head_off+32)||
     mag->len>MAG_MAXSIZE) /* short header or too large a file */
    { fclose(input); return MAGERR_DATA_PARTIAL; }
  /* memory map input image */
  if((mag->body=mmap(NULL,mag->len,PROT_READ,MAP_PRIVATE,fd,0))==NULL)
    { fclose(input); return MAGERR_MEM_ERROR; }
  fclose(input);
  return MAGERR_NOERROR;
}

/**
 * MAGclose - close MagImage object opened with MAGopen function
 */
extern int MAGclose(MagImage *mag)
{
  /* unmap mapped MAG file */
  if(munmap(mag->body,mag->len)<0)
    return MAGERR_MEM_ERROR;
  return MAGERR_NOERROR;
}

/**
 * MAGcommentSize - returns the length of comment embedded in the mag file
 *  in bytes
 */
extern long MAGcommentSize(MagImage *mag)
{
  return mag->head_off-32;
}

/**
 * MAGcomment - returns pointer to _NOT_NULL_TERMINATED_ comment string.
 *  use MAGcommentSize function to obtain size
 */
extern char *MAGcomment(MagImage *mag)
{
  return &(((char *) mag->body)[32]);
}

/**
 * MAGmacType - returns pointer to 4-byte human-readable type of machine 
 *  on which the file was created (example: PC98,PC88,ESEQ,X68K,MSX2,...)
 */
extern char *MAGmacType(MagImage *mag)
{
  return &(((char *) mag->body)[8]);
}

/**
 * MAGuserName - returns pointer to 18-byte name of the artist
 *  the string is not null-terminated
 */
extern char *MAGuserName(MagImage *mag)
{
  return &(((char *) mag->body)[12]);
}

/**
 * MAGmacCode - returns the id of machine on which the MAG file was created
 *  return value:
 *   00h: PC-98/X68000/...
 *   03h: MSX
 *   88h: PC-88
 *   68h: PST68
 *   FFh: ??
 */
extern unsigned char MAGmacCode(MagImage *mag)
{
  return ((unsigned char *)mag->body)[mag->head_off+1];
}

/** machine dependent flags */
extern unsigned char MAGmacFlags(MagImage *mag)
{
  return ((unsigned char *)mag->body)[mag->head_off+2];
}

/** Screen mode(packed in a byte) - use accessor methods instead */
extern unsigned char MAGscrModeCode(MagImage *mag)
{
  return ((unsigned char *)mag->body)[mag->head_off+3];
}

/** true if dot aspect=1:2 */
extern int MAGscrMode200Lines(MagImage *mag)
{
  return (MAGscrModeCode(mag)&0x01)>0;
}

/**
 * MAGscrModeNumColors - returns number of colors for the image
 *  return value:
 *   256 - 256 colors
 *    16 - 16 colors
 *     8 - 8 colors
 */
extern int MAGscrModeNumColors(MagImage *mag)
{
  unsigned char scrMode=MAGscrModeCode(mag);
  if(scrMode&0x80)
    return 256;
  else if(scrMode&0x02)
    return 8;
  else
    return 16;
}

/**
 * MAGpixelWidth - returns row width in dots
 *  4 for 16/8 color image, 2 for 256 color image
 */
extern int MAGpixelWidth(MagImage *mag)
{
  int colors=MAGscrModeNumColors(mag);
  if(colors==256)
    return 2;
  else if(colors==16 || colors==8)
    return 4;
  else
    return -1; /* error */
}

/** 
 * MAGscrModeIsDigital - true if color palette is old PC-88/PC-98 style.
 *  this flag was used for skipping pallete approximation on older hardware,
 *  and serves no need today
 */
extern int MAGscrModeIsDigital(MagImage *mag)
{
  return (MAGscrModeCode(mag)&0x04)>0;
}

/**
 * MAGshortOfHeaderField - returns header field of 16-bit type at [pos] bytes
 *  from the beginning of the heaeer
 */
static uint16_t MAGshortOfHeaderField(MagImage *mag, unsigned long pos)
{
  unsigned long pos_from_start=mag->head_off+pos;
  unsigned char *data=&(((unsigned char *)mag->body)[pos_from_start]);
  return (data[1]<<8)|data[0];
}

/**
 * MAGlongOfHeaderField - returns 32-bit header field value at [pos] bytes
 *  from the beginning of the header
 */
static uint32_t MAGlongOfHeaderField(MagImage *mag, unsigned long pos)
{
  unsigned long pos_from_start=mag->head_off+pos;
  unsigned char *data=&(((unsigned char *)mag->body)[pos_from_start]);
  return (data[3]<<24)|(data[2]<<16)|(data[1]<<8)|data[0];
}

/** MAGimageStartX - returns x coordinate of start position */
extern int MAGimageStartX(MagImage *mag)
{
  return MAGshortOfHeaderField(mag,4);
}

/** MAGimageStartY - returns y coordinate of start position */
extern int MAGimageStartY(MagImage *mag)
{
  return MAGshortOfHeaderField(mag,6);
}

/** MAGimageEndX - returns x coordinate of end position */
extern int MAGimageEndX(MagImage *mag)
{
  return MAGshortOfHeaderField(mag,8);
}

/** MAGimageEndY - returns y coordinate of end position */
extern int MAGimageEndY(MagImage *mag)
{
  return MAGshortOfHeaderField(mag,10);
}

/**
 * MAGimageWidth - returns image width
 */
extern int MAGimageWidth(MagImage *mag)
{
  return MAGimageEndX(mag)-MAGimageStartX(mag)+1;
}

/**
 * MAGimageHeight - returns image height
 */
extern int MAGimageHeight(MagImage *mag)
{
  return MAGimageEndY(mag)-MAGimageStartY(mag)+1;
}

/**
 * MAGAFlagsOffset - returns the offset _from_the_beginning_of_the_file_
 *  to the A Flags 
 */
static unsigned long MAGAFlagsOffset(MagImage *mag)
{
  return MAGlongOfHeaderField(mag,12)+mag->head_off;
}

/**
 * MAGBFlagsOffset - returns the offset _from_the_beginning_of_the_file_
 *  to the B Flags 
 */
static unsigned long MAGBFlagsOffset(MagImage *mag)
{
  return MAGlongOfHeaderField(mag,16)+mag->head_off;
}

/**
 * MAGAFlagsLength - returns the length of A flags in bytes
 */
static unsigned long MAGAFlagsLength(MagImage *mag)
{
  int width,height;
  long bits;
  height=MAGimageHeight(mag);
  if((width=MAGnumPixelsInColumn(mag))<0)
    return -1;
  bits=width*height;
  if((bits&1)==1)
    bits++;
  bits=bits>>1;
  if((bits&0x07)>0)
    bits=(bits^(bits&0x07))+8;
  return bits>>3; /* in bytes */
}

/**
 * MAGBFlagsLength - returns the length of B flags in bytes
 */
static unsigned long MAGBFlagsLength(MagImage *mag)
{
  return MAGlongOfHeaderField(mag,20);
}

/**
 * MAGpixelsOffset - returns the offset from the beginning of the file 
 *  to the pixels
 */
static unsigned long MAGpixelsOffset(MagImage *mag)
{
  return MAGlongOfHeaderField(mag,24)+mag->head_off;
}

/**
 * MAGpixelsLength - returns the length of pixels region in bytes
 */
static unsigned long MAGpixelsLength(MagImage *mag)
{
  return MAGlongOfHeaderField(mag,28);
}

/**
 * MAGoffsetIsInRange - returns if it is safe to access the offset in
 *  the file.
 */
static int MAGoffsetIsInRange(MagImage *mag,long offset)
{
  return offset<(mag->len);
}

/**
 * MAGpalette - returns palette color for index.
 *  returns negative value if index is out of range or file broken
 *  Each of green, red, blue color intensities are represented as
 *  8-bit unsigned integer, and the result packs these three in one int.
 *   result <= 0x00RRGGBB
 *  0 <= index < MAGscrModeNumColors(mag)
 */
extern int MAGpalette(MagImage *mag, int index)
{
  int result;
  long offset;
  if(index>=MAGscrModeNumColors(mag))
    return -1;
  offset=mag->head_off+32+index*3;
  if(MAGoffsetIsInRange(mag,offset+2))
    {
      unsigned char *colors=&(((unsigned char*)mag->body)[offset]);
      result=(colors[1]<<16)|(colors[0]<<8)|colors[2];
    }    
  else
    result=-1;
  return result;
}

/**
 * MAGnumPixelsInColumn - returns how many pixels are packed in a column
 */
static int MAGnumPixelsInColumn(MagImage *mag)
{
  int result;
  int imagewidth=MAGimageWidth(mag);
  int pixelwidth=MAGpixelWidth(mag);
  if(pixelwidth==4)
    {
      result=(imagewidth>>2)+((imagewidth&1)|((imagewidth&2)>>1));
    }
  else if(pixelwidth==2)
    {
      result=(imagewidth>>1)+(imagewidth&1);
    }
  else 
    result=-1;
  return result;
}

/**
 * MAGReader - Fills fields of MagReader for use.
 *  call MAGReaderDelete(rd) after use
 */
extern int MAGReader(MagImage *mag, MagReader *rd)
{
  unsigned int i;
  unsigned long pixelsEnd, AFlagsEnd, BFlagsEnd;
  int ppc=MAGnumPixelsInColumn(mag);

  /* check boundaries */
  pixelsEnd=MAGpixelsOffset(mag)+MAGpixelsLength(mag)-1;
  AFlagsEnd=MAGAFlagsOffset(mag)+MAGAFlagsLength(mag)-1;
  BFlagsEnd=MAGBFlagsOffset(mag)+MAGBFlagsLength(mag)-1;
  if(!MAGoffsetIsInRange(mag,pixelsEnd) ||
     !MAGoffsetIsInRange(mag,AFlagsEnd) ||
     !MAGoffsetIsInRange(mag,BFlagsEnd))
    return MAGERR_DATA_PARTIAL;
  /* fill fields */
  rd->mag=mag;
  if((rd->lastflag=(unsigned char *)calloc(ppc,sizeof(unsigned char)))==NULL)
    return MAGERR_MEM_ERROR;
  for(i=0; i<17; i++)
    if((rd->lastcol[i]=(unsigned char *)calloc(ppc,sizeof(unsigned char)*2))==NULL)
      {
	/* free partially alloc'd memory regions */
	unsigned int j;
	for(j=0; j<i; j++)
	  free(rd->lastcol[j]);
	return MAGERR_MEM_ERROR;
      }
  rd->flaga_bit=0;
  rd->flagb_nibble=0;
  rd->flaga_head=0;
  rd->flagb_head=0;
  rd->pixel_head=0;
  rd->zero_partial=0;
  return MAGERR_NOERROR;
}

/**
 * MAGReaderDelete - purge the resource occupied by the MagReader
 *  doesn't free the memory region used by by the MagReader itself
 */
extern int MAGReaderDelete(MagReader *rd)
{
  unsigned int i;
  for(i=0; i<17; i++)
    free(rd->lastcol[i]); 
  free(rd->lastflag);
  return MAGERR_NOERROR;
}

/**
 * MAGReaderNextPixel - reads the next pixel for reader rd, copies the
 *  contents to buf. A `pixel' is 4 dots for 8/16 color images, and 2 dots
 *  for 256 color images. Length of buf must be over 4/2 bytes for each 
 *  color mode. Use MAGrowWidth to get the row width.
 */
extern int MAGReaderNextPixel(MagReader *rd, unsigned char *buf)
{
  static unsigned char refPixel[15][2]=
    { {1,0},{2,0},{4,0},
      {0,1},{1,1},
      {0,2},{1,2},{2,2},
      {0,4},{1,4},{2,4},
      {0,8},{1,8},{2,8},
      {0,16} };
  int width;
  MagImage *mag=rd->mag;
  unsigned char flag;
  int y,x; /* position of the reading pixel */
  int err;
  int pixelWidth=MAGpixelWidth(mag);
  unsigned long pixelCount; /* serial number of the reading pixel */
  unsigned char *pixelptr;

  pixelCount=(rd->flaga_head<<4)|(rd->flaga_bit<<1)|rd->zero_partial|rd->flagb_nibble;
  width=MAGnumPixelsInColumn(mag);
  x=pixelCount%width;
  y=pixelCount/width;
  
  if((err=MAGReaderNextFlag(rd,x,&flag))<0)
    return err;

  /* rotate recent row history */
  if(x==0 && y>0)
    {
      unsigned int j;
      unsigned char *purged=rd->lastcol[16];
      for(j=16; j>0; j--)
	rd->lastcol[j]=rd->lastcol[j-1];
      rd->lastcol[0]=purged;
    }
  
  /* get the pointer to the pixel data */
  if(flag==0)
    {
      if(rd->pixel_head<MAGpixelsLength(mag))
	pixelptr=&(((unsigned char*)mag->body)[MAGpixelsOffset(mag)+rd->pixel_head]);
      else
	return MAGERR_DATA_INVALID;
      rd->pixel_head+=2;
    }
  else
    {
      int histx=x-refPixel[flag-1][0];
      int histy=refPixel[flag-1][1];
      if(histy<0)
	return MAGERR_DATA_INVALID;
      pixelptr=&(rd->lastcol[histy][histx<<1]);
    }
  /* update history buffer */
  *((uint16_t *)&(rd->lastcol[0][x<<1]))=*((uint16_t*)pixelptr);
  /* write result */
  if(pixelWidth==2)
    {
      buf[0]=pixelptr[0]; buf[1]=pixelptr[1];
    }
  else if(pixelWidth==4)
    {
      buf[1]=pixelptr[0]&0xF; buf[0]=(pixelptr[0]&0xF0)>>4;
      buf[3]=pixelptr[1]&0xF; buf[2]=(pixelptr[1]&0xF0)>>4;
    }
  else
    return MAGERR_DATA_INVALID;
  return MAGERR_NOERROR;
}

/**
 * MAGReaderNextFlag - get next flag. also update information needed for 
 *  successive reads. It's just a state machine after all.
 */
static int MAGReaderNextFlag(MagReader *rd,int x,unsigned char* flagp)
{
  unsigned char flag;
  MagImage *mag=rd->mag;

  /* get flag value */
  if(rd->flagb_nibble) /* there is a half-read B flag */
    {
      /* The B flag is already read, so it is safe to assume the address
	 of the B flag is valid. */
      flag=((unsigned char *)mag->body)[MAGBFlagsOffset(mag)+rd->flagb_head]&0x0F;
      rd->flagb_nibble=0;
      rd->flagb_head++;
      rd->flaga_bit++;
    }
  else if(rd->zero_partial)
    {
      flag=0;
      rd->zero_partial=0;
      rd->flaga_bit++;
    }
  else /* read new A flag */
    {
      unsigned long aaddr;
      if(!(rd->flaga_head<MAGAFlagsLength(mag)))
	return MAGERR_DATA_END;
      aaddr=MAGAFlagsOffset(mag)+rd->flaga_head;
      
      if(MAGoffsetIsInRange(mag,aaddr))
	{
	  int aflag=((((unsigned char*)mag->body)[aaddr])>>(7-rd->flaga_bit))&1;
	  if(aflag!=0) /* read new bflag */
	    {
	      unsigned long baddr;
	      baddr=MAGBFlagsOffset(mag)+rd->flagb_head;
	      if(MAGoffsetIsInRange(mag,baddr))
		{
		  flag=((unsigned char *)mag->body)[MAGBFlagsOffset(mag)+rd->flagb_head]>>4;
		  rd->flagb_nibble=1;
		}
	      else
		return MAGERR_DATA_PARTIAL;
	    }
	  else /* next two flags are 0 */
	    {
	      flag=0;
	      rd->zero_partial=1;
	    }
	}
      else
	return MAGERR_DATA_PARTIAL;
    }

  if(rd->flaga_bit>7)
    {
      rd->flaga_head+=1;
      rd->flaga_bit=0;
    }
  flag=flag^(rd->lastflag[x]);
  rd->lastflag[x]=flag;
  if(flagp!=NULL)
    *flagp=flag;
  return MAGERR_NOERROR;
}
