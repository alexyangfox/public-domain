

#include "ColorPool.h"

#include "spriteBank.h"

#include "numeral.h"


#include <math.h>

#include <GL/gl.h>


#include "minorGems/util/random/CustomRandomSource.h"
#include "minorGems/util/stringUtils.h"

extern CustomRandomSource randSource;

extern char colorblindMode;



#define numColors 7

Color pieceColors[numColors] = { 
    //Color( 255/255.0, 128/255.0, 0/255.0 ),
    // redder orange
    Color( 255/255.0, 112/255.0, 0/255.0 ),
    
    Color( 128/255.0, 255/255.0, 0/255.0 ),
    
    //    Color( 96/255.0,  0/255.0,   128/255.0 ),
    // brighter purple
    //Color( 120/255.0,  0/255.0,   160/255.0 ),
    // bluer purple
    Color( 96/255.0,  0/255.0,   192/255.0 ),

    Color( 192/255.0, 0/255.0,   0/255.0 ),
    
    //Color( 0/255.0,   128/255.0, 96/255.0 ),
    // darker teal
    //Color( 0/255.0,   96/255.0, 72/255.0 ),
    // bluer teal
    //Color( 0/255.0,   96/255.0, 96/255.0 ),
    // dark green instead?
    Color( 0/255.0,   96/255.0, 0/255.0 ),

    //Color( 255/255.0, 96/255.0,  255/255.0 ),
    // ligher pink (to avoid purple)
    Color( 255/255.0, 128/255.0,  255/255.0 ),
    
    //Color( 128/255.0, 96/255.0,  0/255.0 )
    // greener, lighter brown
    //Color( 128/255.0, 128/255.0,  0/255.0 )    
    
    // grey
    Color( 128/255.0, 128/255.0,  128/255.0 )    
    //Color( 255/255.0, 255/255.0, 160/255.0 )//,
    //    Color( 128/255.0, 96/255.0,  0/255.0 )    
    };



char colorblindSymbols[ numColors ] = { 'z', 'b', 'j', 'm', 'i', 'u', 'p' };



#define startingSteps 96
//#define startingSteps 2
#define minSteps 6
//#define minSteps 2

ColorPool::ColorPool( int inX, int inY )
        : mX( inX ), mY( inY ),
          mSomeMovesMade( false ),
          mStartingActiveColors( 3 ),
          mNumActiveColors( 3 ), mColorsToSkip( 0 ),
          mEndPhase( false ),
          mEndPhaseFirstColor( false ),
          mStepsBetweenUpdates( startingSteps ),
          mStepsUntilUpdate( startingSteps ),
          mLastStepCount( startingSteps ),
          mStepCountTransitionProgress( 1 ),
          mRewinding( false ),
          mLevel( 1 ),
          mSavedState( NULL ) {

    int i;
    
    int xOffset =  -40 * 3;
    
    for( i=0; i<7; i++ ) {
        
        mSpaces[i] = new GridSpace( inX + xOffset, inY );

        xOffset += 40;
        }
    
    for( i=0; i<6; i++ ) {
        mSpaces[i]->mNeighbors[ GridSpace::right ] = mSpaces[i+1];
        mSpaces[i+1]->mNeighbors[ GridSpace::left ] = mSpaces[i];
        }

    for( i=mColorsToSkip; i<mNumActiveColors; i++ ) {
        mSpaces[i]->setColor( pieceColors[i].copy() );
        }
    
    }



ColorPool::~ColorPool() {
    for( int i=0; i<7; i++ ) {
        delete mSpaces[i];
        }
    if( mSavedState != NULL ) {
        delete [] mSavedState;
        }
    }



// for generating a nice tile icon graphic
int fixedSetIndex = 0;
int fixedSet[15] = {2,2,1,1,1,1,0,0,1,1,2,0,0,1,0};



