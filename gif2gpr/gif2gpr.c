/*   gif2gpr.c - Displays a GIF image on the apollo screen using GPF graphics.
 * Works on color & monochrome displays, will dither images as needed.
 *
 * The majority of this code was lifted right out 
 * of Scott Hemphill's gif to postscript converter.
 * The display portion of the code was written by
 * Ben Samit and Ken Hampel.  Since Scott Hemphill was 
 * kind enough to put his code in the public domain,
 * I feel obliged to do the same. 
 *
 * This program is hereby placed in the public domain.
 * There are no restrictions on the use of all or any
 * part of this program. 
 * 
 * Ben Samit   March 1989
 * samit@demon.siemens.com
 *
 * Resizing feature added by Roque D. Oliveira,  April 1989.
 * Save-to-a-bitmap feature added by Roque D. Oliveira,  April 1989.
 * oliveria@caen.engin.umich.edu  
 * I would appreciate hearing of any improvements (or bug fixes) you
 * make to this program.
 *               
 * Code optimized for speed by Brett W. Davis,  Feb 1990.
 * Full screen view option added by Brett W. Davis,  Feb 1990.
 * bwdavis@icaen.uiowa.edu
 * Set_gray_map() and dither patterns from Roger Black, feb 1990.
 * jrblack@adus.ecn.uiowa.edu
 * Dithering on B&W screens by Brett W. Davis,  Feb 1990.
 * Memory use decreased from 2.5megs to 300K by Brett W. Davis,  Feb 1990.
 * Outerleave blur to screen feature added by Brett W. Davis,  Feb 1990.
 * bwdavis@icaen.uiowa.edu
 * Cleanup handler pascal source from David B. Funk, feb 1990.
 * dbfunk@icaen.uiowa.edu
 * 4bit color supported via grayscale feature added by Brett W. Davis,  Feb 1990.
 * I would appreciate hearing of any suggestions for added features
 * (and bug reports) regarding this program.  Brett Davis.
 */


#include <stdio.h>
#include <string.h>
#include <apollo/base.h>
#include <apollo/gpr.h>
#include <apollo/pad.h>
#include <apollo/error.h>
#include <apollo/pgm.h>
#include <apollo/pfm.h>

char *memcpy();
char *malloc();
int strncmp();

#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) > (y) ? (x) : (y))
#define FALSE 0
#define TRUE 1
char *progname;				/* the name of this program */

status_$t            status;
gpr_$color_vector_t  color_map; 
gpr_$window_t        display;
gpr_$rgb_plane_t     hi_plane;
gpr_$disp_char_t     display_characteristics;

static short int     disp_len = sizeof(gpr_$disp_char_t);
       short int     disp_len_returned;

gpr_$offset_t        init_size , SIZE , eSIZE;
gpr_$position_t      ORIGIN , eORIGIN;
gpr_$bitmap_desc_t   display_bitmap_desc;
gpr_$display_mode_t  mode=gpr_$direct;
stream_$id_t         graphics_stream , unit;
pad_$window_desc_t   pad_window ;

char        wait_string[]   = {"Resizing Image.  Please wait..."};
short int   wait_string_len = sizeof(wait_string);

int save_to_bitmap = false;
char *bitmap_file_name = NULL;

void redraw(); 
gpr_$rwin_pr_t  rwin = redraw;        /* entry point for refreshing window */
gpr_$rhdm_pr_t  rhdm = NULL;          /* entry point for refreshing hidden display memory */

typedef int bool;
typedef struct codestruct
 {
  struct codestruct *prefix;
  unsigned char first,suffix;
 } codetype;


FILE *infile;
unsigned int screenwidth;           /* The dimensions of the screen */
unsigned int screenheight;          /*   (not those of the image)   */
bool global;                        /* Is there a global color map? */
int globalbits;                     /* Number of bits of global colors */
unsigned char globalmap[256][3];    /* RGB values for global color map */
unsigned char *raster;              /* Decoded image data */
codetype *codetable;                /* LZW compression code data */
int datasize,codesize,codemask;     /* Decoder working variables */
int clear,eoi;                      /* Special code values */

