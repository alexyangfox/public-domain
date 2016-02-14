
/**************************************************************************
 *
 *  FILE:           TGASAVE.C
 *
 *  MODULE OF:      TGASAVE
 *
 *  DESCRIPTION:    Routines to create a Truevision Targa (TGA) -file.
 *                  See TGASAVE.DOC for a description . . .
 *
 *                  The functions were originally written using Borland's
 *                  C-compiler on an IBM PC -compatible computer, but they
 *                  are compiled and tested on SunOS (Unix) as well.
 *
 *  WRITTEN BY:     Sverre H. Huseby
 *                  Bjoelsengt. 17
 *                  N-0468 Oslo
 *                  Norway
 *
 *                  sverrehu@ifi.uio.no
 *
 *  LAST MODIFIED:
 *
 **************************************************************************/



#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tgasave.h"



/**************************************************************************
 *                                                                        *
 *                       P R I V A T E    D A T A                         *
 *                                                                        *
 **************************************************************************/

typedef unsigned Word;          /* At least two bytes (16 bits) */
typedef unsigned char Byte;     /* Exactly one byte (8 bits) */


/*========================================================================*
 =                                                                        =
 =                             I/O Routines                               =
 =                                                                        =
 *========================================================================*/

static FILE
    *OutFile;                   /* File to write to */


/*========================================================================*
 =                                                                        =
 =                               Main routines                            =
 =                                                                        =
 *========================================================================*/

static char
    ImageID[256],
    Author[41],
    SoftwareID[41];
static Word
    ImageHeight,
    ImageWidth,
    AspectWidth,
    AspectHeight = 0;
static void
    (*GetPixel)(int x, int y, TGA_Color *c);





/**************************************************************************
 *                                                                        *
 *                   P R I V A T E    F U N C T I O N S                   *
 *                                                                        *
 **************************************************************************/

/*========================================================================*
 =                                                                        =
 =                         Routines to do file IO                         =
 =                                                                        =
 *========================================================================*/

/*-------------------------------------------------------------------------
 *
 *  NAME:           Create()
 *
 *  DESCRIPTION:    Creates a new file, and enables referencing using the
 *                  global variable OutFile. This variable is only used
 *                  by these IO-functions, making it relatively simple to
 *                  rewrite file IO.
 *
 *  PARAMETERS:     filename   Name of file to create.
 *
 *  RETURNS:        TGA_OK         OK
 *                  TGA_ERRWRITE   Error opening the file.
 *
 */
static int Create(char *filename)
{
    if ((OutFile = fopen(filename, "wb")) == NULL)
        return TGA_ERRCREATE;

    return TGA_OK;
}



/*-------------------------------------------------------------------------
 *
 *  NAME:           Write()
 *
 *  DESCRIPTION:    Output bytes to the current OutFile.
 *
 *  PARAMETERS:     buf   Pointer to buffer to write.
 *                  len   Number of bytes to write.
 *
 *  RETURNS:        TGA_OK         OK
 *                  TGA_ERRWRITE   Error writing to the file.
 *
 */
static int Write(void *buf, unsigned len)
{
    if (fwrite(buf, sizeof(Byte), len, OutFile) < len)
        return TGA_ERRWRITE;

    return TGA_OK;
}



/*-------------------------------------------------------------------------
 *
 *  NAME:           WriteByte()
 *
 *  DESCRIPTION:    Output one byte to the current OutFile.
 *
 *  PARAMETERS:     b   Byte to write.
 *
 *  RETURNS:        TGA_OK         OK
 *                  TGA_ERRWRITE   Error writing to the file.
 *
 */
static int WriteByte(Byte b)
{
    if (putc(b, OutFile) == EOF)
        return TGA_ERRWRITE;

    return TGA_OK;
}



