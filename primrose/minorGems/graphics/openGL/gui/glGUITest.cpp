/*
 * Modification History
 *
 * 2001-September-15		Jason Rohrer
 * Created.
 * Added support for testing ButtonGL.
 *
 * 2001-September-16		Jason Rohrer
 * Added support for testing StickyButtonGL.
 * Added support for testing MultiButtonGL.
 * Added support for testing MouseCursorRegionGL.
 *
 * 2001-September-16		Jason Rohrer
 * Added support for testing SliderGL.
 */

#include "SliderGL.h"
#include "MouseCursorRegionGL.h"
#include "MultiButtonGL.h"
#include "GUIPanelGL.h"
#include "GUIContainerGL.h"
#include "GUITranslatorGL.h"
#include "ButtonGL.h"
#include "StickyButtonGL.h"

#include "minorGems/graphics/Color.h"

#include "minorGems/graphics/openGL/ScreenGL.h"

#include "minorGems/graphics/converters/BMPImageConverter.h"

#include "minorGems/io/file/File.h"
#include "minorGems/io/file/FileInputStream.h"

// prototype
/**
 * Loads an image from a BMP file.
 *
 * @param inFileName the name of the file to load from.
 *
 * @return the loaded image, or NULL if reading from the file fails.
 */
Image *loadBMPImage( char *inFileName );




// a test program for gl-based guis
int main() {

	Image *buttonUpImage = loadBMPImage( "test.bmp" );
	if( buttonUpImage == NULL ) {
		printf( "image loading failed.\n" );
		return 1;
		}
	
	Image *buttonDownImage = loadBMPImage( "test2.bmp" );
	if( buttonUpImage == NULL ) {
		printf( "image loading failed.\n" );
		return 1;
		}

	Image *sliderImage = loadBMPImage( "test3.bmp" );
	if( sliderImage == NULL ) {
		printf( "image loading failed.\n" );
		return 1;
		}

	// construct the screen first so that other GL calls work
	ScreenGL *screen = new ScreenGL( 400, 300, false, "Test GL GUI" );

	// construct a button with these images
	ButtonGL *button =
		new ButtonGL( 0.45, 0.45, 0.1, 0.1, buttonUpImage, buttonDownImage );

	StickyButtonGL *stickyButton =
		new StickyButtonGL( 0.45, 0.56, 0.1, 0.1,
							buttonUpImage->copy(),
							buttonDownImage->copy() );

	int numMultiButton = 4;
	Image **pressedImages = new Image*[numMultiButton];
	Image **unpressedImages = new Image*[numMultiButton];

	for( int i=0; i<numMultiButton; i++ ) {
		pressedImages[i] = buttonDownImage->copy();
		unpressedImages[i] = buttonUpImage->copy();
		}
	
	double gutterFraction = 0.125;

	MultiButtonGL *multiButton =
		new MultiButtonGL(  0.4, 0.67, 0.2, 0.2,
							numMultiButton,
							unpressedImages, pressedImages,
							gutterFraction );

	SliderGL *slider =
		new SliderGL(
			0.35, 0.34, 0.3,
			0.1, sliderImage,
			0.33333,
			new Color( 0, 0, 0), new Color( 1.0f, 0, 0 ),
			new Color( 0.5f, 0.5f, 0.5f ),
			new Color( 0.6f, 0.6f, 0.6f ) );
	
	
	GUIContainerGL *container = new GUIContainerGL( 0, 0, 1, 1 );

	GUITranslatorGL *translator = new GUITranslatorGL( container, screen );

	screen->addRedrawListener( translator );
	screen->addMouseHandler( translator );
	
	GUIPanelGL *panel = new GUIPanelGL( 0.25, 0.25, 0.5, 0.7,
										new Color( 0.25f,
												   0.25f, 0.25f ) );

	panel->add( button );
	panel->add( stickyButton );
	panel->add( multiButton );
	panel->add( slider );
	
	container->add( panel );

	MouseCursorRegionGL *cursor =
		new MouseCursorRegionGL( 0, 0, 1.0, 1.0, 0.125,
								 new Color( 0, 0, 1.0f ),
								 new Color( 0, 0, .75f ) );

	container->add( cursor );

	
	GUIPanelGL *panel2 = new GUIPanelGL( 0, 0, 0.25, 1,
										 new Color( 0, 1.0f, 0 ) );

	container->add( panel2 );

	container->add( cursor );
	
	screen->start();
	

	delete translator;  // this deletes container too
	delete screen;


	return 0;
	}



Image *loadBMPImage( char *inFileName ) {
	// load images for the button
	File *imageFile = new File( NULL, inFileName,
									  strlen( inFileName ) );
	FileInputStream *imageStream
		= new FileInputStream( imageFile );

	BMPImageConverter *converter = new BMPImageConverter();

	Image *returnImage = converter->deformatImage( imageStream );

	delete imageFile;
	delete imageStream;
	delete converter;

	return returnImage;
	}
