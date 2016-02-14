#include "HighScorePanel.h"

#include "numeral.h"
#include "spriteBank.h"

#include "gameControl.h"


#include "minorGems/util/stringUtils.h"


#include <GL/gl.h>


extern SimpleVector<ScoreBundle*> savedAllTimeScores;
extern SimpleVector<ScoreBundle*> savedTodayScores;




HighScorePanel::HighScorePanel( int inW, int inH )
        : Panel( inW, inH ), mMenuPanel( NULL ) {
    

    int i=0;
    int a=0;
    
    int xStep = inW / 2 - 10;
    int xStart = inW / 2 - (21 + 9);
    
    
    int yStart = 19 + 21 + 40;
    int yStep = ( inH / 2 - yStart - 21 ) / 3;
    

    int x, y;
    
    // grid of 2x4
    for( x=0; x<2; x++ ) {
        for( y=0; y<4; y++ ) {
                            
            mAllTimeButtons[i] = new Button( xStart + x * xStep,
                                             yStart + y * yStep,
                                             "play" );
    
            addButton( mAllTimeButtons[i] );
            
            mAllButtons[a] = mAllTimeButtons[i];
            
            i++;
            a++;
            }
        }



    i = 0;
    
    yStart += inH / 2 - 18;
    
    // grid of 2x4
    for( x=0; x<2; x++ ) {
        for( y=0; y<4; y++ ) {
                        
            mTodayButtons[i] = new Button( xStart + x * xStep,
                                             yStart + y * yStep,
                                             "play" );
    
            addButton( mTodayButtons[i] );

            mAllButtons[a] = mTodayButtons[i];

            i++;
            a++;
            }
        }
    

    clearScores();
    
    // add the ones that are saved
    for( i=0; i<savedAllTimeScores.size(); i++ ) {
        ScoreBundle *s = *( savedAllTimeScores.getElement( i ) );
        
        addAllTimeScore( s->copy()  );
        }
    
    for( i=0; i<savedTodayScores.size(); i++ ) {
        ScoreBundle *s = *( savedTodayScores.getElement( i ) );
        
        addTodayScore( s->copy()  );
        }


    }


    
HighScorePanel::~HighScorePanel() {
    int i;
    
    for( i=0; i<16; i++ ) {
        delete mAllButtons[i];
        }
    
    clearScores();
    }



void HighScorePanel::clearScores() {
    int i;

    for( i=0; i<mAllTimeScores.size(); i++ ) {
        delete *( mAllTimeScores.getElement( i ) );
        }
    for( i=0; i<mTodayScores.size(); i++ ) {
        delete *( mTodayScores.getElement( i ) );
        }

    mAllTimeScores.deleteAll();
    mTodayScores.deleteAll();

    for( i=0; i<16; i++ ) {
        mAllScores[i] = NULL;
        }
    }


        
void HighScorePanel::addAllTimeScore( ScoreBundle *inScore ) {
    if( mAllTimeScores.size() < 8 ) {
        mAllTimeScores.push_back( inScore );
        mAllScores[ mAllTimeScores.size() - 1 ] = ( inScore );
        }
    else {
        printf( "More than 8 scores added to all-time display\n" );
        }
    }



void HighScorePanel::addTodayScore( ScoreBundle *inScore ) {
    if( mTodayScores.size() < 8 ) {
        mTodayScores.push_back( inScore );
        // skip 8 all-time slots in array
        mAllScores[ 8 + mTodayScores.size() - 1 ] = ( inScore );
        }
    else {
        printf( "More than 8 scores added to today display\n" );
        }
    }



void HighScorePanel::setVisible( char inIsVisible ) {
    Panel::setVisible( inIsVisible );

    
    // disable buttons for empty slots
    
    int i;
    
    for( i = 0; i<16; i++ ) {
        if( mAllScores[i] == NULL ) {
            mAllButtons[i]->forceInvisible();
            }
        }
    }



