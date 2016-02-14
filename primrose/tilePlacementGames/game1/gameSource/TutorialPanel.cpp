#include "TutorialPanel.h"

#include "numeral.h"

#include "gameControl.h"

#include "sound.h"


#include <GL/gl.h>


Color nextPieceDemoColors[3] = { 
    Color( 255/255.0, 112/255.0, 0/255.0 ),
    
    Color( 128/255.0, 255/255.0, 0/255.0 ),
    
    Color( 96/255.0,  0/255.0,   192/255.0 ) 
    };



TutorialPanel::TutorialPanel( int inW, int inH )
        : Panel( inW, inH ),
          mNextButton( inW - ( 19 + 21 ), 19 + 21, "next" ),
          mNextPanel( inW, inH ),
          mNextPieceDemo( inW / 2, inH / 3, NULL,
                          nextPieceDemoColors[0].copy(),
                          nextPieceDemoColors[1].copy() ),
          mDemoStage( 0 ),
          mStepsBetweenStages( 50 ), mStepCount( 0 ) {


    addButton( &mNextButton );
    addSubPanel( &mNextPanel );
    


    int x, y, i;
    
    i=0;
    
    int gridH = 4;
    int gridW = 7;
    
    int yOffset = mNextPieceDemo.mY + 41 + 40;
    
    for( y=0; y<gridH; y++ ) {
        for( x=0; x<gridW; x++ ) {
            
            mAllDemoSpaces[i] = 
            mGridDemo[y][x] =
                new GridSpace( x * 40 + 19 + 21, y * 40 + yOffset + 21 );
            
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

    }


    
TutorialPanel::~TutorialPanel() {
    for( int i=0; i<28; i++ ) {
        delete mAllDemoSpaces[i];
        }
    }



void TutorialPanel::setStageZero() {
    
    int i;
    for( i=0; i<28; i++ ) {
        mAllDemoSpaces[i]->mActive = false;
        mAllDemoSpaces[i]->setColor( NULL );
        }
    }



void TutorialPanel::setVisible( char inIsVisible ) {
    Panel::setVisible( inIsVisible );
    
    }



char TutorialPanel::pointerUp( int inX, int inY ) {
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



void TutorialPanel::step() {
    Panel::step();
    
    mNextPieceDemo.step();
    
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
        
        setStageZero();
        }


    if( mStepCount >= mStepsBetweenStages ) {
        

        mStepCount = 0;
        
        mDemoStage++;
        
        if( mDemoStage > 2 ) {
            mDemoStage = 0;
            }
        

        if( mDemoStage == 0 ) {
            setStageZero();
            }
        else if( mDemoStage == 1 ) {
            GridSpace *s = mGridDemo[2][2];
            
            

            s->setColor( nextPieceDemoColors[0].copy() );
            
            if( getSoundOn() ) {
        
                int x = 2;
                
                float leftVolume, rightVolume;
                
                computeEarLoudness( x, &leftVolume, &rightVolume );

                playPlacementSound( s->getColorIndex(), 
                                    leftVolume, rightVolume );
                }


            for( i=0; i<28; i++ ) {
                GridSpace *other = mAllDemoSpaces[i];
                
                if( other != s ) {
                    if( other->mY == s->mY ||
                        other->mX == s->mX ) {
                        other->mActive = true;
                        }
                    }
                }
            }
        else if( mDemoStage == 2 ) {
            GridSpace *s = mGridDemo[0][2];
            
            s->setColor( nextPieceDemoColors[1].copy() );
        

            if( getSoundOn() ) {
                
                int x = 2;
                
                float leftVolume, rightVolume;
                
                computeEarLoudness( x, &leftVolume, &rightVolume );
                
                playPlacementSound( s->getColorIndex(), 
                                    leftVolume, rightVolume );
                }

            

            for( i=0; i<28; i++ ) {
                mAllDemoSpaces[i]->mActive = false;
                }
            }
        
        
            

        }
    
    }



Color tutorialTextColor( 255/255.0, 255/255.0, 160/255.0 );

        
void TutorialPanel::drawBase() {
    
    Panel::drawBase();
    
    if( mFadeProgress > 0 ) {
        
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        drawString( "colored tiles come in pairs", left, 
                    19,
                    mNextPieceDemo.mY - 41 - 30,
                    &tutorialTextColor, mFadeProgress );        
        
        drawString( "press grid to place first tile", left, 
                    19,
                    mNextPieceDemo.mY + 41 + 20,
                    &tutorialTextColor, mFadeProgress );        
        

        drawString( "second tile of pair must go in", left, 
                    19,
                    mGridDemo[3][0]->mY + 20 + 20,
                    &tutorialTextColor, mFadeProgress );        

        drawString( "the same row or column", left, 
                    19,
                    mGridDemo[3][0]->mY + 20 + 40,
                    &tutorialTextColor, mFadeProgress );        
        

        glDisable( GL_BLEND );
        

        mNextPieceDemo.draw( mFadeProgress );
        
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



void TutorialPanel::closePressed() {
    // return to step 0
    mDemoStage = 0;
    mStepCount = 0;
    
    setStageZero();
    }


        
        
