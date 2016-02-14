#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED



#include "minorGems/graphics/Image.h"

//#include <SDL/SDL.h>

// reads a TGA file from the default ("graphics") folder
Image *readTGA( char *inFileName );


Image *readTGA( char *inFolderName, char *inFileName );



#endif