char HighScorePanel::pointerUp( int inX, int inY ) {
    
    char consumed = Panel::pointerUp( inX, inY );
    
    if( consumed ) {
        return true;
        }
    

    if( ! isSubPanelVisible() ) {
    

        int i;
        
        for( i = 0; i<16; i++ ) {
            if( mAllButtons[i]->isVisible() &&
                mAllScores[i] != NULL &&
                mAllButtons[i]->isInside( inX, inY ) ) {
                
                if( mMenuPanel != NULL ) {
                    mMenuPanel->forceInvisible();
                    }

                clearSavedScores();
                
                // save scores because we're about to be destroyed
                
                int j;
                
                for( j=0; j<mAllTimeScores.size(); j++ ) {
                    ScoreBundle *s = *( mAllTimeScores.getElement( j ) );
                    savedAllTimeScores.push_back( s->copy() );
                    }
                for( j=0; j<mTodayScores.size(); j++ ) {
                    ScoreBundle *s = *( mTodayScores.getElement( j ) );
                    savedTodayScores.push_back( s->copy() );
                    }
                

                setVisible( false );
                
                playbackGame( mAllScores[i]->copy() );
                
                return true;
                }
            }
        }
    
    return false;
    }

Color scoreHeaderColor( 76/255.0, 76/255.0, 255/255.0 );

Color scoreRuleColor( 1, 1, 1 );

Color scoreNumberColor( 255/255.0, 255/255.0, 160/255.0 );


        
void HighScorePanel::drawBase() {
    
    Panel::drawBase();
    
    if( mFadeProgress > 0 ) {
        
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        int headerX = mW / 2;
        

        drawStringBig( "all time", center, 
                       headerX,
                       mAllTimeButtons[0]->mY - 40,
                       &scoreHeaderColor, mFadeProgress );
        
        int i;
        
        for( i=0; i<mAllTimeScores.size() && i<8; i++ ) {
            ScoreBundle *b = *( mAllTimeScores.getElement( i ) );
            
            int textX = mAllTimeButtons[i]->mX - 21 - 10;
            
            drawScore( b->mScore,
                       textX,
                       mAllTimeButtons[i]->mY - 2,
                       &scoreNumberColor, mFadeProgress );
            
            drawString( b->mName, right,
                        textX,
                        mAllTimeButtons[i]->mY + 5 + 2,
                        &scoreHeaderColor, mFadeProgress );
            }

        if( mAllTimeScores.size() == 0 ) {
            drawStringBig( "none", center, 
                           headerX,
                           mAllTimeButtons[1]->mY,
                           &scoreHeaderColor, mFadeProgress );
            }


        // rules around TODAY to separate

        int ruleY = mTodayButtons[0]->mY - 40 + 21;
        
        drawSprite( gridLineTop, headerX - mW/4, ruleY, 32, 32, 
                    &scoreRuleColor, mFadeProgress );
        
        drawSprite( gridLineTop, headerX + mW/4, ruleY, 32, 32, 
                    &scoreRuleColor, mFadeProgress );


        drawStringBig( "today", center, 
                       headerX,
                       mTodayButtons[0]->mY - 40,
                       &scoreHeaderColor, mFadeProgress );

        for( i=0; i<mTodayScores.size() && i<8; i++ ) {
            ScoreBundle *b = *( mTodayScores.getElement( i ) );
            
            int textX = mTodayButtons[i]->mX - 21 - 10;
            
            drawScore( b->mScore,
                       textX,
                       mTodayButtons[i]->mY - 2,
                       &scoreNumberColor, mFadeProgress );
            
            drawString( b->mName, right,
                        textX,
                        mTodayButtons[i]->mY + 5 + 2,
                        &scoreHeaderColor, mFadeProgress );
            }

        if( mTodayScores.size() == 0 ) {
            drawStringBig( "none", center, 
                           headerX,
                           mTodayButtons[1]->mY,
                           &scoreHeaderColor, mFadeProgress );
            }
        

        glDisable( GL_BLEND );

        
                
        }
    
    }



void HighScorePanel::closePressed() {
        
    }


        
        
