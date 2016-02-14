#include "game.h"
#include "spriteBank.h"
#include "sound.h"

#include "GridSpace.h"
#include "NextPieceDisplay.h"
#include "numeral.h"
#include "ColorPool.h"

#include "Button.h"

#include "Panel.h"
#include "MenuPanel.h"

#include "ScoreBundle.h"

#include "secureSalt.h"



#include <stdio.h>
#include <math.h>

#include <GL/gl.h>


#include "minorGems/util/random/CustomRandomSource.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/crypto/hashes/sha1.h"


unsigned int gameSeed = time( NULL );

CustomRandomSource randSource( gameSeed );


// global used to track how many sprite draws occur per frame
int spriteDrawCount = 0;



int screenW, screenH;


int pointerX, pointerY;


#define gridW 7
#define gridH 7
#define numGridSpaces 49

Color scoreColor( 255/255.0, 255/255.0, 160/255.0 );

// fade title out when app launches
float titleFade = 1.0;
char titleBlocked = false;


GridSpace *spaces[gridW][gridH];
GridSpace *allSpaces[ numGridSpaces ];


#define numButtons 6

Button *allButtons[ numButtons ];

Button *undoButton;
Button *menuButton;
Button *doneButton;
Button *playStopButton;
Button *stepButton;
Button *fastSlowButton;


#define numPanels 1

Panel *allPanels[ numPanels ];

Panel *menuPanel;



ColorPool *colorPool;

NextPieceDisplay *nextPiece;


// true if placement in progress
char piecePlaced = false;
// track the level of the piece just place (in case the level increments
// due to this placement)
int levelOfPlacement = 1;


// grid spot where last piece placed
int lastGridX = -1;
int lastGridY = -1;

int score = 0;

int moveCount = 0;

char savedStateAvailable = false;


// for smooth transition between scores
int lastScore = 0;
float scoreTransitionProgress = 1;

// for rewind
int savedScore = 0;


int chainLength = 1;


char gameOver = false;
char scoreWasSent = false;


// for scheduling a restart
char mustRestart = false;

char playerName[9];
// has the player ever set a name?
char nameSet = false;


char colorblindMode = false;

char soundOn = true;




ScoreBundle *gameToPlayback = NULL;
int gamePlaybackStep = 0;
char gamePlaybackDone = false;
char autoPlay = false;
int stepsSinceLastAutoMove = 0;
int stepsBetweenAutoMoves = 25;
char manualStep = false;
// for save/restore
int savedPlaybackStep = 0;

char playbackFast = false;



// only fetch once per app run
char *savedServerURL = NULL;


// we'll destroy these when they're finished (instead of blocking on them
// with a join)
SimpleVector<FinishedSignalThread *> threadsToDestory;

void addThreadToDestroy( FinishedSignalThread *inThread ) {
    threadsToDestory.push_back( inThread );
    }




// used to keep high score panel consisten across the destroys that
// happen to it when a new game is triggered
SimpleVector<ScoreBundle*> savedAllTimeScores;
SimpleVector<ScoreBundle*> savedTodayScores;


void clearSavedScores() {
    int i;
    for( i=0; i<savedAllTimeScores.size(); i++ ) {
        delete *( savedAllTimeScores.getElement( i ) );
        }
    
    for( i=0; i<savedTodayScores.size(); i++ ) {
        delete *( savedTodayScores.getElement( i ) );
        }

    savedAllTimeScores.deleteAll();
    savedTodayScores.deleteAll();
    }





// a subset of base-64 encoding
// converts numbers in range [0,48] to alphabetical characters
// uses the first 49 digits of base64 as described here:
//   http://tools.ietf.org/html/rfc4648
char base49Encode( unsigned int inNumber ) {
    if( inNumber < 26 ) {
        return 'A' + inNumber;
        }
    else if( inNumber < 49 ) {
        return 'a' + (inNumber - 26);
        }
    else {
        printf( "Bad base-49 encoding request: %u\n", inNumber );
        
        return 'A';
        }
    }



unsigned int base49Decode( char inLetter ) {
    unsigned int returnValue;
    
    if( inLetter < 'A' + 26 ) {
        returnValue = inLetter - 'A';
        }
    else {
        returnValue = inLetter - 'a' + 26;
        }

    if( returnValue > 48 ) {
        printf( "Bad base-49 encoding request: %c\n", inLetter );
        return 0;
        }
    else {
        return returnValue;
        }
    }




SimpleVector<char> moveHistory;
// for saving game
char *savedMoveHistory = NULL;





