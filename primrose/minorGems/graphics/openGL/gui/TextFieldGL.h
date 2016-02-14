/*
 * Modification History
 *
 * 2001-October-29	Jason Rohrer
 * Created.
 *
 * 2001-November-2	Jason Rohrer
 * Added support for cursor position.
 * Added support for clicking to reposition the cursor.
 * Fixed an array out-of-bounds bug in the keyReleased function.
 *
 * 2008-September-29	Jason Rohrer
 * Improved text width handling and cursor spacing.
 * Added length limit.
 *
 * 2008-October-27	Jason Rohrer
 * Switched to implementation-independent keycodes.
 * Switched to key press (instead of release) events.
 *
 * 2008-November-1	Jason Rohrer
 * Added support for forced upper case.
 */
 
 
#ifndef TEXT_FIELD_GL_INCLUDED
#define TEXT_FIELD_GL_INCLUDED 

#include "GUIComponentGL.h"
#include "TextGL.h"

#include "minorGems/graphics/Color.h"

#include "minorGems/util/stringUtils.h"




/**
 * A text field for OpenGL-based GUIs.
 *
 * @author Jason Rohrer
 */
class TextFieldGL : public GUIComponentGL {


	public:



		/**
		 * Constructs a text field.
		 *
		 * @param inAnchorX the x position of the upper left corner
		 *   of this component.
		 * @param inAnchorY the y position of the upper left corner
		 *   of this component.
		 * @param inWidth the width of this component.
		 * @param inHeight the height of this component.
		 * @param inString the text to display in this text field.
		 *    Is copied internally, so must be destroyed
		 *    by caller if not const.
		 * @param inText the text object to use when drawing
		 *   this textField.  Must be destroyed by caller after
		 *   this class is destroyed.
		 * @param inBorderColor the color of this field's border.
		 *   Will be destroyed when this class is destroyed.
		 * @param inFocusedBorderColor the color of this field's border
		 *   when the field is focused.
		 *   Will be destroyed when this class is destroyed.
		 * @param inBackgroundColor the color of this field's background.
		 *   Will be destroyed when this class is destroyed.
		 * @param inCursorColor the color of this field's cursor line.
		 *   Will be destroyed when this class is destroyed.
         * @param inLengthLimit the length limit for the entered string,
         *   or -1 for no limit.  Defaults to -1.
         * @param inForceUppercase true to force upper case for text entered.
         *   Defaults to false.
		 */
		TextFieldGL(
			double inAnchorX, double inAnchorY, double inWidth,
			double inHeight, char *inString, TextGL *inText,
			Color *inBorderColor, Color *inFocusedBorderColor,
			Color *inBackgroundColor, Color *inCursorColor,
            int inLengthLimit = -1,
            char inForceUppercase = false );


		
		~TextFieldGL();


		
		/**
		 * Sets the text displayed by this text field.
		 *
		 * @param inString the text to display in this text field.
		 *    Is copied internally, so must be destroyed
		 *    by caller if not const.
		 */
		void setText( char *inString );


		
		/**
		 * Gets the text displayed by this text field.
		 *
		 * @return the text to display in this textField.
		 *    Must not be destroyed or modified by caller.
		 */
		char *getText();


		
		// override functions in GUIComponentGL
		virtual void setFocus( char inFocus );
		virtual char isFocused();
		virtual void keyPressed( unsigned char inKey, double inX, double inY );
		virtual void specialKeyPressed( int inKey, double inX, double inY );
		virtual void keyReleased( unsigned char inKey,
								  double inX, double inY );
		virtual void specialKeyReleased( int inKey, double inX, double inY );

		virtual void mouseReleased( double inX, double inY );
		
		virtual void fireRedraw();


		
	protected:


		
		/**
		 * Adjusts the cursor position to bring it in bounds of the
		 * current string.
		 */
		void fixCursorPosition();

		
		
		TextGL *mText;

		char *mString;

		Color *mBorderColor;
		Color *mFocusedBorderColor;
		Color *mBackgroundColor;
		Color *mCursorColor;
		
		Color *mCurrentBorderColor;
		
        int mLengthLimit;
        char mForceUppercase;
        
		char mFocused;

		// cursor position, in number of characters
		int mCursorPosition;
	};



inline TextFieldGL::TextFieldGL(
	double inAnchorX, double inAnchorY, double inWidth,
	double inHeight, char *inString, TextGL *inText,
	Color *inBorderColor, Color *inFocusedBorderColor,
	Color *inBackgroundColor, Color *inCursorColor,
    int inLengthLimit,
    char inForceUppercase )
	: GUIComponentGL( inAnchorX, inAnchorY, inWidth, inHeight ),
	  mText( inText ), mString( NULL ),
	  mBorderColor( inBorderColor ),
	  mFocusedBorderColor( inFocusedBorderColor ),
	  mBackgroundColor( inBackgroundColor ),
	  mCursorColor( inCursorColor ),
	  mCurrentBorderColor( inBorderColor ),
      mLengthLimit( inLengthLimit ),
      mForceUppercase( inForceUppercase ),
	  mFocused( false ),
	  mCursorPosition( 0 ) {

	setText( inString );
	}



inline TextFieldGL::~TextFieldGL() {
	if( mString != NULL ) {
		delete [] mString;
		}

	delete mBorderColor;
	delete mFocusedBorderColor;
	delete mBackgroundColor;
	delete mCursorColor;
	}



inline void TextFieldGL::setText( char *inString ) {
	if( mString != NULL ) {
		delete [] mString;
		}
	int length = strlen( inString ) + 1;
	
	mString = new char[ length ];

	memcpy( mString, inString, length );
	}



