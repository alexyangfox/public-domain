#include "TutorialPanelB.h"

#include "numeral.h"

#include "gameControl.h"
#include "sound.h"


#include <GL/gl.h>


extern Color nextPieceDemoColors[3];


TutorialPanelB::TutorialPanelB( int inW, int inH )
        : Panel( inW, inH ),
          mNextButton( inW - ( 19 + 21 ), 19 + 21, "next" ),
          mNextPanel( inW, inH ),
          mDemoStage( 0 ),
          mStepsBetweenStages( 50 ), mStepCount( 0 ) {


    addButton( &mNextButton );
    addSubPanel( &mNextPanel );
    


    int x, y, i;
    
    i=0;
    
    int gridH = 4;
    int gridW = 7;
    
    int yOffset = inH / 3 - 12;
    
    for( y=0; y<gridH; y++ ) {
        for( x=0; x<gridW; x++ ) {
            
            mAllDemoSpaces[i] = 
            mGridDemo[y][x] =
                new GridSpace( x * 40 + 19 + 21, y * 40 + yOffset + 21 );
          
            mAllDemoSpaces[i]->mAddToGlobalScore = false;
            
            
            i++;
            }
        }
    
    // set neighbor relationships
    for( y=0; y<gridH; y++ ) {
        for( x=0; x<gridW; x++ ) {
        
            
            if( y > 0 ) {
                mGridDemo[y][x]->mNeighbors[ GridSpace::top ] = 
                    mGridDemo[y-1][x];
                }
            if( y < gridH-1 ) {
                mGridDemo[y][x]->mNeighbors[ GridSpace::bottom ] = 
                    mGridDemo[y+1][x];
                }
            if( x > 0 ) {
                mGridDemo[y][x]->mNeighbors[ GridSpace::left ] = 
                    mGridDemo[y][x-1];
                }
            if( x < gridW-1 ) {
                mGridDemo[y][x]->mNeighbors[ GridSpace::right ] = 
                    mGridDemo[y][x+1];
                }
            }
        }
    
    mInnerSpaces[0] = mGridDemo[1][2];
    mInnerSpaces[1] = mGridDemo[1][3];
    mInnerSpaces[2] = mGridDemo[2][3];
    mInnerSpaces[3] = mGridDemo[2][4];
    
    mInnerSpacesX[0] = 2;
    mInnerSpacesX[1] = 3;
    mInnerSpacesX[2] = 3;
    mInnerSpacesX[3] = 4;
    

    mOuterSpaces[0] = mGridDemo[0][3];
    mOuterSpaces[1] = mGridDemo[1][4];
    mOuterSpaces[2] = mGridDemo[2][5];
    mOuterSpaces[3] = mGridDemo[3][4];
    mOuterSpaces[4] = mGridDemo[3][3];
    mOuterSpaces[5] = mGridDemo[2][2];
    mOuterSpaces[6] = mGridDemo[1][1];
    
    mKeySpace = mGridDemo[0][2];

    setStageZero();
    }



void TutorialPanelB::setStageZero() {
    
    int i;
    
    for( i=0; i<4; i++ ) {
        mInnerSpaces[i]->setColor( nextPieceDemoColors[0].copy() );
        }
    for( i=0; i<7; i++ ) {
        mOuterSpaces[i]->setColor( nextPieceDemoColors[1].copy() );
        }
    mKeySpace->setColor( NULL );
    
    }


    
TutorialPanelB::~TutorialPanelB() {
    for( int i=0; i<28; i++ ) {
        delete mAllDemoSpaces[i];
        }
    }



void TutorialPanelB::setVisible( char inIsVisible ) {
    Panel::setVisible( inIsVisible );
    
    }



char TutorialPanelB::pointerUp( int inX, int inY ) {
    char consumed = Panel::pointerUp( inX, inY );
    
    if( consumed ) {
        return true;
        }
    

    if( ! isSubPanelVisible() ) {

        if( mNextButton.isInside( inX, inY ) ) {
            
            mNextPanel.setVisible( true );
            
            return true;
            }
        }
    
    return false;
    }