void newGame() {
    moveHistory.deleteAll();

    if( savedMoveHistory != NULL ) {
        delete [] savedMoveHistory;
        savedMoveHistory = NULL;
        }
    

    if( gameToPlayback == NULL ) {
        
        gameSeed = time( NULL );
        }
    else {
        gameSeed = gameToPlayback->mSeed;
        }
    
    randSource.reseed( gameSeed );
    

    piecePlaced = false;
    levelOfPlacement = 1;
    

    
    lastGridX = -1;
    lastGridY = -1;
    score = 0;
    
    moveCount = 0;
    
    savedStateAvailable = false;
    

    lastScore = 0;
    scoreTransitionProgress = 1;
    
    savedScore = 0;
    

    chainLength = 1;
    
    gameOver = false;
    scoreWasSent = false;
    
    playbackFast = false;


    pointerX = -100;
    pointerY = -100;
    

    int i=0;
    int x;
    int y;
    
    for( y=0; y<gridH; y++ ) {
        for( x=0; x<gridW; x++ ) {
            
            allSpaces[i] = 
            spaces[y][x] =
                new GridSpace( x * 40 + 19 + 21, y * 40 + 19 + 21 );
            
            i++;
            }
        }
    
    // set neighbor relationships
    for( y=0; y<gridH; y++ ) {
        for( x=0; x<gridW; x++ ) {
        
            
            if( y > 0 ) {
                spaces[y][x]->mNeighbors[ GridSpace::top ] = spaces[y-1][x];
                }
            if( y < gridH-1 ) {
                spaces[y][x]->mNeighbors[ GridSpace::bottom ] = spaces[y+1][x];
                }
            if( x > 0 ) {
                spaces[y][x]->mNeighbors[ GridSpace::left ] = spaces[y][x-1];
                }
            if( x < gridW-1 ) {
                spaces[y][x]->mNeighbors[ GridSpace::right ] = spaces[y][x+1];
                }
            }
        }


    /*
      nextPiece = new NextPieceDisplay( 19 + 21,
                                      inHeight - (19 + 41) );
    */

    // center below grid
    
    colorPool = new ColorPool( spaces[6][3]->mX,
                               screenH - 20 - 19 - 1 );
    
    nextPiece = new NextPieceDisplay( spaces[6][3]->mX,
                                      spaces[6][3]->mY + 40 + 20 + 19 + 1,
                                      colorPool );
    
    undoButton = new Button( nextPiece->mX - 60, nextPiece->mY - 20, "undo" );
    
    
    menuButton = new Button( spaces[0][0]->mX, colorPool->mY - 41 - 19, "p" );

    doneButton = new Button( undoButton->mX, 
                             menuButton->mY, "send" );

    // up a bit from undo (to give space between play button)
    stepButton = 
        new Button( undoButton->mX, undoButton->mY - 10, "step" );

    // above menu button
    fastSlowButton = 
        new Button( menuButton->mX, stepButton->mY, "fast" );


    // same spot as done button
    playStopButton = 
        new Button( doneButton->mX, doneButton->mY, "run" );

    allButtons[0] = undoButton;
    allButtons[1] = menuButton;
    allButtons[2] = doneButton;
    allButtons[3] = playStopButton;
    allButtons[4] = stepButton;
    allButtons[5] = fastSlowButton;
    
    undoButton->setVisible( false );
    menuButton->forceVisible();
    doneButton->setVisible( false );
    
    playStopButton->setVisible( false );
    stepButton->setVisible( false );
    
    fastSlowButton->setVisible( false );
    
    
    if( gameToPlayback != NULL ) {
        playStopButton->setVisible( true );
        stepButton->setVisible( true );
        }

    menuPanel = new MenuPanel( screenW, screenH );
    
    allPanels[0] = menuPanel;
    
    

    if( mustRestart ) {
        // previous menu panel requested a restart
        // show this new one at full blend at first, then fade out to 
        // restarted game
        if( gameToPlayback == NULL ) {
            menuPanel->forceVisible();
            }
        else {
            // it was high score display that triggered restart
            ( (MenuPanel *)menuPanel )->forceFadeOutScoreDisplay();
            }
        }
    
    menuPanel->setVisible( false );
    }



void endGame() {
    
    int i;
    for( i=0; i<numGridSpaces; i++ ) {
        delete allSpaces[i];
        }
    for( i=0; i<numButtons; i++ ) {
        delete allButtons[i];
        }
    for( i=0; i<numPanels; i++ ) {
        delete allPanels[i];
        }
    
    delete nextPiece;
    delete colorPool;
    }



void restartGame() {
    mustRestart = true;

    // clear any game in playback
    if( gameToPlayback != NULL ) {
        delete gameToPlayback;
        }
    gameToPlayback = NULL;
    gamePlaybackStep = 0;
    gamePlaybackDone = false;
    
    }



char isGameOver() {
    return gameOver;
    }


void scoreSent() {
    // don't let them send it again
    doneButton->setVisible( false );
    scoreWasSent = true;

    SettingsManager::setHashingOn( true );
    
    SettingsManager::setSetting( "scoreWasSent", 1 );
    
    SettingsManager::setHashingOn( false );
    }




void playbackGame( ScoreBundle *inBundle ) {
    if( gameToPlayback != NULL ) {
        delete gameToPlayback;
        }
    gameToPlayback = inBundle;
    gamePlaybackStep = 0;
    gamePlaybackDone = false;
    
    // default to not autoPlay
    autoPlay = false;
    stepsSinceLastAutoMove = 0;
    manualStep = false;
    
    //printf( "Playing back game with moves:\n%s\n", inBundle->mMoveHistory );
    

    mustRestart = true;
    }



