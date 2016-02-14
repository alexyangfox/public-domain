
/**************************************************************************
 *
 *  FILE            example.c
 *
 *  DESCRIPTION     Example program using GIFSAVE.
 *
 *                  Produces output to an EGA-screen, then dumps it to
 *                  a GIF-file.
 *
 *                  This program is rather slow. The bottleneck is
 *                  Borland's getpixel() -function, not the GIFSAVE-
 *                  functions.
 *
 *  WRITTEN BY      Sverre H. Huseby <sverrehu@online.no>
 *
 **************************************************************************/

#ifndef __TURBOC__
  #error This program must be compiled using a Borland C compiler
#endif

#include <stdlib.h>
#include <stdio.h>
#include <graphics.h>

#include "gifsave.h"

/**************************************************************************
 *                                                                        *
 *                   P R I V A T E    F U N C T I O N S                   *
 *                                                                        *
 **************************************************************************/

/*-------------------------------------------------------------------------
 *
 *  NAME          DrawScreen
 *
 *  DESCRIPTION   Produces some output on the graphic screen.
 */
static void
DrawScreen(void)
{
    int  color = 1, x, y;
    char *text = "GIF-file produced by GIFSAVE";

    /* output some lines */
    setlinestyle(SOLID_LINE, 0, 3);
    for (x = 10; x < getmaxx(); x += 20) {
        setcolor(color);
        line(x, 0, x, getmaxy());
        if (++color > getmaxcolor())
            color = 1;
    }
    for (y = 8; y < getmaxy(); y += 17) {
        setcolor(color);
        line(0, y, getmaxx(), y);
        if (++color > getmaxcolor())
            color = 1;
    }

    /* and then some text */
    setfillstyle(SOLID_FILL, DARKGRAY);
    settextstyle(TRIPLEX_FONT, HORIZ_DIR, 4);
    bar(20, 10, textwidth(text) + 40, textheight(text) + 20);
    setcolor(WHITE);
    outtextxy(30, 10, text);
}



/*-------------------------------------------------------------------------
 *
 *  NAME          gpixel
 *
 *  DESCRIPTION   Callback function. Near version of getpixel()
 *
 *                If this program is compiled with a model using
 *                far code, Borland's getpixel() can be used
 *                directly.
 *
 *  INPUT         As for getpixel()
 *
 *  RETURNS       As for getpixel()
 */
static int
gpixel(int x, int y)
{
    return getpixel(x, y);
}



/*-------------------------------------------------------------------------
 *
 *  NAME          GIF_DumpEga10
 *
 *  DESCRIPTION   Outputs a graphics screen to a GIF-file. The screen
 *                must be in the mode 0x10, EGA 640x350, 16 colors.
 *
 *                No error checking is done! Probably not a very good
 *                example, then... :-)
 *
 *  INPUT         filename
 *                        name of GIF-file
 */
static void
GIF_DumpEga10(char *filename)
{
  #define WIDTH            640  /* 640 pixels across screen */
  #define HEIGHT           350  /* 350 pixels down screen */
  #define NUMCOLORS         16  /* Number of different colors */
  #define BITS_PR_PRIM_COLOR 2  /* Two bits pr primary color */

    int q;                      /* Counter */
    int color;                  /* Temporary color value */
    int red[NUMCOLORS];         /* Red component for each color */
    int green[NUMCOLORS];       /* Green component for each color */
    int blue[NUMCOLORS];        /* Blue component for each color */
    struct palettetype pal;


    /* Get the color palette, and extract the red, green and blue
     * components for each color. In the EGA palette, colors are
     * stored as bits in bytes:
     *
     *     00rgbRGB
     *
     * where r is low intensity red, R is high intensity red, etc.
     * We shift the bits in place like
     *
     *     000000Rr
     *
     * for each component */
    getpalette(&pal);
    for (q = 0; q < NUMCOLORS; q++) {
        color = pal.colors[q];
        red[q]   = ((color & 4) >> 1) | ((color & 32) >> 5);
        green[q] = ((color & 2) >> 0) | ((color & 16) >> 4);
        blue[q]  = ((color & 1) << 1) | ((color & 8) >> 3);
    }

    /* create and set up the GIF-file */
    GIF_Create(filename, WIDTH, HEIGHT, NUMCOLORS, BITS_PR_PRIM_COLOR);

    /* set each color according to the values extracted from the
     * palette */
    for (q = 0; q < NUMCOLORS; q++)
        GIF_SetColor(q, red[q], green[q], blue[q]);

    /* store the entire screen as an image using the user defined
     * callback function gpixel() to get pixel values from the screen */
    GIF_CompressImage(0, 0, -1, -1, gpixel);

    /* finish it all and close the file */
    GIF_Close();
}



/**************************************************************************
 *                                                                        *
 *                    P U B L I C    F U N C T I O N S                    *
 *                                                                        *
 **************************************************************************/

int
main(void)
{
    int gdr, gmd, errcode;

    /* initiate graphics screen for EGA mode 0x10, 640x350x16 */
    gdr = EGA;
    gmd = EGAHI;
    initgraph(&gdr, &gmd, "");
    if ((errcode = graphresult()) != grOk) {
        printf("Graphics error: %s\n", grapherrormsg(errcode));
        exit(-1);
    }

    /* put something on the screen */
    DrawScreen();

    /* dump the screen to a GIF-file */
    GIF_DumpEga10("EXAMPLE.GIF");

    /* return to text mode */
    closegraph();

    return 0;
}