Color *ColorPool::pickColor( int inIndex ) {

    int colorIndex;

    if( inIndex >= 0 ) {
        colorIndex = inIndex;
        }
    else {
        // random pick
        colorIndex = randSource.getRandomBoundedInt( mColorsToSkip, 
                                                     mNumActiveColors - 1 );
        }
    
    /*
      // turn on to generate title image color sequence
    if( index < 15 ) {
        colorIndex = fixedSet[fixedSetIndex];
        fixedSetIndex ++;
        }
    */
    return pieceColors[ colorIndex ].copy();
    }



char ColorPool::getColorblindSymbol( Color *inColor ) {
    int index = getColorIndex( inColor );
    
    if( index >= 0 ) {
        return colorblindSymbols[ index ];
        }
    
    // not found
    return ' ';
    }



int ColorPool::getColorIndex( Color *inColor ) {
    
    if( inColor != NULL ) {
        
        for( int i=0; i<numColors; i++ ) {
            if( pieceColors[ i ].equals( inColor ) ) {
                return i;
                }
            }
        }
    
    // not found or NULL
    return -1;    
    }



void ColorPool::registerMove() {
    mSomeMovesMade = true;

    char updateNow = false;

    mStepsUntilUpdate --;

    if( ! mEndPhase ) {
        if( mStepsUntilUpdate == 0 ) {
            updateNow = true;
            }        
        }
    else {
        
        // only do this every 2 moves
        if( mStepsUntilUpdate == 0 ) {
            

            // flip weighted coin to decide whether to move on to next level
            
            // P(H) = 1/36, where flipping H moves us on to next level
            // This is a geometric random variable with expected value of 36
            // On average, each level lasts for 72 moves (36 pairs) 
            //   (halfway between 48 and 96)
            
            
            if( randSource.getRandomFloat() <= 1.0f / 36.0f ) {
                updateNow = true;
                }

            // try again after 2 more moves
            mStepsUntilUpdate = 2;
            }
        
        }
    
        
    
    mStepCountTransitionProgress = 0;
    

    
        
    

    if( updateNow ) {
        

        if( ! mEndPhase ) {

            if( mNumActiveColors < numColors ) {
                mNumActiveColors ++;
                
                int i = mNumActiveColors - 1;
                
                mSpaces[i]->setColor( pieceColors[i].copy() );

                mLevel++;
                }
            else if( mColorsToSkip < numColors - 1 ) {
                mSpaces[mColorsToSkip]->setColor( NULL );            
                mColorsToSkip++;
                
                mLevel++;

                if( mColorsToSkip >= numColors - 1 ) {
                    printf( "Entering end phase\n" );
                    
                    mEndPhase = true;
                    mEndPhaseFirstColor = true;
                    }
                
                }
            }
        else {
            // end mode.  One color at a time

            mNumActiveColors ++;
            mColorsToSkip++;
            
            mEndPhaseFirstColor = false;

            if( mColorsToSkip >= numColors ) {
                // wrap around
                mNumActiveColors = 1;
                mColorsToSkip = 0;
                }
            
            
            if( mColorsToSkip > 0 ) {
                mSpaces[ mColorsToSkip - 1 ]->setColor( NULL );
                }
            else {
                // wrap around
                mSpaces[ numColors - 1 ]->setColor( NULL );
                }

            int i = mNumActiveColors - 1;
            mSpaces[i]->setColor( pieceColors[i].copy() );
            
            mLevel++;

            }
        
    
        printf( "Level %d\n", mLevel );
        

        if( !mEndPhase ) {
            
            mStepsBetweenUpdates /= 2;
            
            if( mStepsBetweenUpdates < minSteps ) {
                mStepsBetweenUpdates = minSteps;
                }
            }
        else {
            // don't adjust steps between updates, since we're
            // flipping a coin after each pair moves above

            /*

            // random steps between updates
            
            // between 48 (just enough to not fill grid)
            // and 96 (just enough to not fill grid twice)
            mStepsBetweenUpdates = randSource.getRandomBoundedInt( 48,
                                                                   96 );
            
            //printf( "%d steps until next phase\n", mStepsBetweenUpdates );
            

            if( mStepsBetweenUpdates % 2 != 0 ) {
                mStepsBetweenUpdates ++;
                }
            */
            }
        
        
        mStepsUntilUpdate = mStepsBetweenUpdates;

        }
    }



