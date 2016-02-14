/*
 * Modification History
 *
 * 2001-September-15		Jason Rohrer
 * Created.
 *
 * 2006-July-3		Jason Rohrer
 * Changed to be an abstract base class.
 *
 * 2006-July-4		Jason Rohrer
 * Fixed behavior when mouse dragged back into button.
 * Made constructor protected.  Added disabled component support. 
 *
 * 2006-September-8		Jason Rohrer
 * Added mouse hover support.
 */
 
 
#ifndef BUTTON_GL_INCLUDED
#define BUTTON_GL_INCLUDED 

#include "GUIComponentGL.h"

#include "minorGems/ui/event/ActionListenerList.h"



/**
 * A base class for buttons in GL-based GUIs.
 *
 * @author Jason Rohrer
 */
class ButtonGL : public GUIComponentGL, public ActionListenerList {


	public:

		
		virtual ~ButtonGL();

        
        // subclasses must implement these

        /**
         * Draw this button's image in its pressed state.
         */
        virtual void drawPressed() = 0;

        
        /**
         * Draw this button's image in its unpressed state.
         */
        virtual void drawUnpressed() = 0;



        /**
         * Gets whether mouse is hovering.
         *
         * @return true if mouse is over this button.
         */
        virtual char isMouseOver();

        
        
		// override funxtions in GUIComponentGL
        virtual void mouseMoved( double inX, double inY );
		virtual void mouseDragged( double inX, double inY );
		virtual void mousePressed( double inX, double inY );
		virtual void mouseReleased( double inX, double inY );
		virtual void fireRedraw();


		
	protected:

        
        /**
		 * Constructs a button.
		 *
         * Should only be called by subclass constructors.
         *
		 * @param inAnchorX the x position of the upper left corner
		 *   of this component.
		 * @param inAnchorY the y position of the upper left corner
		 *   of this component.
		 * @param inWidth the width of this component.
		 * @param inHeight the height of this component.
		 */
		ButtonGL(
			double inAnchorX, double inAnchorY, double inWidth,
            double inHeight );

        
        char mPressed;

        char mHover;

    };



inline ButtonGL::ButtonGL(
	double inAnchorX, double inAnchorY, double inWidth, double inHeight )
	: GUIComponentGL( inAnchorX, inAnchorY, inWidth, inHeight ),
      mPressed( false ), mHover( false ) {

	}



inline ButtonGL::~ButtonGL() {
	}



inline char ButtonGL::isMouseOver() {
    if( ! isEnabled() ) {
        // never notice hover when disabled
        // and we may have not turned hover off after we became
        // disabled since we would have stopped getting mouse events

        // thus, we need to turn it off here so that when we are re-enabled,
        // we start back in the correct, non-hovering state
        mHover = false;
        }

    return mHover;
    }



inline void ButtonGL::mouseMoved( double inX, double inY ) {
	if( !isInside( inX, inY ) ) {
		// the mouse has been moved outside of us, so un-hover
		mHover = false;
		}
    else if( isEnabled() ){
        // moved back in, and we're enabled, so re-hover
        mHover = true;
        }
	}



inline void ButtonGL::mouseDragged( double inX, double inY ) {
	if( !isInside( inX, inY ) ) {
		// the mouse has been dragged outside of us, so unpress
		mPressed = false;
        mHover = false;
        }
    else if( isEnabled() ){
        // dragged back in, and we're enabled, so re-press
        mPressed = true;
        mHover = true;
        }
	}



inline void ButtonGL::mousePressed( double inX, double inY ) {
    if( isEnabled() ) {
        // we'll only get pressed events if the mouse is pressed on us
        // so we don't need to check isInside status
        mPressed = true;
        }
	}



inline void ButtonGL::mouseReleased( double inX, double inY ) {
	// always unpress on a release
	mPressed = false;
	
	if( isEnabled() && isInside( inX, inY ) ) {
		// fire an event
		fireActionPerformed( this );
		}
	}


		
inline void ButtonGL::fireRedraw() {

    if( mPressed ) {
        drawPressed();
        }
    else {
        drawUnpressed();
        }
	}



#endif