void initFrameDrawer( int inWidth, int inHeight ) {
    
    #ifdef IPHONE
        // working directory on iPhone is inside bundle, but 
        // we can't save persistent files there.
        // Document directory is backed up.
        SettingsManager::setDirectoryName( "../Documents" );
    #endif




    // don't use the same salt every time

    // read the last-used salt off the disk


    char *lastHashSalt = SettingsManager::getStringSetting( "hashSalt" );

    if( lastHashSalt != NULL ) {
        
        // salt used is always saved salt plus internal security salt
        
        char *totalSalt = autoSprintf( "%s%s", lastHashSalt, secureSalt );

        delete [] lastHashSalt;
        
        SettingsManager::setHashSalt( totalSalt );
        
        delete [] totalSalt;
        }
    else {
        SettingsManager::setHashSalt( secureSalt );
        }
    
    SettingsManager::setHashingOn( false );
    
    

    int i;
        
    for( i=0; i<10; i++ ) {
        //printf( "Rand output: %u\n", randSource.getRandomInt() );
        }
    

    screenW = inWidth;
    screenH = inHeight;
    
    initSpriteBank();

    
    char *nameSetting = SettingsManager::getStringSetting( "name" );
    
    if( nameSetting != NULL ) {
        int nameLength = strlen( nameSetting );
        
        if( nameLength > 8 ) {
            nameLength = 8;
            }
        
        memcpy( playerName, nameSetting, nameLength );

        playerName[ nameLength ] = '\0';
        
        delete [] nameSetting;
        }
    else {
        memcpy( playerName, "anon", 5 );
        }
    
    char found;
    
    nameSet = SettingsManager::getIntSetting( "nameSet", &found );
    
    if( !found ) {
        nameSet = false;
        }
    

    colorblindMode = SettingsManager::getIntSetting( "colorblindMode",
                                                     &found );

    if( !found ) {
        colorblindMode = false;
        }
    
    soundOn = SettingsManager::getIntSetting( "soundOn",
                                              &found );

    if( !found ) {
        soundOn = true;
        }
    
    
    
    initSound();
    //exit( 0 );
    
    if( soundOn ) {    
        setSoundPlaying( true );
        }
    


    newGame();



    // should we restore from disk?

    // saved games are secure

    // note that I realize this is weak security.
    // all someone needs to do is figure out the secure salt, which
    // they can do by looking at the code.  However, it's still
    // harder than simply editing the settings files with a text
    // editor.  Strong security is impossible here.
    SettingsManager::setHashingOn( true );


    int wasSaved = SettingsManager::getIntSetting( "saved", &found );
    
    if( found && wasSaved ) {
        char anyFailed = false;
        

        char *state = SettingsManager::getStringSetting( "moveHistorySaved" );

        if( state != NULL ) {
            moveHistory.setElementString( state );
            delete [] state;
            }
        else {
            anyFailed = true;
            }
        
        state = 
            SettingsManager::getStringSetting( "nextPieceDisplaySaved" );
        
        if( state != NULL ) {
            nextPiece->restoreFromSavedState( state );
            delete [] state;
            }
        else {
            anyFailed = true;
            }
        
        state = 
            SettingsManager::getStringSetting( "colorPoolSaved" );
        
        if( state != NULL ) {
            colorPool->restoreFromSavedState( state );
            delete [] state;
            }
        else {
            anyFailed = true;
            }
        
        
        state = 
            SettingsManager::getStringSetting( "randStateSaved" );
        
        if( state != NULL ) {
            unsigned int randState;
            
            int numRead = sscanf( state, "%u", &randState );
            
            if( numRead == 1 ) {
                randSource.restoreFromSavedState( randState );
                }
            else {
                anyFailed = true;
                }
        
            delete [] state;
            }
        else {
            anyFailed = true;
            }
        

        
        state = 
            SettingsManager::getStringSetting( "gameSeedSaved" );
        
        if( state != NULL ) {
            sscanf( state, "%u", &gameSeed );
            
            delete [] state;
            }
        else {
            anyFailed = true;
            }
        


        score = SettingsManager::getIntSetting( "scoreSaved", &found );
        
        if( !found ) {
            anyFailed = true;
            score = 0;
            }
        
        gameOver = (char)( 
            SettingsManager::getIntSetting( "gameOverSaved", &found ) );
        
        if( !found ) {
            anyFailed = true;
            gameOver = 0;
            }

        scoreWasSent = (char)( 
            SettingsManager::getIntSetting( "scoreWasSent", &found ) );
        
        if( !found ) {
            anyFailed = true;

            // assume score sent if flag not present
            scoreWasSent = 1;
            }
        
        if( gameOver && !scoreWasSent ) {
            doneButton->setVisible( true );
            }
        

        state = SettingsManager::getStringSetting( "gridSaved" );

        if( state != NULL ) {
        
            int numParts;
            char **parts = split( state, "#", &numParts );
            
            if( numParts == numGridSpaces ) {
            
                for( int i=0; i<numParts; i++ ) {
                    int index;
                    
                    int numRead = sscanf( parts[i], "%d", &index );
                    
                    if( numRead == 1 ) {
                        Color *c = NULL;
                        
                        if( index >= 0 ) {
                            c = colorPool->pickColor( index );
                            }
                        allSpaces[i]->setColor( c );
                        }
                    else {
                        anyFailed = true;
                        }
                    }
                }
            else {
                anyFailed = true;
                }
            
            for( int i=0; i<numParts; i++ ) {
                delete []  parts[i];
                }
            delete [] parts;
            
            delete [] state;
            }  
        else {
            anyFailed = true;
            }



        int playingBackGame = 
            SettingsManager::getIntSetting( "playingBackGameSaved", &found );
        
        if( found ) {
            
            if( playingBackGame ) {
        
                state = 
                    SettingsManager::getStringSetting( "gameToPlaybackSaved" );
            
                if( state != NULL ) {
                    gameToPlayback = new ScoreBundle( state );

                    delete [] state;

                    if( !gameOver ) {
                    
                        playStopButton->setVisible( true );
                        stepButton->setVisible( true );
                        }
                    else {
                        gamePlaybackDone = true;
                        }
                

                    // never allow sending score on a playback game
                    doneButton->setVisible( false );
                    }
                else {
                    anyFailed = true;
                    }
                    
                gamePlaybackStep = SettingsManager::getIntSetting( 
                    "gamePlaybackStepSaved", &found );
            
                if( !found ) {
                    anyFailed = true;
                
                    gamePlaybackStep = 0;
                    }
                }
            }
        else {
            anyFailed = true;
            }
        
        
        
        if( anyFailed ) {
            // game was supposedly saved, but we failed to load
            // the whole thing (hash-checked)... 
            // corrupted, so don't trust any of it
        
            // don't want to load only certain parts of a saved game
            // correctly (like the score, but not the grid)

            printf( "Failed to load part of saved game."
                    "  Starting new game instead.\n" );
            

            endGame();
            newGame();
            }
        
        }
    
    // back to normal, insecure settings mode
    SettingsManager::setHashingOn( false );
    



    if( colorblindMode ) {
        
        for( i=0; i<4 && !titleBlocked; i++ ) {
            if( ! spaces[5][ 2 + i ]->isEmpty() ) {
                titleBlocked = true;
                }
            }
        }

    }




