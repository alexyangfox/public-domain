#include "MenuPanel.h"

#include "numeral.h"

#include "gameControl.h"


#include <GL/gl.h>



MenuPanel::MenuPanel( int inW, int inH )
        : Panel( inW, inH ),
          mNewButton( inW - 21 - 19, 100, "+" ),
          mTutorialButton( inW - 21 - 19, 165, "+" ),
          mHighScoreButton( inW - 21 - 19, 230, "+" ),
          mEditNameButton( inW - 21 - 19, 295, "+" ),
          mColorblindButton( inW - 21 - 19, 360, "off" ),
          mSoundButton( inW - 21 - 19, 425, " on " ),
          mHighScoreLoadingPanel( inW, inH ),
          mEditNamePanel( inW, inH ),
          mTutorialPanel( inW, inH ) {
          

    mNewButton.setVisible( false );
    mTutorialButton.setVisible( false );
    mHighScoreButton.setVisible( false );
    mEditNameButton.setVisible( false );
    mEditNameButton.setVisible( false );
    mColorblindButton.setVisible( false );
    mSoundButton.setVisible( false );
    
    if( getColorblindMode() ) {
        mColorblindButton.forceString( " on " );
        }
    if( ! getSoundOn() ) {
        mSoundButton.forceString( "off" );
        }
    
    
    
    addButton( &mNewButton );
    addButton( &mTutorialButton );
    addButton( &mHighScoreButton );
    addButton( &mEditNameButton );
    addButton( &mColorblindButton );
    addButton( &mSoundButton );
    
    mHighScoreLoadingPanel.setVisible( false );
    mEditNamePanel.setVisible( false );
    
    
    addSubPanel( &mEditNamePanel );
    // high score loading panel may need pop up on top of name editing panel
    addSubPanel( &mHighScoreLoadingPanel );

    addSubPanel( &mTutorialPanel );
    
    // display panel created by loading panel, but sits under us
    mDisplayPanel = mHighScoreLoadingPanel.getDisplayPanel();
    
    mDisplayPanel->setMenuPanel( this );
    
    addSubPanel( mDisplayPanel );
    }


    
MenuPanel::~MenuPanel() {

    }



void MenuPanel::postScore( ScoreBundle *inScore ) {
    if( getNameSet() ) {
        mHighScoreLoadingPanel.setScoreToPost( inScore );
        mHighScoreLoadingPanel.setVisible( true );
        }
    else {
        mEditNamePanel.setScoreToPost( inScore, this );
        mEditNamePanel.setVisible( true );
        }
    }



void MenuPanel::forceFadeOutScoreDisplay() {
    mDisplayPanel->forceVisible();
    
    mDisplayPanel->setVisible( false );
    }



char MenuPanel::pointerUp( int inX, int inY ) {
    
    char consumed = Panel::pointerUp( inX, inY );
    
    if( consumed ) {
        return true;
        }
    

    if( ! isSubPanelVisible() ) {
        
        if( mNewButton.isInside( inX, inY ) ) {
            restartGame();
            
            setVisible( false );
            return true;
            }
        if( mEditNameButton.isInside( inX, inY ) ) {
            
            mEditNamePanel.setVisible( true );
            
            return true;
            }
        if( mTutorialButton.isInside( inX, inY ) ) {
            
            mTutorialPanel.setVisible( true );
            
            return true;
            }
        if( mHighScoreButton.isInside( inX, inY ) ) {
            
            mHighScoreLoadingPanel.setLoadingMessage();
            mHighScoreLoadingPanel.setVisible( true );
            
            return true;
            }
        if( mColorblindButton.isInside( inX, inY ) ) {
            // flip
            char oldMode = getColorblindMode();
            char *newString;
            
            if( oldMode ) {
                newString = "off";
                }
            else {
                newString = " on ";
                }
            mColorblindButton.setString( newString );
            setColorblindMode( !oldMode );
            
            return true;
            }
        if( mSoundButton.isInside( inX, inY ) ) {
            // flip
            char oldMode = getSoundOn();
            char *newString;
            
            if( oldMode ) {
                newString = "off";
                }
            else {
                newString = " on ";
                }
            mSoundButton.setString( newString );
            setSoundOn( !oldMode );
            
            return true;
            }
        
        } 
    
    return false;
    }



Color menuItemColor( 76/255.0, 76/255.0, 255/255.0 );

        
void MenuPanel::drawBase() {
    
    Panel::drawBase();
    
    if( mFadeProgress > 0 ) {
        
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        drawStringBig( "new game", right, 
                       mNewButton.mX - 21 - 19,
                       mNewButton.mY,
                       &menuItemColor, mFadeProgress );

        drawStringBig( "instructions", right, 
                       mTutorialButton.mX - 21 - 19,
                       mTutorialButton.mY,
                       &menuItemColor, mFadeProgress );

        drawStringBig( "high scores", right, 
                       mHighScoreButton.mX - 21 - 19,
                       mHighScoreButton.mY,
                       &menuItemColor, mFadeProgress );

        drawStringBig( "edit name", right, 
                       mEditNameButton.mX - 21 - 19,
                       mEditNameButton.mY,
                       &menuItemColor, mFadeProgress );

        drawStringBig( "colorblind", right, 
                       mColorblindButton.mX - 21 - 19,
                       mColorblindButton.mY,
                       &menuItemColor, mFadeProgress );

        drawStringBig( "sound", right, 
                       mSoundButton.mX - 21 - 19,
                       mSoundButton.mY,
                       &menuItemColor, mFadeProgress );
        

        glDisable( GL_BLEND );
        
        }
    


    
    }




void MenuPanel::step() {
    Panel::step();
    
    
    // check if we should force ourself visible
    // (to pass button presses through to loading and display panel, and be
    //  there to catch it when either of their BACK buttons are pressed)
    if( !mVisible &&
        ( mHighScoreLoadingPanel.isFullyVisible() 
          || 
          mDisplayPanel->isFullyVisible()
          ||
          mEditNamePanel.isFullyVisible() ) ) {
        
        forceVisible();
        }
    if( mHighScoreLoadingPanel.isFullyVisible()
        &&
        mEditNamePanel.isVisible() ) {
        
        // loading panel has popped up fully over edit panel
        mEditNamePanel.forceInvisible();
        }
    }



void MenuPanel::closePressed() {
    // don't do this anymore
    // instead, allow them to look at their game even after score is posted

    // they can manually create a new one when ready
    
    // treat close like NEW if game over
    //if( isGameOver() ) {
    //    restartGame();
    //    }

    }


        
        