unsigned  WIDTH,  HEIGHT;
unsigned eWIDTH, eHEIGHT;
gpr_$pixel_value_t *foo;             /* Scan line buffer */                 
int *ix_inc_table;                   /* Resize x table buffer */
int *interleavetable;                /* Scan line interleave table */
int full_screen = false;             /* Don't expand to max size by defalt */
gpr_$pixel_value_t gray[256];	     /* Four-bit values for b/w dither, also 4bit color */
int monocrome = false;
int outerleave = false;
pfm_$cleanup_rec cl_rec;
status_$t pfm_status;
int four_bit_color = false;

    /* Here is the dithering array.  The four octal numbers on each  
       line represent a four-by-four bitmap that is displayed on the 
       Apollo screen for that color code.  There are, of course, 16  
       color codes from 0 to 15. */                                  
                                                                     
    static gpr_$pixel_value_t dither[256] = {                                    
             1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
             1,1,1,1, 1,1,0,1, 1,1,1,1, 1,1,1,1,
             1,1,1,1, 1,1,0,1, 1,1,1,1, 0,1,1,1,
             1,0,1,1, 1,1,0,1, 1,1,1,1, 0,1,0,1,
             1,1,1,1, 1,1,0,1, 1,1,1,1, 0,1,0,1,
             1,0,1,1, 1,1,0,1, 1,0,1,1, 0,1,0,1,
             1,0,1,0, 1,1,0,1, 1,0,1,1, 0,1,0,1,
             1,0,1,0, 1,1,0,1, 1,0,1,0, 0,1,0,1,
             1,0,1,0, 0,1,0,1, 1,0,1,0, 0,1,0,1,
             1,0,1,0, 0,1,0,1, 1,0,1,0, 0,1,0,0,
             1,0,1,0, 0,0,0,1, 1,0,1,0, 0,1,0,0,
             1,0,1,0, 0,0,0,0, 1,0,1,0, 0,1,0,0,
             1,0,1,0, 0,0,0,0, 1,0,1,0, 0,0,0,0,
             1,0,0,0, 0,0,0,0, 1,0,1,0, 0,0,0,0,
             1,0,0,0, 0,0,0,0, 0,0,1,0, 0,0,0,0,
             0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
				 };  
      
/***********************************/
void usage()
{
    fprintf(stderr,"usage: %s [ -o bitmap_file_name ] gif_file_name\n",progname);
    exit(-1);
}

/***********************************/
void fatal(s)
char *s;
{
  fprintf(stderr,"%s: %s\n",progname,s);
  exit(-1);
}

/***********************************/
void check(messagex)
char *messagex;
{
   if (status.all)
   {   error_$print (status);
       printf("Error occurred while %s.\n", messagex);
   }    
}

/***********************************/
void checksignature()
{
  char buf[6];

  fread(buf,1,6,infile);
  if (strncmp(buf,"GIF",3)) fatal("file is not a GIF file");
  if (strncmp(&buf[3],"87a",3)) fatal("unknown GIF version number");
}

/***********************************/
/* Get information which is global to all the images stored in the file */
void readscreen()
{
  unsigned char buf[7];

  fread(buf,1,7,infile);
  screenwidth = buf[0] + (buf[1] << 8);
  screenheight = buf[2] + (buf[3] << 8);
  global = buf[4] & 0x80;
  if (global)
  {
    globalbits = (buf[4] & 0x07) + 1;
    printf("global bitmap  : %d colors\n",(1<<globalbits));
    fread(globalmap,3,1<<globalbits,infile);
  }
}

/***********************************/
/* Output the bytes associated with a code to the raster array */
void outcode(p,fill)
register codetype *p;
register unsigned char **fill;
{
  if (p->prefix) outcode(p->prefix,fill);
  *(*fill)++ = p->suffix;
}

/***********************************/
/* Process a compression code.  "clear" resets the code table.  Otherwise
   make a new code table entry, and output the bytes associated with the code. */
void process(code,fill)
register code;
unsigned char **fill;
{
  static avail,oldcode;
  register codetype *p;

  if (code == clear)
  {
    codesize = datasize + 1;
    codemask = (1 << codesize) - 1;
    avail = clear + 2;
    oldcode = -1;
  }
   else if (code < avail)
   {
    outcode(&codetable[code],fill);
    if (oldcode != -1)
     {
      p = &codetable[avail++];
      p->prefix = &codetable[oldcode];
      p->first = p->prefix->first;
      p->suffix = codetable[code].first;
      if ((avail & codemask) == 0 && avail < 4096)
          {
           codesize++;
           codemask += avail;
          }
     }
    oldcode = code;
   } 
   else if (code == avail && oldcode != -1)
   {
    p = &codetable[avail++];
    p->prefix = &codetable[oldcode];
    p->first = p->prefix->first;
    p->suffix = p->first;
    outcode(p,fill);
    if ((avail & codemask) == 0 && avail < 4096)
      {
        codesize++;
        codemask += avail;
      }
   oldcode = code;
   } else
      {
         fatal("illegal code in raster data");
      }
}