/*-------------------------------------------------------------------------
 *
 *  NAME:           WriteWord()
 *
 *  DESCRIPTION:    Output one word (2 bytes with byte-swapping, like on
 *                  the IBM PC) to the current OutFile.
 *
 *  PARAMETERS:     w   Word to write.
 *
 *  RETURNS:        TGA_OK         OK
 *                  TGA_ERRWRITE   Error writing to the file.
 *
 */
static int WriteWord(Word w)
{
    if (putc(w & 0xFF, OutFile) == EOF)
        return TGA_ERRWRITE;

    if (putc((w >> 8), OutFile) == EOF)
        return TGA_ERRWRITE;

    return TGA_OK;
}



/*-------------------------------------------------------------------------
 *
 *  NAME:           WriteZeros()
 *
 *  DESCRIPTION:    Output the given number of zero-bytes to the current
 *                  OutFile.
 *
 *  PARAMETERS:     n   Number of bytes to write.
 *
 *  RETURNS:        TGA_OK         OK
 *                  TGA_ERRWRITE   Error writing to the file.
 *
 */
static int WriteZeros(int n)
{
    while (n--)
        if (putc(0, OutFile) == EOF)
            return TGA_ERRWRITE;

    return TGA_OK;
}



/*-------------------------------------------------------------------------
 *
 *  NAME:           GetFilePos()
 *
 *  DESCRIPTION:    Get the current position in the current OutFile.
 *
 *  PARAMETERS:     None
 *
 *  RETURNS:        The byte-offset from the start of the file.
 *
 */
static long GetFilePos(void)
{
    return ftell(OutFile);
}



/*-------------------------------------------------------------------------
 *
 *  NAME:           Close()
 *
 *  DESCRIPTION:    Close current OutFile.
 *
 *  PARAMETERS:     None
 *
 *  RETURNS:        Nothing
 *
 */
static void Close(void)
{
    fclose(OutFile);
}



/*========================================================================*
 =                                                                        =
 =                              Other routines                            =
 =                                                                        =
 *========================================================================*/

/*-------------------------------------------------------------------------
 *
 *  NAME:           WriteFileHeader()
 *
 *  DESCRIPTION:    Output the Targa file header to the current TGA-file
 *
 *  PARAMETERS:     None
 *
 *  RETURNS:        TGA_OK         OK
 *                  TGA_ERRWRITE   Error writing to the file.
 *
 */
static int WriteFileHeader(void)
{
    /*
     *  Start with the lengt of the Image ID string.
     */
    if (WriteByte(strlen(ImageID)) != TGA_OK)
        return TGA_ERRWRITE;

    /*
     *  Then the Color Map Type. There's no color map here (0).
     */
    if (WriteByte(0) != TGA_OK)
        return TGA_ERRWRITE;

    /*
     *  And then the Image Type. Uncompressed True Color (2) is
     *  the only one supported by TGASAVE.
     */
    if (WriteByte(2) != TGA_OK)
        return TGA_ERRWRITE;

    /*
     *  The 5 Color Map Specification bytes should be 0, since we
     *  don'e use no color table.
     */
    if (WriteZeros(5) != TGA_OK)
        return TGA_ERRWRITE;

    /*
     *  Now we should output the image specification. The last byte
     *  written (32) indicates that the first pixel is upper left on
     *  the screen. TGASAVE supports 24-bit pixels only.
     */
    if (WriteWord(0) != TGA_OK || WriteWord(0) != TGA_OK)
        return TGA_ERRWRITE;
    if (WriteWord(ImageWidth) != TGA_OK || WriteWord(ImageHeight) != TGA_OK)
        return TGA_ERRWRITE;
    if (WriteByte(24) != TGA_OK || WriteByte(32) != TGA_OK)
        return TGA_ERRWRITE;

    /*
     *  Now we write the Image ID string.
     */
    if (Write(ImageID, strlen(ImageID)) != TGA_OK)
        return TGA_ERRWRITE;

    /*
     *  This is where the color map should go if this was not
     *  True Color. (Wery interresting...)
     */

    return TGA_OK;
}



