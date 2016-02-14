#include "common.h"

#include "minorGems/graphics/converters/TGAImageConverter.h"

#include "minorGems/io/file/File.h"

#include "minorGems/io/file/FileInputStream.h"


#include <math.h>



Image *readTGA( char *inFileName ) {
    return readTGA( "graphics", inFileName );
    }



Image *readTGA( char *inFolderName, char *inFileName ) {
    File tgaFile( new Path( inFolderName ), inFileName );
    FileInputStream tgaStream( &tgaFile );
    
    TGAImageConverter converter;
    
    return converter.deformatImage( &tgaStream );
    }