/***********************************/
/* Decode a raster image */
void readraster(width,height)
unsigned width,height;
{
  unsigned char *fill = raster;
  unsigned char buf[255];
  register bits=0;
  register unsigned count,datum=0;
  register unsigned char *ch;
  register int code;

  datasize = getc(infile);
  clear = 1 << datasize;
  eoi = clear+1;
  codesize = datasize + 1;
  codemask = (1 << codesize) - 1;
  codetable = (codetype*)malloc(4096*sizeof(codetype));
  if (!codetable) fatal("not enough memory for code table");
  for (code = 0; code < clear; code++)
   {
    codetable[code].prefix = (codetype*)0;
    codetable[code].first = code;
    codetable[code].suffix = code;
   }
  for (count = getc(infile); count > 0; count = getc(infile))
   {
    fread(buf,1,count,infile);
    for (ch=buf; count-- > 0; ch++)
     {
      datum += *ch << bits;
      bits += 8;
      while (bits >= codesize)
       {
        code = datum & codemask;
        datum >>= codesize;
        bits -= codesize;
        if (code == eoi) goto exitloop;  /* This kludge put in
                                            because some GIF files
                                            aren't standard */
        process(code,&fill);
       }
     }
   }
exitloop:
    if (fill != raster + width*height) fatal("raster has the wrong size");
    free(codetable);
}

/***********************************/
void readimage()
{
  unsigned char buf[9];
  unsigned left,top,width,height;
  bool local,interleaved;
  char localmap[256][3];
  int localbits;
  register row;
  register i;
  int rows;  
  int table;

  fread(buf,1,9,infile);
  left = buf[0] + (buf[1] << 8);
  top = buf[2] + (buf[3] << 8);
  width = buf[4] + (buf[5] << 8);
  height = buf[6] + (buf[7] << 8);
  WIDTH= width;
  HEIGHT=height;
  printf("gif dimensions : %d x %d pixels\n",WIDTH,HEIGHT);
  local = buf[8] & 0x80;
  interleaved = buf[8] & 0x40;
  if (local)
   {
    localbits = (buf[8] & 0x7) + 1;
    fread(localmap,3,1<<localbits,infile);
   }
  else if (!global)
        {
         fatal("no colormap present for image");
        }
  raster = (unsigned char*)malloc(width*height);
  if (!raster) fatal("not enough memory for image");
  readraster(width,height);
  interleavetable = (int*)malloc(height*sizeof(int));
  if (!interleavetable) fatal("not enough memory for interleave table");

  row = 0;
  if (interleaved)
   {
    for (i = 0; i < height; i += 8) interleavetable[i] = row++;
    for (i = 4; i < height; i += 8) interleavetable[i] = row++;
    for (i = 2; i < height; i += 4) interleavetable[i] = row++;
    for (i = 1; i < height; i += 2) interleavetable[i] = row++;

    fprintf(stdout,"Interleaved scan lines. top %d left %d.\n",top,left);
   } 
  else
   {
    for (i=0; i< height; i++) interleavetable[i] = row++;
   }
}

/***********************************/
/* Read a GIF extension block (and do nothing with it). */
void readextension()
 {
  unsigned char code,count;
  char buf[255];
  while (count = getc(infile)) fread(buf,1,count,infile);
 }