/*-------------------------------------------------------------------------
 *
 *  NAME:           WriteImageData()
 *
 *  DESCRIPTION:    Write the image data to the file by calling the
 *                  callback function.
 *
 *  PARAMETERS:     None
 *
 *  RETURNS:        TGA_OK         OK
 *                  TGA_ERRWRITE   Error writing to the file.
 *
 */
static int WriteImageData(void)
{
    Word x, y;
    TGA_Color col;


    for (y = 0; y < ImageHeight; y++)
        for (x = 0; x < ImageWidth; x++) {
            GetPixel(x, y, &col);
            if (WriteByte(col.b) != TGA_OK
                || WriteByte(col.g) != TGA_OK
                || WriteByte(col.r) != TGA_OK)
                return TGA_ERRWRITE;
        }

    return TGA_OK;
}



/*-------------------------------------------------------------------------
 *
 *  NAME:           WriteFileFooter()
 *
 *  DESCRIPTION:    Output the Targa file footer to the current TGA-file.
 *                  This function actually outputs all that follows the
 *                  image data.
 *
 *  PARAMETERS:     None
 *
 *  RETURNS:        TGA_OK         OK
 *                  TGA_ERRWRITE   Error writing to the file.
 *
 */
static int WriteFileFooter(void)
{
    long extoffs = 0L;          /* Extension file offset. */


    /*
     *  There's no Developer Area.
     */

    /*
     *  If any special info is given by the TGA_Set...()-functions,
     *  we need an Extension Area.
     */
    if (strlen(Author) || strlen(SoftwareID) || AspectHeight) {
        extoffs = GetFilePos();

        /*
         *  The Extension Area Size is fixed.
         */
        if (WriteWord(495) != TGA_OK)
            return TGA_ERRWRITE;

        /*
         *  Output all fields. Some are not used by this program.
         */
        if (Write(Author, sizeof(Author)) != TGA_OK)
            return TGA_ERRWRITE;
        if (WriteZeros(
                324             /* Author Comments */
              +  12             /* Date/Time Stamp */
              +  41             /* Job Name/ID */
              +   6             /* Job Time */
            ) != TGA_OK)
            return TGA_ERRWRITE;
        if (Write(SoftwareID, sizeof(SoftwareID)) != TGA_OK)
            return TGA_ERRWRITE;
        if (WriteZeros(
                  3             /* Software Version */
              +   4             /* Key Color */
            ) != TGA_OK)
            return TGA_ERRWRITE;
        if (WriteWord(AspectWidth) != TGA_OK
            || WriteWord(AspectHeight) != TGA_OK)
            return TGA_ERRWRITE;
        if (WriteZeros(
                  4             /* Gamma Value */
              +   4             /* Color Correction Offset */
              +   4             /* Postage Stamp Offset */
              +   1             /* Attributs Type */
            ) != TGA_OK)
            return TGA_ERRWRITE;

    }

    /*
     *  Now we get to the actual footer. Write offset of Extension
     *  Area and Developer Area, followed by the signature.
     */
    if (WriteWord(extoffs) != TGA_OK)
        return TGA_ERRWRITE;
    if (WriteWord(0L) != TGA_OK)
        return TGA_ERRWRITE;
    if (Write("TRUEVISION-XFILE.", 18) != TGA_OK)
        return TGA_ERRWRITE;

    return TGA_OK;
}





/**************************************************************************
 *                                                                        *
 *                    P U B L I C    F U N C T I O N S                    *
 *                                                                        *
 **************************************************************************/

/*-------------------------------------------------------------------------
 *
 *  NAME:           TGA_SetImageID()
 *
 *  DESCRIPTION:    Set the image ID string of the file. This is a string
 *                  of up to 255 bytes.
 *
 *  PARAMETERS:     s   The image ID. If none is needed, this function
 *                      need not be called.
 *
 *  RETURNS:        Nothing
 *
 */