void TutorialPanelB::step() {
    Panel::step();
    
        
    int i;

    for( i=0; i<28; i++ ) {
        mAllDemoSpaces[i]->step();
        }
        
    
    if( isVisible() && ! isSubPanelVisible() ) {
        mStepCount++;
        }
    else {
        // return to step 0
        mDemoStage = 0;
        mStepCount = 0;
        }
    


    // check if we need to revert back to stage zero
    // this might happen after we close or after displaying a sub-panel
    if( mDemoStage == 0 
        && 
        ( ! mKeySpace->isEmpty() || mInnerSpaces[0]->isEmpty() )
        && 
        mInnerSpaces[0]->isAnimationDone() ) {
        
        // lingering at end of score animation
        
        setStageZero();
        }
        
    

    if( mStepCount >= mStepsBetweenStages ) {
        

        mStepCount = 0;
        
        mDemoStage++;
        
        if( mDemoStage > 5 ) {
            mDemoStage = 0;
            }
        

        if( mDemoStage == 0 ) {
            setStageZero();
            }
        else if( mDemoStage == 1 ) {
            // hold zero longer
            }
        else if( mDemoStage == 2 ) {
            mKeySpace->setColor( nextPieceDemoColors[1].copy() );

            if( getSoundOn() ) {
                
                int x = 2;
                
                float leftVolume, rightVolume;
                
                computeEarLoudness( x, &leftVolume, &rightVolume );
                
                playPlacementSound( mKeySpace->getColorIndex(), 
                                    leftVolume, rightVolume );
                }

            }
        else if( mDemoStage == 3 ) {
            // accumulate average volume
            float leftEarWeight = 0;
            float rightEarWeight = 0;
            
            int oldColorIndex = mInnerSpaces[0]->getColorIndex();

            for( i=0; i<4; i++ ) {
                mInnerSpaces[i]->mScore = 20;
                mInnerSpaces[i]->flipToClear();
                
                float lV, rV;
                
                computeEarLoudness( mInnerSpacesX[i], &lV, &rV );
                leftEarWeight += lV;
                rightEarWeight += rV;        
                }

            if( getSoundOn() ) {
                
                playClearingSound( oldColorIndex, 4,
                                   1,
                                   leftEarWeight / 4,
                                   rightEarWeight / 4 );
                }
                
            }
        else if( mDemoStage == 4 ) {
            // hold stage 2 for extra step
            }
        else if( mDemoStage == 5 ) {
            // blank before next cycle
            for( i=0; i<28; i++ ) {
                mAllDemoSpaces[i]->setColor( NULL );
                }
            // this stage short
            mStepCount += mStepsBetweenStages / 2;
            }
        
            

        }
    
    }



extern Color tutorialTextColor;

        
void TutorialPanelB::drawBase() {
    
    Panel::drawBase();
    
    if( mFadeProgress > 0 ) {
        
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        
        drawString( "clear a tile group of one color", left, 
                    19,
                    mGridDemo[0][0]->mY - 20 - 60,
                    &tutorialTextColor, mFadeProgress );        
        drawString( "by surrounding it with another", left, 
                    19,
                    mGridDemo[0][0]->mY - 20 - 40,
                    &tutorialTextColor, mFadeProgress );        
        drawString( "color and score points", left, 
                    19,
                    mGridDemo[0][0]->mY - 20 - 20,
                    &tutorialTextColor, mFadeProgress );        

        drawString( "the surrounding tiles flip to", left, 
                    19,
                    mGridDemo[3][0]->mY + 20 + 20,
                    &tutorialTextColor, mFadeProgress );
        drawString( "the color that was cleared", left, 
                    19,
                    mGridDemo[3][0]->mY + 20 + 40,
                    &tutorialTextColor, mFadeProgress );


        drawString( "clear bigger groups for even", left, 
                    19,
                    mGridDemo[3][0]->mY + 20 + 91,
                    &tutorialTextColor, mFadeProgress );        

        drawString( "more points", left, 
                    19,
                    mGridDemo[3][0]->mY + 20 + 111,
                    &tutorialTextColor, mFadeProgress );        
        

        glDisable( GL_BLEND );
        

        int i;
        
        int numGridSpaces = 28;
        
        for( i=0; i<numGridSpaces; i++ ) {
            mAllDemoSpaces[i]->drawGrid( mFadeProgress );
            }
        for( i=0; i<numGridSpaces; i++ ) {
            mAllDemoSpaces[i]->drawPieceCenter( mFadeProgress );
            }
        for( i=0; i<numGridSpaces; i++ ) {
            mAllDemoSpaces[i]->drawPieceHalo( mFadeProgress );
            }
        

        }
    

    
    }



void TutorialPanelB::closePressed() {
    // return to step 0
    mDemoStage = 0;
    mStepCount = 0;
    

    // can't do this safely here because score animation might still be
    // running 
    // setStageZero();
    }


        
        