/***********************************/
main(argc,argv)
int argc;
char **argv;
{
	int		c;
/* getopt stuff */
	extern int	optind;     /* index of which argument is next */
	extern char	*optarg;    /* pointer to argument of this option */ 

  int quit = FALSE;
  char ch;

progname = strrchr(argv[0], '/');
if (progname)
	progname++;
else
	progname = argv[0];

while ((c=getopt(argc,argv,"14bfo:?"))!=EOF) 
 {
	switch (c)
     {                
        case '1':    monocrome = true;   /*** testing mode ***/
                     break;

        case '4':    four_bit_color = true;   /*** testing mode ***/
                     break;

        case 'b':    outerleave = true;
                     break;

        case 'f':    full_screen = true;
                     break;

		case 'o':
                     save_to_bitmap = true;
                     bitmap_file_name = optarg;
                     fprintf(stdout,"output  file = %s \n",bitmap_file_name);
                     break;      
		case '?':
		default :
                     usage();
     }
 }


if (argc == optind)
 {
   usage();
 }
else
 { 
  infile = fopen(argv[optind],"r");
  if (!infile)
    {
     fprintf(stdout,"%s  : couldn't open input file %s \n",progname,argv[optind]);
     exit(-1);
    }
  optind++;
 }

if ( optind != argc ) usage();

  checksignature();
  readscreen();
  do 
   {
     ch = getc(infile);
     switch (ch)
     {
      case '\0':  break; 
      case ',':   readimage();
        break;
      case ';':   quit = TRUE;
        break;
      case '!':   readextension();
        break;
      default:    fatal("illegal GIF block type");
        break;
     }
   } while (!quit);

    Initialize();
    if (monocrome)
       {
        set_gray_map();
       }
    else
       {         
        if (four_bit_color) set_gray_map();
        save_color_map();

        pfm_status = pfm_$cleanup(&cl_rec);    /*** set up the cleanup handler ***/
        if (pfm_status.all != pfm_$cleanup_set)
             {     /*** we're here because of a fault, do our clean-up work ***/
                   /***  restore old color map ***/
              reset_color_map();
              Close();
               
              if (pfm_status.all = status_$ok)  pgm_$exit;
                else pfm_$signal(pfm_status);    /*** quit ***/
             }  

        if (four_bit_color) set_four_bit_color_map();
             else set_color_map();
       }  

    redraw(); 

    gpr_$set_cursor_active(true,&status);
/* Establish the refresh function. Once established, if the window needs to be 
   refreshed as the result of a pop or grow, GPR will automatically call the redraw function. */
    gpr_$set_refresh_entry (rwin,rhdm, &status);
    check("called gpr_$set_refresh_entry");

    KbEnable();
    if(save_to_bitmap) 
      { 
       SaveImage(display_bitmap_desc,bitmap_file_name);
      }
    if (monocrome == false) 
       {
        reset_color_map();
/*** we've done the work, blow off the clean-up handler ***/
        pfm_$rls_cleanup(&cl_rec,&status);   
       }
    Close();
}

/***********************************/
Initialize()
{ 
   float expand;
   float expand_X, expand_Y; 
   int skip, xskip, yskip;

   gpr_$inq_disp_characteristics(mode,unit,disp_len,&display_characteristics,&disp_len_returned,&status);
   check("in Initialiaze after calling gpr_$inq_display_characteristics");

   hi_plane  = display_characteristics.n_planes - 1;
   if ( hi_plane > 0 ) 
     {
      fprintf(stdout,"This apollo has %d planes (color display). \n",display_characteristics.n_planes);
     }
   else
     {
      fprintf(stdout,"This apollo has %d plane (black and white display).  \n",display_characteristics.n_planes);
      monocrome = true;
     }
    if (globalbits < 5) four_bit_color = false;
    if ((hi_plane == 3) & (globalbits > 4)) four_bit_color = true;
            
    eWIDTH  = WIDTH;
    eHEIGHT = HEIGHT;

    if ((full_screen) | 
        (display_characteristics.x_visible_size < WIDTH) |
        (display_characteristics.y_visible_size < HEIGHT) )   
      {
       expand_X = display_characteristics.x_visible_size / (float)WIDTH;
       expand_Y = display_characteristics.y_visible_size / (float)HEIGHT;
/* expand gets the smaller value for a even X,Y expansion. */  
       expand = (expand_X > expand_Y) ? expand_Y : expand_X;       
       eWIDTH  = WIDTH * expand;
       eHEIGHT = HEIGHT * expand;
       full_screen = true;
      }

     if (monocrome & (full_screen != true)) {
         if (WIDTH < display_characteristics.x_visible_size) {              
           xskip = display_characteristics.x_visible_size/WIDTH;              
         }                                         
         if (HEIGHT < display_characteristics.y_visible_size) {            
           yskip = display_characteristics.y_visible_size/HEIGHT;    
         }                                         
         skip = xskip;                             
         if (yskip < xskip) skip = yskip;  
         if (skip > 1) {
           eWIDTH = eWIDTH * skip;
           eHEIGHT = eHEIGHT * skip;
           full_screen = true;
         } 
     }                                             

    foo = (gpr_$pixel_value_t *) malloc(display_characteristics.x_visible_size * 4 *
                                        sizeof(gpr_$pixel_value_t));
    if (!foo) fatal("in Initialize: not enough memory for scan line buffer");
    ix_inc_table = (int *) malloc(display_characteristics.x_visible_size * sizeof(int)); 
    if (!ix_inc_table) fatal("in Initialize: not enough memory for scan line table");
                      
    pad_window.top    = 0;
    pad_window.left   = 0;
    pad_window.width  = eWIDTH;
    pad_window.height = eHEIGHT; 

  pad_$create_window((char *)0,(short)0,pad_$transcript,(short)1,pad_window,&graphics_stream,&status);

  unit = graphics_stream ;

  pad_$set_full_window(unit,(short) 1,&pad_window,&status);
  pad_$set_auto_close(unit, (short) 1, true, &status );
  pad_$set_border (unit,(short) 1, false, &status);
  pad_$set_scale (unit,(short) 1,(short) 1, &status);

  init_size.x_size = 8192;
  init_size.y_size = 8192;
  gpr_$init(mode,unit, init_size, hi_plane, &display_bitmap_desc, &status);
  check("in Initialize after calling gpr_$init");
}

