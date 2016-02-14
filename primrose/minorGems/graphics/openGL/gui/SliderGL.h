/*
 * Modification History
 *
 * 2001-September-17		Jason Rohrer
 * Created.
 *
 * 2001-September-21		Jason Rohrer
 * Fixed a bug in thumb positioning in response to mouse position.
 *
 * 2001-September-23		Jason Rohrer
 * Fixed a bug in icon color clearing.
 */
 
 
#ifndef SLIDER_GL_INCLUDED
#define SLIDER_GL_INCLUDED 

#include "GUIComponentGL.h"

#include "minorGems/graphics/Image.h"
#include "minorGems/graphics/Color.h"

#include "minorGems/graphics/openGL/SingleTextureGL.h"

#include "minorGems/ui/event/ActionListenerList.h"

#include <GL/gl.h>


/**
 * A textured slider for GL-based GUIs.
 *
 * @author Jason Rohrer
 */
class SliderGL : public GUIComponentGL, public ActionListenerList {


	public:



		/**
		 * Constructs a slider.
		 *
		 * @param inAnchorX the x position of the upper left corner
		 *   of this component.
		 * @param inAnchorY the y position of the upper left corner
		 *   of this component.
		 * @param inWidth the width of this component.
		 * @param inHeight the height of this component.
		 * @param inIconImage the image to display
		 *   to the left of this slider.  Must have dimensions
		 *   that are powers of 2.
		 *   Will be destroyed when this class is destroyed.
		 *   If set to NULL, then no icon will be drawn.
		 * @param inIconWidthFraction the fraction of the slider's
		 *   width that should be taken up by the icon.
		 * @param inBarStartColor the color on the left end of
		 *   the slider bar.
		 *   Will be destroyed when this class is destroyed.
		 * @param inBarEndColor the color on the right end of
		 *   the slider bar.
		 *   Will be destroyed when this class is destroyed.
		 * @param inThumbColor the color of the slider thumb.
		 *   Will be destroyed when this class is destroyed.
		 * @param inBorderColor the color of the slider border.
		 *   Will be destroyed when this class is destroyed.
		 */
		SliderGL(
			double inAnchorX, double inAnchorY, double inWidth,
			double inHeight, Image *inIconImage,
			double inIconWidthFraction,
			Color *inBarStartColor, Color *inBarEndColor,
			Color *inThumbColor, Color *inBorderColor );


		
		~SliderGL();


		
		/**
		 * Gets the position of the slider thumb.
		 *
		 * @return the thumb position, in [0,1.0].
		 */
		double getThumbPosition();



		/**
		 * Sets the position of the slider thumb.
		 *
		 * Note that if the slider thumb position changes
		 * as a result of this call, then an action
		 * will be fired to all registered listeners.
		 *
		 * @param inPosition the thumb position, in [0,1.0].
		 */
		void setThumbPosition( double inPosition );

		
		
		// override funxtions in GUIComponentGL
		virtual void mouseDragged( double inX, double inY );
		virtual void mousePressed( double inX, double inY );
		virtual void mouseReleased( double inX, double inY );
		virtual void fireRedraw();


		
	protected:

		SingleTextureGL *mIconTexture;
		double mIconWidthFraction;
		
		Color *mBarStartColor;
		Color *mBarEndColor;
		Color *mThumbColor;
		Color *mBorderColor; 

		double mThumbPosition;

		// true iff the slider is currently being dragged
		char mDragging;
	};



inline SliderGL::SliderGL(
	double inAnchorX, double inAnchorY, double inWidth,
	double inHeight, Image *inIconImage, double inIconWidthFraction,
	Color *inBarStartColor, Color *inBarEndColor,
	Color *inThumbColor, Color *inBorderColor )
	: GUIComponentGL( inAnchorX, inAnchorY, inWidth, inHeight ),
	  mIconWidthFraction( inIconWidthFraction ),
	  mBarStartColor( inBarStartColor ),
	  mBarEndColor( inBarEndColor ),
	  mThumbColor( inThumbColor ),
	  mBorderColor( inBorderColor ),
	  mThumbPosition( 0.5 ), mDragging( false ) {

	if( inIconImage != NULL ) {
		mIconTexture = new SingleTextureGL( inIconImage );
		// the image is no longer needed
		delete inIconImage;
		}
	else {
		mIconTexture = NULL;
		}
	}



inline SliderGL::~SliderGL() {
	if( mIconTexture != NULL ) {
		delete mIconTexture;
		}

	delete mBarStartColor;
	delete mBarEndColor;
	delete mThumbColor;
	delete mBorderColor; 
	}



inline double SliderGL::getThumbPosition() {
	return mThumbPosition;
	}



inline void SliderGL::setThumbPosition( double inPosition ) {
	if( mThumbPosition != inPosition ) {
		mThumbPosition = inPosition;
		fireActionPerformed( this );
		}
	}