void freeFrameDrawer() {
    
    if( savedStateAvailable ) {
        printf( "Saving game state %d moves in\n", moveCount );
        
        
        // secure

        // new hash each time
        char *saltSeed = autoSprintf( "%d%s\n", time( NULL ), secureSalt );
        char *newSalt = computeSHA1Digest( saltSeed );
        delete [] saltSeed;
        
        // save so that it can be used when loading game next time
        SettingsManager::setSetting( "hashSalt", newSalt );


        // salt used for hashing is always saved salt plus internal 
        // security salt
        
        char *totalSalt = autoSprintf( "%s%s", newSalt, secureSalt );
        delete [] newSalt;

        
        SettingsManager::setHashSalt( totalSalt );
        delete [] totalSalt;
                

        SettingsManager::setHashingOn( true );
        

        // save the last saved (known good) state out to disk

        SettingsManager::setSetting( "moveHistorySaved", savedMoveHistory );
        
        
        
        char *state = nextPiece->getSavedState();
        
        SettingsManager::setSetting( "nextPieceDisplaySaved", state );
        
        delete [] state;
        
        
        state = colorPool->getSavedState();
        
        if( state != NULL ) {
        
            SettingsManager::setSetting( "colorPoolSaved", state );
            
            delete [] state;
            }
    
        
        state = autoSprintf( "%u", randSource.getSavedState() );
        
        SettingsManager::setSetting( "randStateSaved", state );
        
        delete [] state;


        state = autoSprintf( "%u", gameSeed );
        
        SettingsManager::setSetting( "gameSeedSaved", state );
        
        delete [] state;
        

        SettingsManager::setSetting( "scoreSaved", savedScore );
    

        SettingsManager::setSetting( "gameOverSaved", (int)gameOver );
        
        SettingsManager::setSetting( "scoreWasSent", (int)scoreWasSent );
        
        
        int i;
        SimpleVector<char*> gridState;
        
        for( i=0; i<numGridSpaces; i++ ) {
            Color *c = allSpaces[i]->getSavedColor();
            
            int index = colorPool->getColorIndex( c );
            
            gridState.push_back( autoSprintf( "%d", index ) );
            }

        char **gridStateArray = gridState.getElementArray();
        
        state = join( gridStateArray, gridState.size(), "#" );
    
        delete [] gridStateArray;
        
        for( i=0; i<numGridSpaces; i++ ) {
            delete [] *( gridState.getElement( i ) );
            }
            
        
        SettingsManager::setSetting( "gridSaved", state );
        
        delete [] state;
        

        if( gameToPlayback != NULL ) {
        
            state = autoSprintf( "%s#%u#%u#%s",
                                 gameToPlayback->mName,
                                 gameToPlayback->mScore,
                                 gameToPlayback->mSeed,
                                 gameToPlayback->mMoveHistory );
            
            SettingsManager::setSetting( "gameToPlaybackSaved", state );
            delete [] state;
            
            SettingsManager::setSetting( "gamePlaybackStepSaved", 
                                         savedPlaybackStep );
            
            SettingsManager::setSetting( "playingBackGameSaved", 1 );
            }
        else {
            SettingsManager::setSetting( "playingBackGameSaved", 0 );
            }


        SettingsManager::setSetting( "saved", "1" );

        
        SettingsManager::setHashingOn( false );
        }    


    endGame();

    freeSpriteBank();
    
    freeSound();
    

    // we don't do this in endGame because sometimes we're ending
    // last game before starting a new playback
    if( gameToPlayback != NULL ) {
        delete gameToPlayback;
        }
    gameToPlayback = NULL;

    
    if( savedServerURL != NULL ) {
        delete [] savedServerURL;
        }
    savedServerURL = NULL;
    

    if( savedMoveHistory != NULL ) {
        delete [] savedMoveHistory;
        }
    savedMoveHistory = NULL;
    
    
    clearSavedScores();


    // what about any threads?

    for( int i=0; i<threadsToDestory.size(); i++ ) {
        FinishedSignalThread *thread = *( threadsToDestory.getElement(i) );
        
        if( !thread->isFinished() ) {
            printf( "Warning:  Thread not finished at freeFrameDrawer.\n"
                    "          Letting it leak instead of blocking.\n" );
            }
        else {
            delete thread;
            }
        
        }
    }





