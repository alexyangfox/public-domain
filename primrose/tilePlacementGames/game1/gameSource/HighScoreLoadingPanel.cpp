#include "HighScoreLoadingPanel.h"

#include "numeral.h"
#include "spriteBank.h"

#include "gameControl.h"


// defines a secret string shared with server
// must contain:
//
//    #define  secureSalt  "change_me"
//
// where "change_me" is a value shared with the server.
#include "secureSalt.h"


#include "minorGems/util/stringUtils.h"
#include "minorGems/crypto/hashes/sha1.h"

#include <math.h>
#include <GL/gl.h>


Color scoreLoadingGreen( 0, 1, 0 );

Color scoreLoadingRed( 1, 0, 0 );


extern char *savedServerURL;



void HighScoreLoadingPanel::startConnectionTry() {
    mFailed = false;
    mBlinkTime = 0;
    mStatusLight.setColor( scoreLoadingGreen.copy() );
    mLightRed = false;

    if( mServerURL == NULL && mServerURLFetchRequest == NULL ) {
        // try again
        mServerURLFetchRequest = new WebRequest( "GET",
                                                 mServerFinderURL,
                                                 NULL );
        }
    }



void HighScoreLoadingPanel::setFailed() {
    mFailed = true;
    mTryingToPostScore = false;
    
    mLastMessage = mMessage;
    mMessage = "connect failed";
    // smooth transition to failure
    mStringTransitionProgress = 0;
    }



HighScoreLoadingPanel::HighScoreLoadingPanel( int inW, int inH )
        : Panel( inW, inH ),
          mMessage( "loading scores" ),
          mLastMessage( NULL ),
          mStringTransitionProgress( 1 ),
          mStatusLight( inW / 2, inH / 2 ),
          mBlinkTime( 0 ),
          mTryingToPostScore( false ),
          mScoreToPost( NULL ),
          mWebRequest( NULL ),
          mServerFinderURL(
            "http://hcsoftware.sourceforge.net/primrose/serverLocation6.txt" ),
          mServerURL( NULL ), 
          mServerURLFetchRequest( NULL ),
          mFailed( false ),
          mLightRed( false ),
          mDisplayPanel( inW, inH ) {
    
    if( savedServerURL != NULL ) {
        mServerURL = stringDuplicate( savedServerURL );
        }
    

    startConnectionTry();

    // do not manage as sub-panel
    // addSubPanel( &mDisplayPanel );
    }


    
HighScoreLoadingPanel::~HighScoreLoadingPanel() {
    if( mScoreToPost != NULL ) {
        delete mScoreToPost;
        }
    if( mWebRequest != NULL ) {
        delete mWebRequest;
        }

    if( mServerURL != NULL ) {
        delete [] mServerURL;
        }
    if( mServerURLFetchRequest != NULL ) {
        delete mServerURLFetchRequest;
        }
    
    }



void HighScoreLoadingPanel::setLoadingMessage() {
    mLastMessage = mMessage;
    mMessage = "loading scores";
    // force message
    mStringTransitionProgress = 1;
    }



void HighScoreLoadingPanel::setScoreToPost( ScoreBundle *inBundle ) {
    if( mScoreToPost != NULL ) {
        delete mScoreToPost;
        }
    
    mScoreToPost = inBundle;

    if( inBundle != NULL ) {
        
        mTryingToPostScore = true;
    
        mLastMessage = mMessage;
        mMessage = "posting score";
        // force message
        mStringTransitionProgress = 1;
        
        startConnectionTry();
        }
    else {
        mTryingToPostScore = false;
        }
    
    
    }





