

#include "NextPieceDisplay.h"

#include "spriteBank.h"


#include "minorGems/util/stringUtils.h"



#include <math.h>

#include <GL/gl.h>



/*
Color pieceColors[8] = { 
    Color( 255/255.0, 128/255.0, 0/255.0 ),
    Color( 128/255.0, 255/255.0, 0/255.0 ),
    Color( 96/255.0,  0/255.0,   128/255.0 ),
    Color( 192/255.0, 0/255.0,   0/255.0 ),
    Color( 0/255.0,   128/255.0, 96/255.0 ),
    Color( 255/255.0, 96/255.0,  255/255.0 ),
    Color( 255/255.0, 255/255.0, 160/255.0 ),
    Color( 128/255.0, 96/255.0,  0/255.0 )    
    };
*/



NextPieceDisplay::NextPieceDisplay( int inX, int inY, ColorPool *inPool )
        : mX( inX ), mY( inY ), mPool( inPool ), mBlinkTime( 0 ) {

    mSpaces[0] = new GridSpace( inX, inY - 20 );
    mSpaces[1] = new GridSpace( inX, inY + 20 );
    
    mSpaces[0]->mNeighbors[ GridSpace::bottom ] = mSpaces[1];
    mSpaces[1]->mNeighbors[ GridSpace::top ] = mSpaces[0];


    // starts empty
    // this will generate two new colors
    update();
    }



NextPieceDisplay::NextPieceDisplay( int inX, int inY, ColorPool *inPool,
                                    Color *inA, Color *inB )        
        : mX( inX ), mY( inY ), mPool( inPool ), mBlinkTime( 0 ) {

    mSpaces[0] = new GridSpace( inX, inY - 20 );
    mSpaces[1] = new GridSpace( inX, inY + 20 );
    
    mSpaces[0]->mNeighbors[ GridSpace::bottom ] = mSpaces[1];
    mSpaces[1]->mNeighbors[ GridSpace::top ] = mSpaces[0];

    mSpaces[0]->setColor( inA );
    mSpaces[1]->setColor( inB );
    }




NextPieceDisplay::~NextPieceDisplay() {
    for( int i=0; i<2; i++ ) {
        delete mSpaces[i];
        }
    }



Color *NextPieceDisplay::getNextPiece() {
    Color *returnColor = mSpaces[0]->getColor();
    mSpaces[0]->setColor( NULL );
    return returnColor;
    }


void NextPieceDisplay::update() {
    //printf( "Update called\n" );
    
    if( mSpaces[1]->isEmpty() ) {
        // two new colors
        
        for( int i=0; i<2; i++ ) {
            mSpaces[i]->setColor( mPool->pickColor() );
            }
        }
    else {
        // move next color up
        mSpaces[0]->setColor( mSpaces[1]->getColor() );
        mSpaces[1]->setColor( NULL );
        }
    }



char NextPieceDisplay::isSecondPiece() {
    
    Color *c = mSpaces[1]->getColor();
    
    char returnVal = ( c == NULL );
    
    delete c;
    
    return returnVal;
    }



void NextPieceDisplay::saveState() {
    for( int i=0; i<2; i++ ) {
        mSpaces[i]->saveState();
        }
    }



void NextPieceDisplay::rewindState() {
    for( int i=0; i<2; i++ ) {
        mSpaces[i]->rewindState();
        }
    }


        
Color nextPieceLineWhite( 1, 1, 1 );



void NextPieceDisplay::draw( float inAlpha ) {
    
    
    // extra grid lines around space 0
    glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    int border = 10;
    

    drawSprite( gridLineTop, 
                mSpaces[0]->mX, mSpaces[0]->mY - border, 32, 32,
                &nextPieceLineWhite, inAlpha );
    
    drawSprite( gridLineLeft, 
                mSpaces[0]->mX - border, mSpaces[0]->mY, 32, 32,
                &nextPieceLineWhite, inAlpha );
    
    drawSprite( gridLineRight, 
                mSpaces[0]->mX + border, mSpaces[0]->mY, 32, 32,
                &nextPieceLineWhite, inAlpha );
    

    glDisable( GL_BLEND );
    

    // now draw spaces
    int i;
    for( i=0; i<2; i++ ) {
        mSpaces[i]->drawGrid( inAlpha );
        }
    for( i=0; i<2; i++ ) {
        mSpaces[i]->drawPieceCenter( inAlpha );
        }

    if( mSpaces[0]->mDrawColor != NULL ) {
        
        float glowVal = sin( mBlinkTime - M_PI / 2 ) * 0.5 + 0.5;
        
        //printf( "glow val = %f\n", glowVal );
        
        glowVal *= mSpaces[0]->mDrawColor->a;
        
        glowVal *= 0.5;
        

        if( glowVal > 0 ) {

            glEnable( GL_BLEND );
            
            
            
            drawSprite( pieceBrightHalo, 
                        mSpaces[0]->mX, mSpaces[0]->mY, 
                        32, 32, mSpaces[0]->mDrawColor, 
                        glowVal * inAlpha );
    

            glBlendFunc( GL_SRC_ALPHA, GL_ONE );
            Color white( 1, 1, 1, 1 );
        
            drawSprite( pieceBrightCenter, 
                        mSpaces[0]->mX, mSpaces[0]->mY, 
                        32, 32, &white, 
                        glowVal * inAlpha );
    
            glDisable( GL_BLEND );
            }
        
        if( mSpaces[0]->mDrawColor->a == 0 ) {
            // start blinking over again
            mBlinkTime = 0;
            }
        }
    else {
        // start blinking over again
        mBlinkTime = 0;
        }
    
    

    for( i=0; i<2; i++ ) {
        mSpaces[i]->drawPieceHalo( inAlpha );
        }
    }



void NextPieceDisplay::step() {
    
    for( int i=0; i<2; i++ ) {
        mSpaces[i]->step();
        }

    mBlinkTime += 0.2;
    }


    
        
char *NextPieceDisplay::getSavedState() {
    int spaceColorIndices[2];
    
    for( int i=0; i<2; i++ ) {
        Color *c = mSpaces[i]->getSavedColor();
        
        spaceColorIndices[i] = mPool->getColorIndex( c );
        }
    


    char *savedState = autoSprintf( "%d#"
                                    "%d",
                                    spaceColorIndices[0],
                                    spaceColorIndices[1] );
    
    //printf( "NextPieceDisplay state saved to:  %s\n", savedState );

    return savedState;
    }


        
void NextPieceDisplay::restoreFromSavedState( char *inSavedState ) {
    
    int spaceColorIndices[2];
    
    int numRead = sscanf( inSavedState,
                          "%d#"
                          "%d",
                          &spaceColorIndices[0],
                          &spaceColorIndices[1] );
    
    if( numRead != 2 ) {
        printf( "Error resoring NextPieceDisplay state\n" );
        }
    else {

        for( int i=0; i<2; i++ ) {
            int index = spaceColorIndices[i];

            // set both to NULL first so they fade in properly
            mSpaces[i]->setColor( NULL );
            

            if( index >= 0 ) {    
                mSpaces[i]->setColor( mPool->pickColor( index ) );
                }
            else {
                // set to NULL again to make sure it starts fully empty
                mSpaces[i]->setColor( NULL );
                }
            }
        }
    
    }


