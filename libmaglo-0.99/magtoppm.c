/**
 * magtoppm - converts a MAG image to ppm format.
 *  use with utilities like ImageMagick
 * This is a public domain software.
 * 2008 TOMARI Hisanobu
 */
#include <stdlib.h>
#include <stdio.h>
#include <maglo.h>
#define MIN(x,y) ((x)>(y)?(y):(x))

static int magtoppm(const char *from,FILE *to);
int main(int argc, char *argv[]);

static int magtoppm(const char *from,FILE *to)
{
  MagImage m;
  MagReader rd;
  unsigned char pixel[4],palette[256][3];
  unsigned int i,width,height,pxlw,count;
  int err;

  /* Open MagImage */
  if((err=MAGopen(&m,from))<0)
    {
      fprintf(stderr,"MAGopen error(code=%d)\n",err);
      return -1;
    }

  /* Get info */
  width=MAGimageWidth(&m);
  height=MAGimageHeight(&m);
  pxlw=MAGpixelWidth(&m);

  /* copy palettes */
  for(i=0; i<MAGscrModeNumColors(&m); i++)
    {
      unsigned int pal=MAGpalette(&m,i);
      palette[i][0]=pal>>16;
      palette[i][1]=(pal&0x00FF00)>>8;
      palette[i][2]=(pal&0xFF);
    }

  /* print ppm header */
  fprintf(to,"P6\n%d %d\n255\n",width,height);

  /* create MagReader */
  if((err=MAGReader(&m,&rd))!=MAGERR_NOERROR)
    {
      fprintf(stderr,"MAGReader failed(code=%d)\n",err);
      MAGclose(&m);
      return -1;
    }
  /* copy dots */
  count=width;
  while((err=MAGReaderNextPixel(&rd,pixel))==MAGERR_NOERROR)
    {
      unsigned int k,valid_dots;
      valid_dots=MIN(pxlw,count);
      for(k=0; k<valid_dots; k++)
	{
	  fputc(palette[pixel[k]][0],to);
	  fputc(palette[pixel[k]][1],to);
	  fputc(palette[pixel[k]][2],to);
	}
      count=count-valid_dots;
      if(count<=0)
	count=width;
    }
  if(err<0 && err!=MAGERR_DATA_END)
    fprintf(stderr,"MAGReaderNextPixel returns error %d\n",err);

  /* cleanup MAGReader */
  MAGReaderDelete(&rd);
  /* cleanup MAGImage */
  MAGclose(&m);
  return 0;
}

int main(int argc, char *argv[])
{
  FILE *to;
  if(argc<2 || argc>3)
    {
      fprintf(stderr,
	      "%s MAGFILE [PPMFILE]\n"
	      "uses stdout if PPMFILE is not specified.\n",argv[0]);
      return EXIT_FAILURE;
    }
  if(argc==2)
    to=stdout;
  else
    {
      if((to=fopen(argv[2],"wb"))==NULL)
	{
	  perror("fopen");
	  return EXIT_FAILURE;
	}
    }

  if(magtoppm(argv[1],to)<0)
    return EXIT_FAILURE;
  else
    return EXIT_SUCCESS;
}

