#include "PNGImageConverter.h"

#include "minorGems/graphics/Image.h"

#include "minorGems/io/file/FileOutputStream.h"


int main() {

    int imageSize = 120;
    Image testImage( imageSize, imageSize, 3 );

    
    // red fades toward bottom
    // green fades toward right
    double *red = testImage.getChannel( 0 );
    double *green = testImage.getChannel( 1 );

    for( int y=0; y<imageSize; y++ ) {
        for( int x=0; x<imageSize; x++ ) {
            red[y*imageSize+x] = 1.0 - ( y / (double)imageSize );
            green[y*imageSize+x] = 1.0 - ( x / (double)imageSize );
            }
        }
    
    PNGImageConverter png;


    File outFileB( NULL, "test.png" );
    FileOutputStream outStreamB( &outFileB );
        
    png.formatImage( &testImage, &outStreamB );
    
    return 0;
    }