/***********************************/
draw_color()
{
    long int x,y,count;
    gpr_$window_t    destination_window;

    destination_window.window_base.x_coord = 0;
    destination_window.window_size.x_size = WIDTH;
    destination_window.window_size.y_size = 1;

    gpr_$acquire_display(&status);
                       
    for(y=0;y<HEIGHT; y++) 
     {              
      count = interleavetable[y] * WIDTH;
      for(x=0;x<WIDTH; x++)
        {
         foo[x]= (gpr_$pixel_value_t) raster[count++];
        }
      
      destination_window.window_base.y_coord = y;
      gpr_$write_pixels(foo, destination_window, &status);
     } 
    gpr_$release_display(&status);
}  

/***********************************/
draw_gray()
{
    long int x,y,i;
    int scan1, scan2, scan3, scan4, g, y_width;
    gpr_$window_t    destination_window;
     
    destination_window.window_base.x_coord = 0;
    destination_window.window_size.x_size = WIDTH;
    destination_window.window_size.y_size = 4;

    gpr_$acquire_display(&status);
                       
    for(y=0;y<eHEIGHT-3; y+=4) 
     {
      scan1 = 0;
      scan2 = WIDTH;
      scan3 = WIDTH + scan2;
      scan4 = WIDTH + scan3;
      y_width =  WIDTH * interleavetable[y];
      for(x=0;x<eWIDTH-3; x+=4)
          {                     
           g = gray[raster[y_width+x]] << 4;
           for(i=0;i<4; i++) foo[scan1++] = dither[g++];
           for(i=0;i<4; i++) foo[scan2++] = dither[g++];
           for(i=0;i<4; i++) foo[scan3++] = dither[g++];
           for(i=0;i<4; i++) foo[scan4++] = dither[g++];
          }
      destination_window.window_base.y_coord = y;
      gpr_$write_pixels(foo, destination_window, &status);
     } 
    gpr_$release_display(&status);
}  

/***********************************/
Close() 
{
  int i=0;
 
  gpr_$terminate (false, &status);
  while(status.fail && i<10) 
   {
    i++;
    gpr_$terminate (false, &status);
    printf("attempting to terminate i= %d\n",i);
   }
  free(raster);
  free(ix_inc_table);
  free(foo);
}      

/***********************************/
/* This routine turns the 24-bit RGB color map into a 4-bit mono map.  The 
   resulting numbers are fed through the dither table to get the actual bit
   patterns displayed on the screen.  Different results can be obtained by 
   playing around with the algorithm used here. */                         
                                                                           
set_gray_map() {                                                          
  int i,red,green,blue;                                                    
                                                                           
  for(i=0;i<(1<<globalbits);i++) {                                         
    red = globalmap[i][0];                                                 
    green = globalmap[i][1];                                               
    blue = globalmap[i][2];                                                
    gray[i]=((red + green + blue)/3) >> 4;                                
  }                                                                        
}                                                                          

/***********************************/
set_color_map() 
{
  int i;
  gpr_$color_vector_t color; 

  gpr_$acquire_display(&status);
  for(i=0;i<(1<<globalbits);i++) 
    {
     color[i]=(globalmap[i][0]<<16)|(globalmap[i][1]<<8)|(globalmap[i][2]);
    }
  gpr_$set_color_map((gpr_$pixel_value_t) 0,(short) (1<<globalbits), color, &status);
  check("in set_color_map after calling gpr_$set_color_map");
  gpr_$release_display(&status);
}

/***********************************/
set_four_bit_color_map() 
{
  int i, shade, gray;
  gpr_$color_vector_t color; 

  gpr_$acquire_display(&status);
  shade = 0;
  gray = (16<<16) | (16<<8) | 16;
  for(i=0;i<(1<<globalbits);i++) 
    {
     color[i] = shade;
     shade += gray;
    }
  gpr_$set_color_map((gpr_$pixel_value_t) 0,(short) (1<<globalbits), color, &status);
  check("in set_four_bit_color_map after calling gpr_$set_color_map");
  gpr_$release_display(&status);
}

/***********************************/
save_color_map() 
{
  gpr_$acquire_display(&status);
  gpr_$inq_color_map((gpr_$pixel_value_t) 0,(short) (1<<globalbits), color_map, &status);
  check("in save_color_map after calling gpr_$inq_color_map");
  gpr_$release_display(&status);
}

