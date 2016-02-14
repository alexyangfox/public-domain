/* $Id: example.c,v 1.2 1998/07/05 16:29:56 sverrehu Exp $ */
/**************************************************************************
 *
 *  FILE            example.c
 *
 *  DESCRIPTION     Example program using gifsave. Draws some random dots
 *                  to a `screen'-buffer, and dumps to a GIF-file.
 *
 *                  Not at all useful, but it might give you the idea
 *                  of how to use gifsave.
 *
 *  WRITTEN BY      Sverre H. Huseby <sverrehu@online.no>
 *
 **************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "gifsave.h"

#define RANDOM(x) (rand() % x)

/**************************************************************************
 *                                                                        *
 *                       P R I V A T E    D A T A                         *
 *                                                                        *
 **************************************************************************/

#define WIDTH        160
#define HEIGHT       120
#define NUMCOLORS      5  /* must change DumpImage if this is changed */
#define COLORRES       8  /* number of bits for each primary color */
#define FILENAME "example.gif"

/* the image buffer */
static unsigned char Screen[WIDTH][HEIGHT];



/**************************************************************************
 *                                                                        *
 *                   P R I V A T E    F U N C T I O N S                   *
 *                                                                        *
 **************************************************************************/

/*-------------------------------------------------------------------------
 *
 *  NAME          PutPixel
 *
 *  DESCRIPTION   Enter a pixel in the image buffer.
 *
 *  INPUT         x,y     the location of the pixel
 *                color   the pixel value to store
 */
static void
PutPixel(int x, int y, int color)
{
    Screen[x][y] = color;
}



/*-------------------------------------------------------------------------
 *
 *  NAME          GetPixel
 *
 *  DESCRIPTION   Callback function fetching a pixel value from the buffer.
 *
 *  INPUT         x,y     the location of the pixel
 *
 *  RETURNS       Pixel value, in the range [0, NUMCOLORS).
 */
static int
GetPixel(int x, int y)
{
    return Screen[x][y];
}



/*-------------------------------------------------------------------------
 *
 *  NAME          DrawImage
 *
 *  DESCRIPTION   Produces some output inn the image buffer.
 */
static void
DrawImage(void)
{
    int q;

    printf("creating an image with random color dots "
	   "(red, green, blue and white)\n");
    for (q = 0; q < 1500; q++)
        PutPixel(RANDOM(WIDTH), RANDOM(HEIGHT), RANDOM(NUMCOLORS - 1) + 1);
}



/*-------------------------------------------------------------------------
 *
 *  NAME          DumpImage
 *
 *  DESCRIPTION   Outputs the image to a GIF-file.
 *
 *                No error checking is done! Probably not a very good
 *                example, then... :-)
 */
static void
DumpImage(void)
{
    printf("dumping image to file `%s'\n", FILENAME);

    /* create and set up the GIF-file */
    GIF_Create(FILENAME, WIDTH, HEIGHT, NUMCOLORS, COLORRES);

    /* define a few colors matching the pixel values used */
    GIF_SetColor(0, 0, 0, 0);        /* black, the background */
    GIF_SetColor(1, 255, 0, 0);      /* red */
    GIF_SetColor(2, 0, 255, 0);      /* green */
    GIF_SetColor(3, 0, 0, 255);      /* blue */
    GIF_SetColor(4, 255, 255, 255);  /* white */

    /* store the image, using the GetPixel function to extract pixel values */
    GIF_CompressImage(0, 0, -1, -1, GetPixel);

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
    srand(time(NULL));
    DrawImage();
    DumpImage();

    return 0;
}