void computeEarLoudness( int inPieceX, 
                       float *outLeftVolume, float *outRightVolume ) {
    float rightEarX = 4;
    float leftEarX = 2;
    
    float screenDistance = 5;
    
    // use triangles
    float vLeft = 
        sqrt( pow( screenDistance, 2 ) + pow( leftEarX - inPieceX, 2 ) );
    float vRight = 
        sqrt( pow( screenDistance, 2 ) + pow( rightEarX - inPieceX, 2 ) );


    *outLeftVolume = pow( screenDistance, 2 ) / pow( vLeft, 2 );
    *outRightVolume = pow( screenDistance, 2 ) / pow( vRight, 2 );    
    }



int computeGroupScoreFactor( int inGroupSize ) {
    if( inGroupSize < 11 ) {
        return (int)( 10 * sqrt( inGroupSize ) );
        }
    else {
        return inGroupSize + 21;
        }
    }



// one round of non-chained clearing
// returns true if some pieces cleared
char checkAndClear() {
    char someCleared = false;
    
    int i;
    int j;

    for( i=0; i<numGridSpaces; i++ ) {
        allSpaces[i]->mVisited =  false;
        allSpaces[i]->mChecked =  false;
        allSpaces[i]->mMarkedForClearing =  false;
        }

    char playingClearingSound = false;
    
    for( j=0; j<numGridSpaces; j++ ) {
        // clear visited flags so that they only mark one group
        // at a time
        for( i=0; i<numGridSpaces; i++ ) {
            allSpaces[i]->mVisited =  false;
            }
        

        GridSpace *space = allSpaces[j];
        
        if( ! space->mChecked ) {
            
            Color *c = space->checkSurrounded();
        
            if( c != NULL ) {
                
                // space returns its own color if its group is
                // surrounded by nothing but edges
                // must be surrounded by another color (not just edges)
                // to clear
                if( ! space->colorMatches( c ) ) {
                
                    // all visited spaces part of group
                
                    // tally score
                    int groupSize = 0;
                
                    int groupColorIndex = -1;
                
                    // accumulate average volume
                    float leftEarWeight = 0;
                    float rightEarWeight = 0;
                

                    for( i=0; i<numGridSpaces; i++ ) {
                        if( allSpaces[i]->mVisited ) {
                            allSpaces[i]->mMarkedForClearing = true;
                        
                            groupColorIndex = allSpaces[i]->getColorIndex();
                        
                            groupSize ++;

                            float lV, rV;
                            int x = i % gridW;
                        
                            computeEarLoudness( x, &lV, &rV );
                            leftEarWeight += lV;
                            rightEarWeight += rV;
                            }
                        }

                
                
                    // play at most one per round
                    if( !playingClearingSound && soundOn ) {
                    
                        // start sounds
                        playClearingSound( groupColorIndex, groupSize,
                                           chainLength,
                                           leftEarWeight / groupSize,
                                           rightEarWeight / groupSize );
                        playingClearingSound = true;
                        }
                
                
                    // set score for them all
                    for( i=0; i<numGridSpaces; i++ ) {
                        if( allSpaces[i]->mVisited ) {
                            /*
                              // old scoring
                            allSpaces[i]->mScore = 
                                (int)( pow( groupSize, 2 ) )
                                *
                                (int)( pow( chainLength, 4 ) )
                                * 
                                (int)( pow( levelOfPlacement, 2 ) );
                            */
                            // new scoring
                            allSpaces[i]->mScore = 
                                (int) (
                                    // level bonus added in instead of 
                                    // multiplied
                                    ( computeGroupScoreFactor( groupSize )
                                      + 12 * (levelOfPlacement - 1) )
                                    *
                                    pow( chainLength, 4 ) );
                            }
                        }
                    }
                

                delete c;
                
                }            
            }
        }

    // clear any that are marked 
    for( i=0; i<numGridSpaces; i++ ) {
        if( allSpaces[i]->mMarkedForClearing ) {
            allSpaces[i]->flipToClear();
            someCleared = true;
            }
        }

    if( someCleared ) {
        chainLength ++;
        }
    
    return someCleared;
    }



void saveStateForUndo() {
    int i;
    
    for( i=0; i<numGridSpaces; i++ ) {
        allSpaces[i]->saveState();
        }
    nextPiece->saveState();
    
    colorPool->saveState();
    
    randSource.saveState();
    
    savedScore = score;

    if( savedMoveHistory != NULL ) {
        delete [] savedMoveHistory;
        }
    savedMoveHistory = moveHistory.getElementString();
    

    savedPlaybackStep = gamePlaybackStep;
    

    savedStateAvailable = true;
    }