inline void SliderGL::mouseDragged( double inX, double inY ) {
	if( mDragging ) {

		double barWidth = mWidth * ( 1 - mIconWidthFraction );
		double iconEndX = mAnchorX + mWidth - barWidth;

		double thumbWidth = 0.1 * barWidth;

		// we want the mouse centered over the thumb
		double thumbUsableBarWidth = barWidth - thumbWidth;

		
		mThumbPosition = ( inX - ( iconEndX + 0.5 * thumbWidth ) )
			/ thumbUsableBarWidth;

		if( mThumbPosition > 1 ) {
			mThumbPosition = 1;
			}
		else if( mThumbPosition < 0 ) {
			mThumbPosition = 0;
			}

		// fire to listeners
		fireActionPerformed( this );
		}
	}



inline void SliderGL::mousePressed( double inX, double inY ) {
	mDragging = true;
	}



inline void SliderGL::mouseReleased( double inX, double inY ) {
	// always stop dragging on a release
	mDragging = false;
	}


		
inline void SliderGL::fireRedraw() {

	// these values will change below if there is an icon
	double barStartX = mAnchorX;
	double barWidth = mWidth;

	if( mIconTexture != NULL ){
		// first, draw the icon

	
		// set our texture
		mIconTexture->enable();

		double textureStartX = mAnchorX;
		double textureStartY = mAnchorY;

		double textureEndX = mAnchorX + mIconWidthFraction * mWidth;
		double textureEndY = mAnchorY + mHeight;
	
		glBegin( GL_QUADS ); {
			// make sure our color is set to white for our texture
			// (to avoid leftover colors)
			glColor3f( 1.0, 1.0, 1.0 );
			
			glTexCoord2f( 0, 1.0f );
			glVertex2d( textureStartX, textureStartY );

			glTexCoord2f( 1.0f, 1.0f );
			glVertex2d( textureEndX, textureStartY ); 

			glTexCoord2f( 1.0f, 0 );
			glVertex2d( textureEndX, textureEndY );
		
			glTexCoord2f( 0, 0 );
			glVertex2d( textureStartX, textureEndY );
			}
		glEnd();

		mIconTexture->disable();


		barWidth = mWidth * ( 1 - mIconWidthFraction );
		barStartX = textureEndX;

		} // end check for a non-NULL icon texture

	
	// now, draw the slider bar

	
	double barHeight = 0.5 * mHeight;

	// center the bar vertically
	double barStartY = mAnchorY + ( mHeight - barHeight ) * 0.5;
	
	// draw its gradient-filled center
	glBegin( GL_QUADS ); {
		
		// start of bar
		glColor3f( mBarStartColor->r,
				   mBarStartColor->g, mBarStartColor->b );
		glVertex2d( barStartX, barStartY );		
		glVertex2d( barStartX, barStartY + barHeight );

		// end of bar
		glColor3f( mBarEndColor->r,
				   mBarEndColor->g, mBarEndColor->b );
		glVertex2d( barStartX + barWidth, barStartY + barHeight );
		glVertex2d( barStartX + barWidth, barStartY ); 

		}
	glEnd();


	
	// draw it's border
	glColor3f( mBorderColor->r,
			   mBorderColor->g, mBorderColor->b );
	glBegin( GL_LINE_LOOP ); {
		glVertex2d( barStartX, barStartY );		
		glVertex2d( barStartX, barStartY + barHeight );
		
		glVertex2d( barStartX + barWidth, barStartY + barHeight );
		glVertex2d( barStartX + barWidth, barStartY ); 
		}
	glEnd();

	// now draw the thumb

	// draw a thumb that is 1/10th as wide as the bar
	double thumbWidth = 0.1 * barWidth;

	// we don't want the thumb going off the ends of the bar
	double thumbUsableBarWidth = barWidth - thumbWidth;
	
	double thumbStartX = mThumbPosition * thumbUsableBarWidth
		+ barStartX;
	double thumbEndX = thumbStartX + thumbWidth;

	glColor3f( mThumbColor->r,
				   mThumbColor->g, mThumbColor->b );
	glBegin( GL_QUADS ); {
		glVertex2d( thumbStartX, mAnchorY );		
		glVertex2d( thumbStartX, mAnchorY + mHeight );

		glVertex2d( thumbEndX, mAnchorY + mHeight );
		glVertex2d( thumbEndX, mAnchorY ); 

		}
	glEnd();

	// draw it's border
	glColor3f( mBorderColor->r,
			   mBorderColor->g, mBorderColor->b );
	glBegin( GL_LINE_LOOP ); {
		glVertex2d( thumbStartX, mAnchorY );		
		glVertex2d( thumbStartX, mAnchorY + mHeight );

		glVertex2d( thumbEndX, mAnchorY + mHeight );
		glVertex2d( thumbEndX, mAnchorY );
		}
	glEnd();

	}



#endif