void ColorPool::deRegisterMove() {
    mStepsUntilUpdate ++;
    mStepCountTransitionProgress = 0;
    mRewinding = true;
    }

    

Color colorAddCountColor( 76/255.0, 76/255.0, 255/255.0 );
Color colorRemoveCountColor( 0/255.0, 0/255.0, 0/255.0 );


        

void ColorPool::draw( float inAlpha ) {    

    // now draw spaces
    int i;
    for( i=0; i<7; i++ ) {
        mSpaces[i]->drawGrid( inAlpha );
        }
    for( i=0; i<7; i++ ) {
        mSpaces[i]->drawPieceCenter( inAlpha );
        }
    for( i=0; i<7; i++ ) {
        mSpaces[i]->drawPieceHalo( inAlpha );
        }


    // switch counter drawing funciton and 
    // counter position (removal counters)
    // when in colorblind mode (so counter not on top of colorblind symbol)
    
    void (*addCounterDrawingFunction)( int, float, float, Color *, float );
    void (*removeCounterDrawingFunction)( int, float, float, 
                                          Color *, float );
    addCounterDrawingFunction = &drawCounterBig;
    
    
    float removeCounterXOffset;
    float removeCounterYOffset;
    
    
    if( !colorblindMode ) {
        removeCounterDrawingFunction = &drawCounterBig;
        removeCounterXOffset = 0;
        removeCounterYOffset = 0;            
        }
    else {
        removeCounterDrawingFunction = &drawCounter;
        removeCounterXOffset = -20 + 8;
        removeCounterYOffset = -20 + 5;            
        }


    
    // back to hiding counters during end game (to prevent careful planning
    // for color transitions that allows people to play forever
    if( !mEndPhase
        // || mNumActiveColors < numColors || mColorsToSkip < (numColors-1) 
        ) {

                    
        void (*counterDrawingFunction)( int, float, float, Color *, float );
        Color *thisNumberColor;
        float counterXOffset;
        float counterYOffset;
        
        
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        
        
        if( mNumActiveColors < numColors || mEndPhase ) {
            i = mNumActiveColors;

            if( mEndPhase && mNumActiveColors > numColors - 1 ) {
                i = 0;
                }
            
            glBlendFunc( GL_SRC_ALPHA, GL_ONE );
            thisNumberColor = &colorAddCountColor;

            counterDrawingFunction = addCounterDrawingFunction;
            counterXOffset = 0;
            counterYOffset = 0;
            }
        else {
            i = mColorsToSkip;
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            thisNumberColor = &colorRemoveCountColor;
            
            counterDrawingFunction = removeCounterDrawingFunction;
            counterXOffset = removeCounterXOffset;
            counterYOffset = removeCounterYOffset;
            }
        
        // draw counter over next active space

        
        // blend between last count
        
        (*counterDrawingFunction)( mStepsUntilUpdate,
                                   mSpaces[i]->mX + counterXOffset,  
                                   mSpaces[i]->mY + counterYOffset,
                                   thisNumberColor,
                                   mStepCountTransitionProgress * inAlpha );
        
        
        if( mStepCountTransitionProgress < 1 && 
            ( mLastStepCount > 1 || mRewinding )
            &&
            mSomeMovesMade ) {
            
            // skip drawing if counter moving to next space 
            // (when last count 1 and not rewinding)
            // thus, counter fades in from blank in next space
            
            (*counterDrawingFunction)( 
                mLastStepCount,
                mSpaces[i]->mX + counterXOffset,  
                mSpaces[i]->mY + counterYOffset,
                thisNumberColor,
                ( 1 - mStepCountTransitionProgress ) * inAlpha  );
            }
        
        
        if( mEndPhase ) {
            // draw count-down number too over current color
            i = mColorsToSkip;
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            thisNumberColor = &colorRemoveCountColor;
            
            counterDrawingFunction = removeCounterDrawingFunction;
            counterXOffset = removeCounterXOffset;
            counterYOffset = removeCounterYOffset;
            
            (*counterDrawingFunction)( 
                mStepsUntilUpdate,
                mSpaces[i]->mX + counterXOffset,  
                mSpaces[i]->mY + counterYOffset,
                thisNumberColor,
                mStepCountTransitionProgress * inAlpha );
            
            
        
            if( mStepCountTransitionProgress < 1 && 
                ( mLastStepCount > 1 || mRewinding ) 
                && mSomeMovesMade ) {
                
                // skip drawing if counter moving to next space 
                // (when last count 0)
                // thus, counter fades in from blank in next space
                
                (*counterDrawingFunction)( 
                    mLastStepCount,
                    mSpaces[i]->mX + counterXOffset,  
                    mSpaces[i]->mY + counterYOffset,
                    thisNumberColor,
                    ( 1 - mStepCountTransitionProgress ) * inAlpha  );
                }
            
            }
        


        int lastSpaceIndex = -1;
        

        if( i > 0 && mNumActiveColors < numColors ) {
            lastSpaceIndex = i-1;
            }
        else if( mNumActiveColors == numColors && mColorsToSkip == 0 ) {
            lastSpaceIndex = numColors - 1;
            }
        else if( mNumActiveColors == numColors && mColorsToSkip > 0 ) {
            lastSpaceIndex = i-1;
            }
                        
        if( mEndPhase ) {
            if( lastSpaceIndex < 0 ) {
                lastSpaceIndex = numColors - 1;
                }
            }
        
            
        
        // skip this at beginning, when there are first active colors
        // only (otherwise, a 1 fades out on last starting active color)
        // we know we are at the beginning when no moves made yet
        if( mSomeMovesMade 
            && 
            //mEndPhase 
            //&&
            lastSpaceIndex >= 0 
            && 
            ( mSpaces[lastSpaceIndex]->mDrawColor == NULL 
              || 
              ( mColorsToSkip <= 0 
                &&
                mSpaces[lastSpaceIndex]->mDrawColor->a < 1 )
              ||
              ( ( mColorsToSkip > 0 || mEndPhase ) 
                &&
                mSpaces[lastSpaceIndex]->mDrawColor->a > 0 ) ) ) {

            
            float alpha;
            if( mColorsToSkip == 0 && ! mEndPhase ) {
                // last space fading in
                alpha = 1;
                
                if( mSpaces[lastSpaceIndex]->mDrawColor != NULL ) {
                    alpha = 1 - mSpaces[lastSpaceIndex]->mDrawColor->a;
                    }

                glBlendFunc( GL_SRC_ALPHA, GL_ONE );
                thisNumberColor = &colorAddCountColor;

                counterDrawingFunction = addCounterDrawingFunction;
                counterXOffset = 0;
                counterYOffset = 0;
                }
            else {
                // last space fading out
                alpha = 0;
                
                if( mSpaces[lastSpaceIndex]->mDrawColor != NULL ) {
                    alpha = mSpaces[lastSpaceIndex]->mDrawColor->a;
                    }
                glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                thisNumberColor = &colorRemoveCountColor;
                
                counterDrawingFunction = removeCounterDrawingFunction;
                counterXOffset = removeCounterXOffset;
                counterYOffset = removeCounterYOffset;
                }
            


            if( alpha > 0 ) {
                
                // fade out 1 on last space as color fades in
            
                (*counterDrawingFunction)( 
                    1,
                    mSpaces[lastSpaceIndex]->mX + counterXOffset,  
                    mSpaces[lastSpaceIndex]->mY + counterYOffset,
                    thisNumberColor,
                    alpha * inAlpha );
                }
            
            }



        if( mEndPhase && mSomeMovesMade ) {
            lastSpaceIndex ++;
            
            if( lastSpaceIndex >= numColors ) {
                lastSpaceIndex = 0;
                }
            
            // middle space fading in
            float alpha = 1;
                
            if( mSpaces[lastSpaceIndex]->mDrawColor != NULL ) {
                alpha = 1 - mSpaces[lastSpaceIndex]->mDrawColor->a;
                }
            
            if( alpha > 0 ) {
            
                glBlendFunc( GL_SRC_ALPHA, GL_ONE );
                thisNumberColor = &colorAddCountColor;
            
                counterDrawingFunction = addCounterDrawingFunction;
                counterXOffset = 0;
                counterYOffset = 0;            
                
                (*counterDrawingFunction)( 
                    1,
                    mSpaces[lastSpaceIndex]->mX + counterXOffset,  
                    mSpaces[lastSpaceIndex]->mY + counterYOffset,
                    thisNumberColor,
                    alpha * inAlpha );
                }
            
            }
        
            
        glDisable( GL_BLEND );
        }
    

    if( mEndPhase && mEndPhaseFirstColor ) {
        
        // ensure smooth "1" fade out on last color
        
        int lastSpaceIndex = numColors - 2;


        if( mSpaces[lastSpaceIndex]->mDrawColor != NULL
            &&
            mSpaces[lastSpaceIndex]->mDrawColor->a > 0 ) {
            
            glEnable( GL_BLEND );
            
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            

            (*removeCounterDrawingFunction)( 
                1,
                mSpaces[lastSpaceIndex]->mX + removeCounterXOffset,  
                mSpaces[lastSpaceIndex]->mY + removeCounterYOffset,
                &colorRemoveCountColor,
                mSpaces[lastSpaceIndex]->mDrawColor->a * inAlpha );
            
            glDisable( GL_BLEND );
            }
        }
    
    
    }