void placeNextPieceAt( unsigned int inSpaceNumber ) {
    
    // always disable immediately after piece placement
    // to avoid player trying to undo during animation
        
    undoButton->setVisible( false );
    
    char considerNonActive = true;
    if( nextPiece->isSecondPiece() ) {
        considerNonActive = false;
        }


    if( moveCount == 0 ) {
        // no saved state, because no moves made yet
        
        // save starting state here
        saveStateForUndo();
        }


    char encodedMove = base49Encode( inSpaceNumber );
    
    moveHistory.push_back( encodedMove );


    // modulate random source by player's move

    // draw some random numbers and discard them

    // the number of draws depends in inSpaceNumber AND the current
    // state of the generator (so that reordering a sequence of moves
    // DOES NOT lead to the same generator state).

    int numToDraw = randSource.getRandomBoundedInt( 0, inSpaceNumber );
    

    // this approach works well because it requires no extra state
    // and it doesn't compromise the statistical properties of the random
    // generator (like mixing inSpaceNumber into the generator seed can)
    for( int d=0; d<numToDraw; d++ ) {
        randSource.getRandomInt();
        }


    
    moveCount ++;
    
    Color *c = nextPiece->getNextPiece();
    

    if( soundOn ) {
        
        int x = inSpaceNumber % gridW;
        
        float leftVolume, rightVolume;
        
        computeEarLoudness( x, &leftVolume, &rightVolume );
        
        playPlacementSound( colorPool->getColorIndex( c ),
                            leftVolume, rightVolume );
        }
    

    allSpaces[inSpaceNumber]->setColor( c );
    piecePlaced = true;
    levelOfPlacement = colorPool->getLevel();
                
    colorPool->registerMove();
                            
                
    lastGridY = inSpaceNumber / gridW;
    lastGridX = inSpaceNumber % gridW;
                
    // reset chain length counter
    chainLength = 1;
    
    
    if( !considerNonActive ) {
        // just placed in an active spot (second piece)
        
        
        // all to non-active
        for( int i=0; i<numGridSpaces; i++ ) {
            allSpaces[i]->mActive = false;
            }
        }
    }




void drawFrame() {
    if( mustRestart ) {
        endGame();


        // we need to prevent people from using the last saved state,
        // which lingers after a new game is started, as a checkpoint.
        // If they play no moves in the new game before quitting, they
        // can restart Primrose later and get back into the old saved state,
        // EVEN IF they made some moves past the saved state before starting
        // the new game.

        // disable the last saved state:

        SettingsManager::setHashingOn( true );

        SettingsManager::setSetting( "saved", "0" );
        
        SettingsManager::setHashingOn( false );



        newGame();

        mustRestart = false;
        }


    int i;

    
    // look for threads that are finished
    for( int i=0; i<threadsToDestory.size(); i++ ) {
        FinishedSignalThread *thread = *( threadsToDestory.getElement(i) );
        
        if( thread->isFinished() ) {
            delete thread;
            threadsToDestory.deleteElement( i );
            i--;
            }
        }


    if( titleFade > 0 ) {
        titleFade -= 0.02;
        
        if( titleFade < 0 ) {
            titleFade = 0;
            }
        }

    
    
    char animDone = true;
    
    for( i=0; i<numGridSpaces; i++ ) {
        allSpaces[i]->step();
        if( playbackFast ) {
            // double-step
            allSpaces[i]->step();
            }
        
        animDone &= allSpaces[i]->isAnimationDone();
        }

    for( i=0; i<numButtons; i++ ) {
        allButtons[i]->step();
        }

    for( i=0; i<numPanels; i++ ) {
        allPanels[i]->step();
        }
    
    nextPiece->step();
    
    colorPool->step();

    if( playbackFast ) {
        // double-step
        nextPiece->step();
        
        colorPool->step();
        }
        



    if( piecePlaced && animDone ) {
        // done with placement, all placement (or clearing) animations done

        
        /*
        // accumulate any points scored
        lastScore = score;
        
        for( i=0; i<numGridSpaces; i++ ) {
            if( allSpaces[i]->mScore > 0 ) {
                score += allSpaces[i]->mScore;
                
                allSpaces[i]->mScore = 0;
                }
            }
        
        if( lastScore != score ) {
            scoreTransitionProgress = 0;
            }
        */



        // check if any more clear (which will start more animations)
        lastScore = score;
        
        if( !checkAndClear() ) {
            
            // check if board full
            char full = true;
            
            for( i=0; i<numGridSpaces && full; i++ ) {
                if( allSpaces[i]->isEmpty() ) {
                    full = false;
                    }
                }
            
            if( full ) {
                gameOver = true;
                
                if( gameToPlayback == NULL ) {
                    // don't save final state yet
                    // need to update nextPiece first.

                    doneButton->setVisible( true );
                    }
                else {
                    // done with playback
                    playStopButton->setVisible( false );
                    stepButton->setVisible( false );
                    fastSlowButton->setVisible( false );
                    playbackFast = false;
                    gamePlaybackDone = true;
                    }
                }
            
            

            

            nextPiece->update();

            
            if( gameOver ) {
                // save state again incase game terminated after
                // game ends
                // can't undo back beyond this point anyway,
                // now that game is over!
                saveStateForUndo();
                }
            

            piecePlaced = false;

        
            
        

            if( nextPiece->isSecondPiece() ) {
                // allow undo of first piece
                
                if( !full && gameToPlayback == NULL ) {
                    undoButton->setVisible( true );
                    }
                
                // set grid spaces in same row/column to active
                int x;
                int y;
            
            
                char anyActive = false;
            
                for( y=0; y<gridH; y++ ) {
                    GridSpace *space = spaces[y][lastGridX];
                
                    if( space->isEmpty() ) {
                        space->mActive = true;
                        anyActive = true;
                        }
                    }
            
                for( x=0; x<gridW; x++ ) {
                    GridSpace *space = spaces[lastGridY][x];
                
                    if( space->isEmpty() ) {
                        space->mActive = true;
                        anyActive = true;
                        }
                    }

            
                if( !anyActive ) {
                    // all empty active
                    for( i=0; i<numGridSpaces; i++ ) {
                        if( allSpaces[i]->isEmpty() ) {
                            allSpaces[i]->mActive = true;
                            }
                        }
                    }
                }
            else {
                // placing new first piece
                undoButton->setVisible( false );

                // save state here to support undoing back to it after
                // next move
                saveStateForUndo();
                }
            }
        
        }
    
    // recheck animDone
    if( animDone ) {    
        animDone = true;
        
        for( i=0; i<numGridSpaces; i++ ) {
            allSpaces[i]->step();
            
            animDone &= allSpaces[i]->isAnimationDone();
            }
        }
    


    // check if we should auto/manual step replay game
    // only step when animation from previous step done
    // pause when sub-panels visible
    char somePanelVisible = false;
    // skip drawing board when one fully visible
    char somePanelFullyVisible = false;
    
    for( i=0; i<numPanels && !somePanelVisible; i++ ) {
        if( allPanels[i]->isVisible() ) {
            somePanelVisible = true;
            }
        if( allPanels[i]->isFullyVisible() ) {
            somePanelFullyVisible = true;
            }
        }



    if( animDone && !somePanelVisible ) {
        
        if( gameToPlayback != NULL && !gamePlaybackDone ) {
            
            if( autoPlay && 
                ( playbackFast || 
                  stepsSinceLastAutoMove > stepsBetweenAutoMoves )
                ||
                manualStep ) {
                
            
                if( gamePlaybackStep < gameToPlayback->mNumMoves ) {
                    
                    unsigned int spaceNumber = base49Decode( 
                        gameToPlayback->mMoveHistory[ gamePlaybackStep ] );
                    
                    placeNextPieceAt( spaceNumber );

                    gamePlaybackStep ++;
                    }


                stepsSinceLastAutoMove = 0;
                manualStep = false;
                }
            else if( autoPlay ) {
                // only step after animations done
                stepsSinceLastAutoMove ++;
                }
            else if( !autoPlay ) {
                // animations done, re-enable
                stepButton->setVisible( true );
                }
            
            }
        }
    

    spriteDrawCount = 0;
    


    if( !somePanelFullyVisible ) {
        
        for( i=0; i<numGridSpaces; i++ ) {
            allSpaces[i]->drawGrid();
            }

        for( i=0; i<numButtons; i++ ) {
            allButtons[i]->draw();
            }

        for( i=0; i<numGridSpaces; i++ ) {
            allSpaces[i]->drawPieceCenter();
            }
        for( i=0; i<numGridSpaces; i++ ) {
            allSpaces[i]->drawPieceHalo();
            }

        
        nextPiece->draw();
        
        colorPool->draw();
    
        

        // additive needed for smooth cross-fade between last score and 
        // new score
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        
        if( scoreTransitionProgress < 1 ) {
            drawScoreBig( lastScore, 320 - 19, nextPiece->mY + 41, 
                          &scoreColor,
                          1 - scoreTransitionProgress );
            }
        
        if( scoreTransitionProgress > 0 ) {  
            drawScoreBig( score, 320 - 19, nextPiece->mY + 41, &scoreColor,
                          scoreTransitionProgress );
            }
        

        
        if( titleFade > 0 && !titleBlocked ) {
            drawStringBig( "primrose", left, 
                           spaces[5][2]->mX,
                           spaces[5][2]->mY,
                           &scoreColor,
                           titleFade );
            }
        
        
        

        glDisable( GL_BLEND );
    

        if( scoreTransitionProgress < 1 ) {
            scoreTransitionProgress += 0.075;
            if( scoreTransitionProgress > 1 ) {
                scoreTransitionProgress = 1;
                }
            }
        }
    

    
    for( i=0; i<numPanels; i++ ) {
        allPanels[i]->draw();
        }


    //printf( "%d sprites drawn this frame\n", spriteDrawCount );

    }