void HighScoreLoadingPanel::step() {
    Panel::step();
    
    mStatusLight.step();


    mBlinkTime += 0.2;

    
    // allow some blinking (for reading message)
    // before we consider message transitions
    // or consider light c
    if( isFullyVisible() &&
        mBlinkTime > 9 ) {
        
        if( mFailed && ! mLightRed ) {
            mLightRed = true;
            mStatusLight.setColor( scoreLoadingRed.copy() );
            }
        

        if( mStringTransitionProgress < 1 ) {
            mStringTransitionProgress += 0.2;
            if( mStringTransitionProgress > 1 ) {
                mStringTransitionProgress = 1;
                }
            }
        }
    




    // check if we should hide ourself
    if( mVisible &&
        mDisplayPanel.isFullyVisible() ) {
        
        // display showing
        setVisible( false );
        }
    



    if( mServerURLFetchRequest != NULL ) {
        int stepResult = mServerURLFetchRequest->step();
        
        if( stepResult == 1 ) {
            // done
            
            char *result = mServerURLFetchRequest->getResult();
            
            SimpleVector<char *> *tokens = tokenizeString( result );
            
            delete [] result;
            
            if( tokens->size() > 0 ) {
                
                char *returnedURL = *( tokens->getElement( 0 ) );
                
                if( mServerURL != NULL ) {
                    delete [] mServerURL;
                    }
                
                mServerURL = stringDuplicate( returnedURL );
                
                printf( "Got server URL: %s\n", mServerURL );
                
                // save it
                if( savedServerURL != NULL ) {
                    delete [] savedServerURL;
                    }
                savedServerURL = stringDuplicate( mServerURL );
                }
            else {
                printf( "Error:  "
                        "Got bad response when fetching server URL.\n" );
                
                setFailed();
                }
            
            for( int i=0; i<tokens->size(); i++ ) {
                delete [] *( tokens->getElement( i ) );
                }
            delete tokens;

            delete mServerURLFetchRequest;
            mServerURLFetchRequest = NULL;
            }
        else if( stepResult == -1 ) {
            // error
            delete mServerURLFetchRequest;
            mServerURLFetchRequest = NULL;
            
            printf( "Error:  "
                    "WebRequest failed when fetching server URL.\n" );
            
            setFailed();
            }
        }
    else if( mWebRequest != NULL ) {
        int stepResult = mWebRequest->step();
    
        // don't consider processing result until at least a bit of blinking
        // has happened 
        // (thus, we give the user time to read the loading/posting
        // message before the score display is shown)
        

        // still, we want to detect a successful score post right away
        // to prevent the user from clicking BACK while we're blinking
        // and then reposting the score later
        // Note that this ignores whether the result is well-formatted
        //   oh well!
        if( stepResult == 1 ) {
            if( mTryingToPostScore ) {
                // post success...
                // prevent reposts
                scoreSent();
                
                mTryingToPostScore = false;
                }
            }
        

        // now consider processing full result after enough blinking
        if( mBlinkTime > 9 && stepResult == 1 ) {
            // done
            char goodResult = false;
            
            char *result = mWebRequest->getResult();
            
            SimpleVector<char *> *tokens = tokenizeString( result );
            
            delete [] result;

            
            if( tokens->size() >= 2 ) {
                
                int numAllTime;
                
                int numRead = sscanf( *( tokens->getElement( 0 ) ),
                                      "%d", &numAllTime );
                    
                if( numRead == 1 ) {
                        
                    if( tokens->size() >= 2 + numAllTime ) {
                        
                        mDisplayPanel.clearScores();
                        

                        int i;
                        
                        for( i=0; i<numAllTime; i++ ) {
                            
                            ScoreBundle *b =
                                new ScoreBundle( 
                                    *( tokens->getElement( 1 + i ) ) );
                            
                            mDisplayPanel.addAllTimeScore( b );
                            }
                        
                        
                        int numToday;
                
                        int numRead = 
                            sscanf( *( tokens->getElement( 1 +numAllTime ) ),
                                    "%d", &numToday );
                    
                        if( numRead == 1 ) {
                        
                            if( tokens->size() >= 2 + numAllTime + numToday ) {
                        
                                int i;
                                
                                goodResult = true;
                                
                                for( i=0; i<numToday; i++ ) {
                            
                                    ScoreBundle *b =
                                        new ScoreBundle( 
                                            *( tokens->getElement( 
                                                   2 + numAllTime + i ) ) );
                            
                                    mDisplayPanel.addTodayScore( b );
                                    }
                                }
                            }
                        }
                    }
                }


            if( !goodResult ) {

                printf( "Error:  "
                        "Got unparsable high score table from server.\n" );
            
                setFailed();
                }
            else {
                
                
                mDisplayPanel.setVisible( true );
                }
            


            
            for( int i=0; i<tokens->size(); i++ ) {
                delete [] *( tokens->getElement( i ) );
                }
            delete tokens;

            delete mWebRequest;
            mWebRequest = NULL;
            }
        else if( stepResult == -1 ) {
            // error
            delete mWebRequest;
            mWebRequest = NULL;
            
            printf( "Error:  "
                    "WebRequest failed when posting or fetching scores.\n" );

            setFailed();
            }
        }
    else if( mScoreToPost != NULL && mServerURL != NULL ) {
        // need to generate a request
        
        char *stringToHash = autoSprintf( "%s%u%u%s%s",
                                          mScoreToPost->mName,
                                          mScoreToPost->mScore,
                                          mScoreToPost->mSeed,
                                          mScoreToPost->mMoveHistory,
                                          secureSalt );
        char *hash = computeSHA1Digest( stringToHash );
        
        delete [] stringToHash;


        char *body = autoSprintf( "action=post_score"
                                  "&name=%s"
                                  "&score=%u"
                                  "&seed=%u"
                                  "&move_history=%s"
                                  "&hash=%s",
                                  mScoreToPost->mName,
                                  mScoreToPost->mScore,
                                  mScoreToPost->mSeed,
                                  mScoreToPost->mMoveHistory,
                                  hash );
        
        delete [] hash;
        
        
        delete mScoreToPost;
        mScoreToPost = NULL;
        
        mWebRequest = new WebRequest( "POST",
                                      mServerURL,
                                      body );
        

        delete [] body;
        }
    else if( mVisible && mFadeProgress == 1 && ! mDisplayPanel.isVisible() &&
             !mFailed && mServerURL != NULL ) {

        // wait until fully faded in to avoid jumping abruptly to display
        // if request is fast

        // generate a load request        
        mWebRequest = new WebRequest( "POST",
                                      mServerURL,
                                      "action=fetch_scores" );
        }
    
                                  
        
    }