void ColorPool::step() {
    
    for( int i=0; i<7; i++ ) {
        mSpaces[i]->step();
        }

    if( mStepCountTransitionProgress < 1 ) {
        
        mStepCountTransitionProgress += 0.2;
        
        if( mStepCountTransitionProgress >= 1 ) {
            mStepCountTransitionProgress = 1;
            
            // done with transition
            mLastStepCount = mStepsUntilUpdate;

            // done rewinding if we were
            mRewinding = false;
            }
        }
    
    }




void ColorPool::saveState() {
    if( mSavedState != NULL ) {
        delete [] mSavedState;
        }
    
    /*
    int mNumActiveColors;
    int mColorsToSkip;
    
    char mEndPhase;
    

    int mStepsBetweenUpdates;
    
    int mStepsUntilUpdate;
    
    int mLastStepCount;
    
    int mLevel;
    */  
    mSavedState = autoSprintf( "%d#"
                               "%d#"
                               "%d#"
                               "%d#"
                               "%d#"
                               "%d#"
                               "%d",
                               mNumActiveColors,
                               mColorsToSkip,
                               (int)mEndPhase,
                               mStepsUntilUpdate,
                               mStepsBetweenUpdates,
                               mLastStepCount,
                               mLevel );
    //printf( "ColorPool state saved to:  %s\n", mSavedState );
    
    }

    
        