/***********************************/
reset_color_map()
{
  gpr_$acquire_display(&status);
  gpr_$set_color_map((gpr_$pixel_value_t) 0,(short) (1<<globalbits), color_map, &status);
  check("in reset_color_map after calling gpr_$set_color_map");
  gpr_$release_display(&status);
}

/***********************************/
KbEnable() 
{
  gpr_$keyset_t    keys;
  gpr_$event_t     ev_type;
  gpr_$position_t  ev_pos;
  unsigned char    ev_char;
  short int        KBD_$CR=0x96;

  lib_$init_set(keys, (short)256);
  lib_$add_to_set(keys, (short)256, ' ');
  lib_$add_to_set(keys, (short)256, 'q');
  lib_$add_to_set(keys, (short)256, 'Q');
  lib_$add_to_set(keys, (short)256, KBD_$CR);
  gpr_$enable_input (gpr_$keystroke, keys, &status);
  check("in KbEnable after calling gpr_$enable_input");
  gpr_$event_wait (&ev_type, &ev_char, &ev_pos, &status);
}

/***********************************/
void inq_window_size()
{
   gpr_$inq_bitmap_position  (display_bitmap_desc, &eORIGIN , &status);
   gpr_$inq_bitmap_dimensions(display_bitmap_desc, &eSIZE   , &hi_plane , &status);
   eWIDTH  = eSIZE.x_size ;
   eHEIGHT = eSIZE.y_size ;
}

/***********************************/
Resize_color()
{
    gpr_$offset_t    textsize ;
    short int        font_id;
    int  ix,iy,ex,ey,iy_width;

    gpr_$window_t    destination_window;

    destination_window.window_base.x_coord = 0;
    destination_window.window_size.x_size = eWIDTH;
    destination_window.window_size.y_size = 1;

    gpr_$acquire_display(&status);
                       
           for (ex=0;  ex<eWIDTH;  ex++) 
                {
                ix_inc_table[ex] = (WIDTH * ex) / eWIDTH; 
                }
           for (ey=0;  ey<eHEIGHT;  ey++)
           {
            iy_width =  WIDTH * interleavetable[(HEIGHT * ey) / eHEIGHT];
            for (ex=0;  ex<eWIDTH;  ex++)
               {
                foo[ex]= (gpr_$pixel_value_t) raster[iy_width + ix_inc_table[ex]];
               }
            destination_window.window_base.y_coord = ey;
            gpr_$write_pixels(foo, destination_window, &status);
         } 
        gpr_$release_display(&status);
}

/***********************************/
Resize_four_bit_color()
{
    gpr_$offset_t    textsize ;
    short int        font_id;
    int  ix,iy,ex,ey,iy_width;

    gpr_$window_t    destination_window;

    destination_window.window_base.x_coord = 0;
    destination_window.window_size.x_size = eWIDTH;
    destination_window.window_size.y_size = 1;

    gpr_$acquire_display(&status);
                       
           for (ex=0;  ex<eWIDTH;  ex++) 
                {
                ix_inc_table[ex] = (WIDTH * ex) / eWIDTH; 
                }
           for (ey=0;  ey<eHEIGHT;  ey++)
           {
            iy_width =  WIDTH * interleavetable[(HEIGHT * ey) / eHEIGHT];
            for (ex=0;  ex<eWIDTH;  ex++)
               {
                foo[ex]= gray[raster[iy_width + ix_inc_table[ex]]];
               }
            destination_window.window_base.y_coord = ey;
            gpr_$write_pixels(foo, destination_window, &status);
         } 
        gpr_$release_display(&status);
}

/***********************************/
Resize_gray()
{
    gpr_$offset_t    textsize ;
    short int        font_id;
    int  ix,iy,ex,ey,iy_width; 
    int scan1, scan2, scan3, scan4, i, g;

    gpr_$window_t    destination_window;

    destination_window.window_base.x_coord = 0;
    destination_window.window_size.x_size = eWIDTH;
    destination_window.window_size.y_size = 4;

    gpr_$acquire_display(&status);
                       
           for (ex=0;  ex<eWIDTH-3;  ex+=4) 
                {
                 ix_inc_table[ex] = (WIDTH * ex) / eWIDTH; 
                }
           for (ey=0;  ey<eHEIGHT-3;  ey+=4)
                {     
                 scan1 = 0;
                 scan2 = eWIDTH;
                 scan3 = eWIDTH + scan2;
                 scan4 = eWIDTH + scan3;
                 iy_width =  WIDTH * interleavetable[(HEIGHT * ey) / eHEIGHT];
                 for(ex=0;ex<eWIDTH-3; ex=ex+4)
                     {                     
                      g = gray[raster[iy_width + ix_inc_table[ex]]] << 4;
                      for(i=0;i<4; i++) foo[scan1++] = dither[g++];
                      for(i=0;i<4; i++) foo[scan2++] = dither[g++];
                      for(i=0;i<4; i++) foo[scan3++] = dither[g++];
                      for(i=0;i<4; i++) foo[scan4++] = dither[g++];
                     }
                destination_window.window_base.y_coord = ey;
                gpr_$write_pixels(foo, destination_window, &status);
         } 
        gpr_$release_display(&status);
}

