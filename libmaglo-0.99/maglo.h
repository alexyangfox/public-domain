/* libmaglo : MAG image format loader library  */
/*      This is a public domain software.      */
/* 2008 TOMARI, Hisanobu                       */
#ifndef MAGLO_H_
#define MAGLO_H_
struct MagImage {
  long head_off; /* offset to the header from the start of file */
  unsigned long len; /* file length */
  void *body; /* mmap'd file body */
};
typedef struct MagImage MagImage;
struct MagReader {
  MagImage *mag; /* associated MagImage structure */
  unsigned char *lastcol[17];
  unsigned char *lastflag;
  unsigned long flaga_head,flagb_head,pixel_head; /* read head position */
  int flaga_bit;
  int flagb_nibble;
  int zero_partial;
};
typedef struct MagReader MagReader;
enum MAGerror {
  MAGERR_NOERROR=0,
  MAGERR_FILE_NOT_FOUND=-1,
  MAGERR_MAGIC_MISMATCH=-2,
  MAGERR_IO_ERROR=-3,
  MAGERR_MEM_ERROR=-4,
  MAGERR_DATA_PARTIAL=-5,
  MAGERR_DATA_END=-6,
  MAGERR_DATA_INVALID=-7
};
extern int MAGversion(void);
extern int MAGopen(MagImage *mag,const char *path);
extern int MAGclose(MagImage *mag);
extern long MAGcommentSize(MagImage *mag);
extern char *MAGcomment(MagImage *mag);
extern char *MAGmacType(MagImage *mag);
extern char *MAGuserName(MagImage *mag);
extern unsigned char MAGmacCode(MagImage *mag);
extern unsigned char MAGmacFlags(MagImage *mag);
extern unsigned char MAGscrModeCode(MagImage *mag);
extern int MAGscrMode200Lines(MagImage *mag);
extern int MAGscrModeNumColors(MagImage *mag);
extern int MAGpixelWidth(MagImage *mag);
extern int MAGscrModeIsDigital(MagImage *mag);
extern int MAGimageStartX(MagImage *mag);
extern int MAGimageStartY(MagImage *mag);
extern int MAGimageEndX(MagImage *mag);
extern int MAGimageEndY(MagImage *mag);
extern int MAGimageWidth(MagImage *mag);
extern int MAGimageHeight(MagImage *mag);
extern int MAGpalette(MagImage *mag, int index);
extern int MAGReader(MagImage *mag, MagReader *rd);
extern int MAGReaderDelete(MagReader *rd);
extern int MAGReaderNextPixel(MagReader *rd, unsigned char *buf);
#endif /* !defined(MAGLO_H_) */