char *ColorPool::getSavedState() {
    if( mSavedState != NULL ) {
        return stringDuplicate( mSavedState );
        }
    else {
        return NULL;
        }
    
    }


        
void ColorPool::restoreFromSavedState( char *inSavedState ) {
    
    int endPhaseInt;
    
    int numRead = sscanf( inSavedState,
                          "%d#"
                          "%d#"
                          "%d#"
                          "%d#"
                          "%d#"
                          "%d#"
                          "%d",
                          &mNumActiveColors,
                          &mColorsToSkip,
                          &endPhaseInt,
                          &mStepsUntilUpdate,
                          &mStepsBetweenUpdates,
                          &mLastStepCount,
                          &mLevel );
    
    if( numRead != 7 ) {
        printf( "Error restoring ColorPool state\n" );
        }
    else {
        mEndPhase = endPhaseInt;

        int i;
        
        for( i=0; i<numColors; i++ ) {
            // set all to NULL first
            // forces them to fade in together (even though 3 of them
            // were already set in constructor above)
            mSpaces[i]->setColor( NULL );

            if( i >= mColorsToSkip && i < mNumActiveColors ) {
                mSpaces[i]->setColor( pieceColors[i].copy() );
                }
            else {
                // set to NULL again to make sure they don't fade
                // out from some other color
                mSpaces[i]->setColor( NULL );
                }
            }
        
        mStartingActiveColors = mNumActiveColors;
        }
    
    }