/***********************************/
outerleave_color()
{
    gpr_$offset_t    textsize ;
    short int        font_id;
    int  ix,iy,ex,ey,iy_width,i; 
    static int  start_table[4] = {0,4,2,1};
    static int  inc_table[4] = {8,8,4,2};

    gpr_$window_t    destination_window;

    destination_window.window_base.x_coord = 0;
    destination_window.window_size.x_size = eWIDTH;
    destination_window.window_size.y_size = 1;

    gpr_$acquire_display(&status);
                       
      for (ex=0;  ex<eWIDTH;  ex++) 
        {
         ix_inc_table[ex] = (WIDTH * ex) / eWIDTH; 
        }      
      for (i=0;i<4;i++)
         {
           for (ey=start_table[i];  ey<eHEIGHT;  ey+=inc_table[i])
           {
            iy_width =  WIDTH * interleavetable[(HEIGHT * ey) / eHEIGHT];
            for (ex=0;  ex<eWIDTH;  ex++)
               {
                foo[ex]= (gpr_$pixel_value_t) raster[iy_width + ix_inc_table[ex]];
               }
            destination_window.window_base.y_coord = ey;
            gpr_$write_pixels(foo, destination_window, &status);
           } 
         }
       gpr_$release_display(&status);
}

/***********************************/
outerleave_gray()
{
    gpr_$offset_t    textsize ;
    short int        font_id;
    int  ix,iy,ex,ey,iy_width; 
    int scan1, scan2, scan3, scan4, i, g, a;
    static int  start_table[4] = {0,4,2,1};
    static int  inc_table[4] = {8,8,4,2};

    gpr_$window_t    destination_window;

    destination_window.window_base.x_coord = 0;
    destination_window.window_size.x_size = eWIDTH;
    destination_window.window_size.y_size = 4;

    gpr_$acquire_display(&status);
                       
          for (ex=0;  ex<eWIDTH-3;  ex+=4) 
            {
             ix_inc_table[ex] = (WIDTH * ex) / eWIDTH; 
            }
          for (a=0;a<4;a++)
            {
             for (ey=start_table[a]*4;  ey<eHEIGHT-3;  ey+=inc_table[a]*4)
                {     
                 scan1 = 0;
                 scan2 = eWIDTH;
                 scan3 = eWIDTH + scan2;
                 scan4 = eWIDTH + scan3;
                 iy_width =  WIDTH * interleavetable[(HEIGHT * ey) / eHEIGHT];
                 for(ex=0;ex<eWIDTH-3; ex=ex+4)
                     {                     
                      g = gray[raster[iy_width + ix_inc_table[ex]]] << 4;
                      for(i=0;i<4; i++) foo[scan1++] = dither[g++];
                      for(i=0;i<4; i++) foo[scan2++] = dither[g++];
                      for(i=0;i<4; i++) foo[scan3++] = dither[g++];
                      for(i=0;i<4; i++) foo[scan4++] = dither[g++];
                     }
                destination_window.window_base.y_coord = ey;
                gpr_$write_pixels(foo, destination_window, &status);
               } 
            } 
           gpr_$release_display(&status);
}

/***********************************/
void redraw() 
{
  inq_window_size();
  if (outerleave)
     {
      if (monocrome) outerleave_gray();
         else if (four_bit_color) Resize_four_bit_color();
              else outerleave_color();
     }
  else
     {
      if ( (WIDTH != eWIDTH) || (HEIGHT != eHEIGHT) ) 
        {
            if (monocrome) Resize_gray();
                else if (four_bit_color) Resize_four_bit_color();
                     else Resize_color();
        }
      else
        {
            if (monocrome) draw_gray();
                else if (four_bit_color) Resize_four_bit_color();
                     else draw_color();
        }
     }
}

