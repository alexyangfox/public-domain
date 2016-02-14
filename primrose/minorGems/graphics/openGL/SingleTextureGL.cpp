/*
 * Modification History
 *
 * 2005-February-21   Jason Rohrer
 * Created.
 *
 * 2005-August-24	  Jason Rohrer
 * Added control over texture wrapping.
 */



#include "SingleTextureGL.h"



void SingleTextureGL::setTextureData( Image *inImage ) {
	
	// first, convert our image to an RGBAImage
	RGBAImage *rgbaImage = new RGBAImage( inImage );

	if( inImage->getNumChannels() < 4 ) {
		// we should fill in 1.0 for the alpha channel
		// since the input image doesn't have an alpha channel
		double *channel = rgbaImage->getChannel( 3 );

		int numPixels = inImage->getWidth() * inImage->getHeight();

		for( int i=0; i<numPixels; i++ ) {
			channel[i] = 1.0;
			}
		}
    
	// extract the rgba data
	unsigned char *textureData = rgbaImage->getRGBABytes();

    int error;
    
	GLenum texFormat = GL_RGBA;
	glBindTexture( GL_TEXTURE_2D, mTextureID );

    error = glGetError();
	if( error != GL_NO_ERROR ) {		// error
		printf( "Error binding to texture id %d, error = %d\n",
                (int)mTextureID,
                error );
		}
    
    
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    if( mRepeat ) {
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
        }
    else {
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
        }
    
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	glTexImage2D( GL_TEXTURE_2D, 0,
				  texFormat, inImage->getWidth(),
				  inImage->getHeight(), 0,
				  texFormat, GL_UNSIGNED_BYTE, textureData );

	error = glGetError();
	if( error != GL_NO_ERROR ) {		// error
		printf( "Error setting texture data for id %d, error = %d, \"%s\"\n",
                (int)mTextureID, error, glGetString( error ) );
        printf( "Perhaps texture image width or height is not a power of 2\n"
                "Width = %lu, Height = %lu\n",
                inImage->getWidth(), inImage->getHeight() );
		}
    
	delete rgbaImage;
	delete [] textureData;
	}