void undoMove() {
    int i;
    
    for( i=0; i<numGridSpaces; i++ ) {
        allSpaces[i]->rewindState();
        }
    nextPiece->rewindState();
    
    colorPool->deRegisterMove();
    
    randSource.rewindState();

    moveCount--;
    

    if( moveHistory.size() > 0 ) {
        moveHistory.deleteElement( moveHistory.size() - 1 );
        }
    

    int temScore = score;
    score = savedScore;
    lastScore = temScore;
    
    scoreTransitionProgress = 0;
    
    
    // only one step of undo allowed
    undoButton->setVisible( false );
    }




void pointerDown( float inX, float inY ) {
    pointerX = (int)inX;
    pointerY = (int)inY;
    }


void pointerMove( float inX, float inY ) {
    pointerX = (int)inX;
    pointerY = (int)inY;
    
    
    }






void pointerUp( float inX, float inY ) {
    pointerX = (int)inX;
    pointerY = (int)inY;
    
    //printf( "Pointer Up at %f,%f\n", pointerX, pointerY );
    
    
    
    // pass through to visible panels
    char somePanelVisible = false;
    
    int i;
    
    for( i=0; i<numPanels; i++ ) {
        if( allPanels[i]->isVisible() 
            || 
            allPanels[i]->isSubPanelVisible() ) {
            
            somePanelVisible = true;
            allPanels[i]->pointerUp( pointerX, pointerY );
            }
        }

    if( somePanelVisible ) {
        // don't pass click to grid
        return;
        }

    char someButtonPressed = false;
    

    for( i=0; i<numButtons; i++ ) {
        if( allButtons[i]->isVisible() && 
            allButtons[i]->isInside( pointerX, pointerY ) ) {
            
            someButtonPressed = true;
            
            if( allButtons[i] == undoButton ) {
                undoMove();
                }
            else if( allButtons[i] == menuButton ) {
                menuPanel->setVisible( true );
                }
            else if( allButtons[i] == doneButton ) {
                char *moveString = moveHistory.getElementString();
                
                ScoreBundle *b = new ScoreBundle( playerName, score, gameSeed,
                                                  moveString );
                delete [] moveString;

                //printf( "posting score\n" );
                
                ( (MenuPanel*)menuPanel )->postScore( b );
                } 
            else if( allButtons[i] == playStopButton ) {
                if( autoPlay ) {
                    playStopButton->setString( "run" );
                    
                    playbackFast = false;
                    }
                else {
                    playStopButton->setString( "stop" );
                    
                    playbackFast = false;
                    fastSlowButton->setString( "fast" );
                    }
                autoPlay = !autoPlay;    

                stepButton->setVisible( !autoPlay );
                fastSlowButton->setVisible( autoPlay );

                // re-enable display of active markers if coming out of
                // fast mode here.
                for( i=0; i<numGridSpaces; i++ ) {
                    allSpaces[i]->mActiveMarkerVisible = !playbackFast;
                    }

                }
            else if( allButtons[i] == stepButton ) {
                manualStep = true;
                // disable during step animations
                stepButton->setVisible( false );
                }
            else if( allButtons[i] == fastSlowButton ) {
                if( playbackFast ) {
                    fastSlowButton->setString( "fast" );
                    }
                else {
                    fastSlowButton->setString( "slow" );
                    }
                playbackFast = !playbackFast;

                // disable display of active markers in fast mode
                // (because they flicker in an annoying way)
                for( i=0; i<numGridSpaces; i++ ) {
                    allSpaces[i]->mActiveMarkerVisible = !playbackFast;
                    }
                
                }
            }
        }


    if( someButtonPressed ) {
        // don't pass click to grid
        return;
        }
    
        
    
    if( gameToPlayback != NULL ) {
        // don't pass click to grid
        return;
        }
    

    if( piecePlaced ) {
        // last placement still in progress
        // ignore grid clicks
        return;
        }
    
    // don't count clicks below a certain point as grid clicks
    // (don't want player to miss clicking a button and place a piece by
    // accident)

    if( pointerY > spaces[6][0]->mY + 29 ) {
        // no grid clicks this far down
        return;
        }
    


    char considerNonActive = true;
    if( nextPiece->isSecondPiece() ) {
        considerNonActive = false;
        }
    

    unsigned int x;
    unsigned int y;
    

    // sloppy pointing:
    // if none hit, look for closest space
    char noneHit = true;

    int closestIndex = -1;
    float closestDistanceSquared = 99999999;
    float closestXDeltaSquared = 99999999;
    float closestYDeltaSquared = 99999999;
    

    for( y=0; y<gridH && noneHit; y++ ) {
        for( x=0; x<gridW && noneHit; x++ ) {
        

            GridSpace *space = spaces[y][x];
            
            if( space->isEmpty()
                && 
                ( considerNonActive || space->mActive ) ) {
                
                unsigned int i = y * gridW + x;

                if( space->isInside( pointerX, pointerY ) ) {
                    closestIndex = i;
                    noneHit = false;
                    }
                else {
                    
                    
                    float xDeltaSquared = 
                        (space->mX - pointerX) * (space->mX - pointerX);
                    float yDeltaSquare =
                        (space->mY - pointerY) * (space->mY - pointerY);
                    
                    float distanceSquared = xDeltaSquared + yDeltaSquare;
                    
                    if( distanceSquared < closestDistanceSquared ) {
                        closestDistanceSquared = distanceSquared;
                        closestIndex = i;

                        closestXDeltaSquared = xDeltaSquared;
                        closestYDeltaSquared = yDeltaSquare;
                        }
                    }
                }
            }
        }

    // ignore clicks more than a 3/4 space-width away from 
    // closest space center (space width is 40)
    float limit = 30 * 30;
    
    // of course, if piece hit directly, ignore distance
    if( ( closestXDeltaSquared < limit
          &&
          closestYDeltaSquared < limit 
          &&
          closestIndex >= 0 )
        ||
        ! noneHit ) {
        
        placeNextPieceAt( closestIndex );
        }
    
    
    
    }




void addToScore( int inPointsToAdd ) {
    
    score += inPointsToAdd;
    
    scoreTransitionProgress = 0;
    }



char getNameSet() {
    return nameSet;
    }



char *getName() {
    return playerName;
    }



void saveName() {
    
    if( strlen( playerName ) == 0 ) {
        // default to anon
        memcpy( playerName, "anon", 5 );
        }
    

    SettingsManager::setSetting( "name", playerName );
    SettingsManager::setSetting( "nameSet", 1 );

    nameSet = true;
    }



char getColorblindMode() {
    return colorblindMode;
    }



void setColorblindMode( char inOn ) {
    colorblindMode = inOn;
    
    SettingsManager::setSetting( "colorblindMode", (int)colorblindMode );
    }



char getColorblindSymbol( Color *inColor ) {
    return colorPool->getColorblindSymbol( inColor );
    }



int getColorIndex( Color *inColor ) {
    return colorPool->getColorIndex( inColor );
    }




char getSoundOn() {
    return soundOn;
    }


void setSoundOn( char inOn ) {
    soundOn = inOn;
    
    SettingsManager::setSetting( "soundOn", (int)soundOn );

    setSoundPlaying( soundOn );
    }
