
/**************************************************************************
 *
 *  FILE:           EXAMPLE.C
 *
 *  MODULE OF:      EXAMPLE
 *
 *  DESCRIPTION:    Example program using TGASAVE.
 *
 *                  Produces output to an EGA-screen, then dumps it to
 *                  a TGA-file.
 *
 *                  This program is rather slow. The bottleneck is
 *                  Borland's getpixel() -function, not the TGASAVE-
 *                  functions.
 *
 *                  Since TGASAVE outputs truecolor TGA-files, it's rather
 *                  `stupid' to use that format on EGA-screen images.
 *                  Take this as an example only!
 *
 *  WRITTEN BY:     Sverre H. Huseby
 *
 **************************************************************************/



#ifndef __TURBOC__
  #error This program must be compiled using a Borland C compiler
#endif


#include <stdlib.h>
#include <stdio.h>
#include <graphics.h>

#include "tgasave.h"


#define NUMCOLORS 16            /* Number of EGA-colors. */



/**************************************************************************
 *                                                                        *
 *                       P R I V A T E    D A T A                         *
 *                                                                        *
 **************************************************************************/

static int
    Red[NUMCOLORS],             /* Red component for each color. */
    Green[NUMCOLORS],           /* Green component for each color. */
    Blue[NUMCOLORS];            /* Blue component for each color. */





/**************************************************************************
 *                                                                        *
 *                   P R I V A T E    F U N C T I O N S                   *
 *                                                                        *
 **************************************************************************/

/*-------------------------------------------------------------------------
 *
 *  NAME:           DrawScreen()
 *
 *  DESCRIPTION:    Produces some output on the graphic screen.
 *
 *  PARAMETERS:     None
 *
 *  RETURNS:        Nothing
 *
 */
static void DrawScreen(void)
{
    int  color = 1, x, y;
    char *text = "TGA-file produced by TGASAVE";


    /*
     *  Output some lines
     */
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

    /*
     *  And then some text
     */
    setfillstyle(SOLID_FILL, DARKGRAY);
    settextstyle(TRIPLEX_FONT, HORIZ_DIR, 4);
    bar(20, 10, textwidth(text) + 40, textheight(text) + 20);
    setcolor(WHITE);
    outtextxy(30, 10, text);
}



/*-------------------------------------------------------------------------
 *
 *  NAME:           SplitPalette()
 *
 *  DESCRIPTION:    Interpret the EGA-palette, extracting red-, green- and
 *                  blue-values. This is what we need for TrueColor.
 *
 *                  The extracted values are stored in global variables.
 *
 *  PARAMETERS:     None
 *
 *  RETURNS:        Nothing
 *
 */
static void SplitPalette(void)
{
    int q,                      /* Counter */
        color;                  /* Temporary color value */
    struct palettetype pal;


    /*
     *  Get the color palette, and extract the red, green and blue
     *  components for each color. In the EGA palette, colors are
     *  stored as bits in bytes:
     *
     *      00rgbRGB
     *
     *  where r is low intensity red, R is high intensity red, etc.
     *  We shift the bits in place like
     *
     *      Rr000000
     *
     *  for each component
     */
    getpalette(&pal);
    for (q = 0; q < NUMCOLORS; q++) {
        color = pal.colors[q];
        Red[q]   = ((color & 4) << 5) | ((color & 32) << 1);
        Green[q] = ((color & 2) << 6) | ((color & 16) << 2);
        Blue[q]  = ((color & 1) << 7) | ((color & 8) << 3);
    }
}



/*-------------------------------------------------------------------------
 *
 *  NAME:           GetPixel()
 *
 *  DESCRIPTION:    Callback function that retreives the pixel color value.
 *
 *  PARAMETERS:     x, y   Image pixel location.
 *                  col    Pointer to color structure.
 *
 *  RETURNS:        Nothing
 *
 */
static void GetPixel(int x, int y, TGA_Color *col)
{
    int idx;


    idx = getpixel(x, y);
    col->r = Red[idx];
    col->g = Green[idx];
    col->b = Blue[idx];
}



/*-------------------------------------------------------------------------
 *
 *  NAME:           TGA_DumpEga10()
 *
 *  DESCRIPTION:    Outputs a graphics screen to a TGA-file. The screen
 *                  must be in the mode 0x10, EGA 640x350, 16 colors.
 *
 *                  No error checking is done! Probably not a very good
 *                  example, then . . . :-)
 *
 *  PARAMETERS:     filename   Name of TGA-file.
 *
 *  RETURNS:        Nothing
 *
 */
static void TGA_DumpEga10(char *filename)
{
  #define WIDTH            640  /* 640 pixels across screen */
  #define HEIGHT           350  /* 350 pixels down screen */


    /*
     *  Get the current palette.
     */
    SplitPalette();

    /*
     *  Set up some variables that are included in the file.
     *  This is not neccesary.
     */
    TGA_SetImageID("Example image");
    TGA_SetAuthor("Sverre H. Huseby");
    TGA_SetSoftwareID("EXAMPLE");

    /*
     *  Create the TGA-file.
     */
    TGA_Save(filename, WIDTH, HEIGHT, GetPixel);
}





/**************************************************************************
 *                                                                        *
 *                    P U B L I C    F U N C T I O N S                    *
 *                                                                        *
 **************************************************************************/

int main(void)
{
    int gdr, gmd, errcode;


    /*
     *  Initiate graphics screen for EGA mode 0x10, 640x350x16
     */
    gdr = EGA;
    gmd = EGAHI;
    initgraph(&gdr, &gmd, "");
    if ((errcode = graphresult()) != grOk) {
        printf("Graphics error: %s\n", grapherrormsg(errcode));
        exit(-1);
    }

    /*
     *  Put something on the screen
     */
    DrawScreen();

    /*
     *  Dump the screen to a TGA-file
     */
    TGA_DumpEga10("EXAMPLE.TGA");

    /*
     *  Return to text mode
     */
    closegraph();

    return 0;
}