void HighScoreLoadingPanel::setVisible( char inIsVisible ) {
    Panel::setVisible( inIsVisible );

    if( inIsVisible ) {
        // try again, afresh
        startConnectionTry();
        }
            
    }



char HighScoreLoadingPanel::pointerUp( int inX, int inY ) {
    
    char consumed = Panel::pointerUp( inX, inY );
    
    if( consumed ) {
        return true;
        }
    

    if( ! isSubPanelVisible() ) {

        } 
    
    return false;
    }



Color loadingMessageColor( 76/255.0, 76/255.0, 255/255.0 );

        
void HighScoreLoadingPanel::drawBase() {
    
    Panel::drawBase();
    
    if( mFadeProgress > 0 ) {
        
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );

        int textY = 100;
            
        if( mStringTransitionProgress > 0 ) {
            
            drawStringBig( mMessage, center, 
                           mW / 2,
                           textY,
                           &loadingMessageColor, 
                           mFadeProgress * mStringTransitionProgress );        
            }
        
        if( mLastMessage != NULL && mStringTransitionProgress < 1 ) {
            
            drawStringBig( mLastMessage, center, 
                           mW / 2,
                           textY,
                           &loadingMessageColor, 
                           mFadeProgress * (1 - mStringTransitionProgress) );
            }
        
            
            
        glDisable( GL_BLEND );

        

        mStatusLight.drawGrid( mFadeProgress );

        float glowVal = sin( mBlinkTime - M_PI / 2 ) * 0.5 + 0.5;

        mStatusLight.drawPieceCenter( mFadeProgress * glowVal );
        mStatusLight.drawPieceHalo( mFadeProgress * glowVal );
        }
    
    }



void HighScoreLoadingPanel::closePressed() {
    
    if( mWebRequest != NULL ) {
        delete mWebRequest;
        }
    mWebRequest = NULL;
    
    }


        
        
