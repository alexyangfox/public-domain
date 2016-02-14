#include "Button.h"
#include "numeral.h"

#include "minorGems/util/stringUtils.h"


#include <GL/gl.h>
#include <math.h>



Button::Button( int inX, int inY, char *inString )
        : mX( inX ), mY( inY ),
          mString( stringDuplicate( inString ) ),
          mSpace( inX, inY ),
          mVisible( false ),
          mFadeProgress( 0 ),
          mLastString( NULL ),
          mStringTransitionProgress( 1 ) {

    }


    
Button::~Button() {
    delete [] mString;

    if( mLastString != NULL ) {
        delete [] mLastString;
        }
    
    }



void Button::setString( char *inString ) {
    if( mLastString != NULL ) {
        delete [] mLastString;
        }
    mLastString = mString;
        
    mString = stringDuplicate( inString );
    
    mStringTransitionProgress = 0;
    }



void Button::forceString( char *inString ) {
    if( mLastString != NULL ) {
        delete [] mLastString;
        }
    mLastString = mString;
        
    mString = stringDuplicate( inString );
    
    mStringTransitionProgress = 1;
    }



char Button::isInside( int inX, int inY ) {
    // slightly bigger than underlying grid
    // to make it easier to press

    if( fabs( inX - mX ) < 23
        &&
        fabs( inY - mY ) < 23 ) {
        
        return true;
        }
    
    return false;
    }


        
void Button::setVisible( char inIsVisible ) {
    mVisible = inIsVisible;
    }



void Button::forceVisible() {
    setVisible( true );
    
    mFadeProgress = 1;
    }

void Button::forceInvisible() {
    setVisible( false );

    mFadeProgress = 0;
    }



void Button::step() {
    if( mVisible && mFadeProgress < 1 ) {
        mFadeProgress += 0.1;
        
        if( mFadeProgress > 1 ) {
            mFadeProgress = 1;
            }
        }
    else if( !mVisible && mFadeProgress > 0 ) {
        mFadeProgress -= 0.1;
        
        if( mFadeProgress < 0 ) {
            mFadeProgress = 0;
            }
        }
    
    if( mStringTransitionProgress < 1 ) {
        mStringTransitionProgress += 0.2;
        if( mStringTransitionProgress > 1 ) {
            mStringTransitionProgress = 1;
            }
        }
    
    }

    
Color buttonStringColor( 76/255.0, 76/255.0, 255/255.0 );

        
void Button::draw() {
    
    if( mFadeProgress > 0 ) {
        

        mSpace.drawGrid( mFadeProgress );

        // additive to support smooth cross-blending
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        

        float alpha;
        
        if( mStringTransitionProgress > 0 ) {
            alpha = mFadeProgress * mStringTransitionProgress;
            
            if( strlen( mString ) < 3 ) {
                drawStringBig( mString, center, mSpace.mX, mSpace.mY,  
                               &buttonStringColor, 
                               alpha );
                }
            else {
                drawString( mString, center, mSpace.mX, mSpace.mY,  
                            &buttonStringColor, 
                            alpha );
                }
            }
        

        if( mStringTransitionProgress < 1 ) {
            alpha = mFadeProgress * ( 1 - mStringTransitionProgress );
            
            if( strlen( mString ) < 3 ) {
                drawStringBig( mLastString, center, mSpace.mX, mSpace.mY,  
                               &buttonStringColor, 
                               alpha );
                }
            else {
                drawString( mLastString, center, mSpace.mX, mSpace.mY,  
                            &buttonStringColor, 
                            alpha );
                }
            }
        
        glDisable( GL_BLEND );
        }
    
    }


        
        