inline char *TextFieldGL::getText() {
	return mString;
	}



inline void TextFieldGL::setFocus( char inFocus ) {
	mFocused = inFocus;

	if( mFocused ) {
		mCurrentBorderColor = mFocusedBorderColor;
		}
	else {
		mCurrentBorderColor = mBorderColor;
		}
	}



inline char TextFieldGL::isFocused() {
	return mFocused;
	}



inline void TextFieldGL::keyPressed(
	unsigned char inKey, double inX, double inY ) {

    //printf( "key press: %d\n", inKey );
	
	// backspace and delete
	if( inKey == 127 || inKey == 8 ) {
		if( mCursorPosition != 0 ) {
			int length = strlen( mString );

			if( length != 0 ) {
				char *newString = new char[ length ];

				// copy mString up to the last char before the deleted char
				memcpy( newString, mString, mCursorPosition-1 );

				// copy the portion of mString after the deleted char
				// this will include the trailing \0
				memcpy( &( newString[mCursorPosition-1] ),
						&( mString[mCursorPosition] ),
						length - mCursorPosition + 1 );
			
				setText( newString );
			
				delete [] newString;

				mCursorPosition--;
				}
			}
		}
	// allowable character key, from space up to tilde
    // ignore after length limit reached
	else if( inKey >= 32 && inKey <= 126 && 
             ( mLengthLimit < 0 || (int)strlen( mString ) < mLengthLimit ) ) {
		// add a character to our string

		int oldStringLength = strlen( mString ) + 2; 
		int length = oldStringLength + 2;
		
		char *newString = new char[ length ];

		
		if( mCursorPosition != 0 ) {
			// copy chars up to cursor position
			memcpy( newString, mString, mCursorPosition );
			}
		// copy chars after cursor position, including trailing \0
		memcpy( &( newString[mCursorPosition+1] ),
				&( mString[mCursorPosition] ),
				oldStringLength - mCursorPosition + 1 );

		

        if( mForceUppercase ) {
            inKey = toupper( inKey );
            }

        // now stick in the inserted char
		newString[ mCursorPosition ] = inKey;
		
		setText( newString );
		
		delete [] newString;

		mCursorPosition++;
		}
	
	// else a non-valid key hit

	}



inline void TextFieldGL::specialKeyPressed(
	int inKey, double inX, double inY ) {
	switch( inKey ) {
		case MG_KEY_RIGHT:
			mCursorPosition++;
			break;
		case MG_KEY_LEFT:
			mCursorPosition--;
		default:
			break;
		}

	fixCursorPosition();
	}



inline void TextFieldGL::keyReleased(
	unsigned char inKey, double inX, double inY ) {

	}



inline void TextFieldGL::specialKeyReleased(
	int inKey, double inX, double inY ) {

	}



inline void TextFieldGL::mouseReleased(
	double inX, double inY ) {

	if( isInside( inX, inY ) ) {

		mCursorPosition = (int)( 0.5 + strlen( mString ) *
								 ( inX - mAnchorX ) / mWidth );
		
		}

	}



inline void TextFieldGL::fixCursorPosition() {
	if( mCursorPosition < 0 ) {
		mCursorPosition = 0;
		}
	else {
		int stringLength = strlen( mString );
		if( mCursorPosition > stringLength ) {
			mCursorPosition = stringLength;
			}
		}
	}


		
inline void TextFieldGL::fireRedraw() {
    
    // offset to give text room
    double offset = mHeight * 0.1;
    

	// draw the background 
	glColor3f( mBackgroundColor->r,
			   mBackgroundColor->g, mBackgroundColor->b );
	glBegin( GL_QUADS ); {
		glVertex2d( mAnchorX - offset, mAnchorY - offset );		
		glVertex2d( mAnchorX - offset, mAnchorY + mHeight + offset );

		glVertex2d( mAnchorX + mWidth + offset, mAnchorY + mHeight + offset );
		glVertex2d( mAnchorX + mWidth + offset, mAnchorY - offset ); 

		}
	glEnd();

    
    double charWidth = mHeight * strlen( mString );
    
	
	// draw the text
	mText->drawText( mString, mAnchorX, mAnchorY,
					 charWidth, mHeight );
    
    if( mFocused ) {
		// draw the cursor
		glColor3f( mCursorColor->r,
				   mCursorColor->g, mCursorColor->b );
		glBegin( GL_LINES ); {
            
            // make a temp sub-string and measure its length
            char *substring = stringDuplicate( mString );
            substring[ mCursorPosition ] = '\0';
            

            double subWidth = mText->measureTextWidth( substring ) * mHeight 
                + offset;
			
            delete [] substring;
            

            double cursorViewX = mAnchorX + subWidth;
            
            glVertex2d( cursorViewX, mAnchorY );		
			glVertex2d( cursorViewX, mAnchorY + mHeight );
			}
		glEnd();
		}
	
	
	// draw the border on top of the text and cursor
	glColor3f( mCurrentBorderColor->r,
			   mCurrentBorderColor->g, mCurrentBorderColor->b );
	glBegin( GL_LINE_LOOP ); {
		glVertex2d( mAnchorX - offset, mAnchorY - offset );		
		glVertex2d( mAnchorX - offset, mAnchorY + mHeight + offset );

		glVertex2d( mAnchorX + mWidth + offset, mAnchorY + mHeight + offset );
		glVertex2d( mAnchorX + mWidth + offset, mAnchorY - offset ); 
		}
	glEnd();

	
	}



#endif