/***********************************/
SaveImage(source_bitmap_desc,file_name)
gpr_$bitmap_desc_t  source_bitmap_desc;
char                *file_name;
{
    status_$t                       status;
    gpr_$bmf_group_header_array_t   header; 
    gpr_$color_vector_t             color_map;
    gpr_$version_t                  version;
    short int                       groups;
    gpr_$attribute_desc_t           attribs;  
    gpr_$window_t                   source_bitmap;     
    gpr_$bitmap_desc_t              disk_bitmap_desc , curs_pat_desc;
    gpr_$offset_t                   source_bitmap_size;    
    gpr_$rgb_plane_t                hi_plane;
    gpr_$position_t                 disk_bitmap_origin , source_bitmap_origin , curs_position, curs_origin;
    gpr_$raster_op_array_t          curs_raster_op;
    boolean                         disk_bmf_created , curs_active;

       gpr_$acquire_display (&status);

       gpr_$inq_cursor (&curs_pat_desc, curs_raster_op, &curs_active, &curs_position, &curs_origin,&status);
       if(curs_active) gpr_$set_cursor_active(false,&status); /* disable the cursor */

/* set the current bitmap the source bitmap */
       gpr_$set_bitmap(source_bitmap_desc,&status);
 check("in SaveImage");

/* get the size and the highest plane id of the source bitmap */
       gpr_$inq_bitmap_dimensions(source_bitmap_desc,&source_bitmap_size,&hi_plane,&status);  
       check("in SaveImage");

/* get the position of the upper left corner of the source bitmap */
/*       gpr_$inq_bitmap_position(source_bitmap_desc,source_bitmap_origin,&status) ;
         check("in SaveImage");
         fprintf(stdout,"source_bitmap_origin.x_coord=%d source_bitmap_origin.y_coord=%d \n",source_bitmap_origin.x_coord,source_bitmap_origin.y_coord);
*/
       gpr_$inq_color_map ((gpr_$pixel_value_t) 0,(short) 256,color_map,&status);   
       check("in SaveImage");

      header[0].n_sects = hi_plane +1;  /* # of sections in a group */
      header[0].pixel_size = 1;         /* # of bits per pixel in each section of a group */
      header[0].allocated_size  = 0;
      header[0].bytes_per_line  = 0;    /* # of bytes in one row of a bitmap. If =0, GPR takes care of it automatically. */
      header[0].bytes_per_sect  = 0;
      header[0].storage_offset  = 0;
      groups = (short) 1;
      version.gpr_$major =(short) 1;
      version.gpr_$minor =(short) 1;
/*
    source_bitmap.window_base.x_coord = source_bitmap_origin.x_coord;
    source_bitmap.window_base.y_coord = source_bitmap_origin.y_coord;
*/
    source_bitmap.window_base.x_coord = 0;
    source_bitmap.window_base.y_coord = 0;
    source_bitmap.window_size.x_size  = source_bitmap_size.x_size;
    source_bitmap.window_size.y_size  = source_bitmap_size.y_size; 
     
    gpr_$allocate_attribute_block(&attribs, &status);  

/* save the whole thing to disk */
    gpr_$open_bitmap_file(gpr_$create, file_name, (short)strlen(file_name),
       &version, &source_bitmap_size, &groups,header, attribs, &disk_bitmap_desc, &disk_bmf_created, &status);  
    check("in SaveImage");

/* set the current bitmap the disk bitmap */
    gpr_$set_bitmap(disk_bitmap_desc,&status); 
    check("in SaveImage");
  
/* set the color map in the disk bitmap file to the color map the application used. You can change the color map here */
    gpr_$set_bitmap_file_color_map (disk_bitmap_desc,(short) 0,(short) 255,color_map,&status);
    check("in SaveImage");

     disk_bitmap_origin.x_coord = 0;
     disk_bitmap_origin.y_coord = 0;

/* now just move the pixels from the source bitmap to the disk bitmap file */
    gpr_$pixel_blt(source_bitmap_desc,source_bitmap,disk_bitmap_origin,&status);
    check("in SaveImage");

/* set current bitmap back to the source bitmap */
    gpr_$set_bitmap(source_bitmap_desc, &status);    
    check("in SaveImage");

/* the disk bitmap file can be unlocked and available for print */
    gpr_$deallocate_bitmap(disk_bitmap_desc,&status);
    check("in SaveImage");

    gpr_$deallocate_attribute_block(attribs, &status);  
    check("in SaveImage");

  if ( hi_plane > 0 ) 
     {
      fprintf(stdout,"The bitmap file %s has been created. The source bitmap has %d planes. \n",file_name,(hi_plane + 1));
     }
  else
     {
      fprintf(stdout,"The bitmap file %s has been created. The source bitmap has %d plane.  \n",file_name,(hi_plane + 1));
     }

    if(curs_active) gpr_$set_cursor_active(true,&status);

    gpr_$release_display(&status) ;
}