void TGA_SetImageID(char *s)
{
    memset(ImageID, 0, sizeof(ImageID));
    strncpy(ImageID, s, sizeof(ImageID) - 1);
}



/*-------------------------------------------------------------------------
 *
 *  NAME:           TGA_SetAuthor()
 *
 *  DESCRIPTION:    Set the name of the image author. This is a string
 *                  of up to 40 bytes.
 *
 *  PARAMETERS:     s   The author name. If none is needed, this function
 *                      need not be called.
 *
 *  RETURNS:        Nothing
 *
 */
void TGA_SetAuthor(char *s)
{
    memset(Author, 0, sizeof(Author));
    strncpy(Author, s, sizeof(Author) - 1);
}



/*-------------------------------------------------------------------------
 *
 *  NAME:           TGA_SetSoftwareID()
 *
 *  DESCRIPTION:    Set the name of the program that generated the image.
 *                  This is a string of up to 40 bytes.
 *
 *  PARAMETERS:     s   The program name. If none is needed, this function
 *                      need not be called.
 *
 *  RETURNS:        Nothing
 *
 */
void TGA_SetSoftwareID(char *s)
{
    memset(SoftwareID, 0, sizeof(SoftwareID));
    strncpy(SoftwareID, s, sizeof(SoftwareID) - 1);
}



/*-------------------------------------------------------------------------
 *
 *  NAME:           TGA_SetAspect()
 *
 *  DESCRIPTION:    Set the pixel aspect ratio.
 *
 *  PARAMETERS:     width    Pixel width.
 *                  height   Pixel height.
 *
 *                  If no aspect ratio is needed, this function need not
 *                  be called.
 *
 *                  If width==height, the pixels are square.
 *
 *  RETURNS:        Nothing
 *
 */
void TGA_SetAspect(int width, int height)
{
    AspectWidth = width;
    AspectHeight = height;
}



/*-------------------------------------------------------------------------
 *
 *  NAME:           TGA_Save()
 *
 *  DESCRIPTION:    Create the TGA-file.
 *
 *                  The pixels are retrieved using a user defined callback
 *                  function. This function should accept three parameters,
 *                  x and y, specifying which pixel to retrieve, and col,
 *                  a pointer to TGA_Color -structure to fill in for return.
 *                  The pixel values sent to this function are as follows:
 *
 *                    x : [0, ImageWidth - 1]
 *                    y : [0,w ImageHeight - 1]
 *
 *                  The function should fill in the color-structure, and
 *                  all three color components (red, green and blur) should
 *                  be in the interval [0, 255].
 *
 *  PARAMETERS:     filename    Name of file to create (including extension).
 *                  width       Number of horisontal pixels in the image.
 *                  height      Number of vertical pixels in the image.
 *                  getpixel    Address of user defined callback function.
 *                              (See above)
 *
 *  RETURNS:        TGA_OK          OK
 *                  TGA_ERRCREATE   Couldn't create file.
 *                  TGA_ERRWRITE    Error writing to the file.
 *
 */
int TGA_Save(char *filename, int width, int height,
             void (*getpixel)(int x, int y, TGA_Color *col))
{
    /*
     *  Initiate variables for new TGA-file
     */
    ImageHeight = height;
    ImageWidth = width;
    GetPixel = getpixel;

    /*
     *  Create file specified
     */
    if (Create(filename) != TGA_OK)
        return TGA_ERRCREATE;

    /*
     *  Write file header.
     */
    if (WriteFileHeader() != TGA_OK)
        return TGA_ERRWRITE;

    /*
     *  Write the image data.
     */
    if (WriteImageData() != TGA_OK)
        return TGA_ERRWRITE;

    /*
     *  Write footer, and close the file.
     */
    if (WriteFileFooter() != TGA_OK)
        return TGA_ERRWRITE;
    Close();

    return TGA_OK;
}
