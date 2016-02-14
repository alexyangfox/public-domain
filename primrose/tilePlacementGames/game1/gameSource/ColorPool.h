#ifndef COLOR_POOL_INCLUDED
#define COLOR_POOL_INCLUDED


#include "GridSpace.h"


class ColorPool {
        
    public:
        
        // constructs w/ center location on screen
        ColorPool( int inX, int inY );
        
        ~ColorPool();
        

        // gets a color from the pool
        // set index of -1 (default) to pick a random color
        // color is copied
        Color *pickColor( int inIndex = -1 );
        
        // gets the symbol for a color
        char getColorblindSymbol( Color *inColor );
        

        // gets the index number of a given color
        // returns -1 if not found or if color NULL
        int getColorIndex( Color *inColor );
        

        void saveState();
        
        // gets state that can be saved to disk
        // can be NULL if no saved state
        // copied to caller
        char *getSavedState();
        
        void restoreFromSavedState( char *inSavedState );
        
        

        void registerMove();
        
        // undoes move registration
        // this is a hack that doesn't work across color addition
        // however, this is okay, because transitions always happen after
        // an even number of moves, and we only allow undo of the odd moves
        void deRegisterMove();
        
        
        int getLevel() {
            return mLevel;
            }
        
        

        // steps animations
        void step();
        

        // draws onto screen
        void draw( float inAlpha = 1.0f );
        

        int mX, mY;



    private:
        
        GridSpace *mSpaces[7];
        
        char mSomeMovesMade;
        
        int mStartingActiveColors;
        int mNumActiveColors;
        int mColorsToSkip;
        
        char mEndPhase;
        char mEndPhaseFirstColor;
        

        int mStepsBetweenUpdates;

        int mStepsUntilUpdate;
        
        int mLastStepCount;
        float mStepCountTransitionProgress;
        
        // if in the middle of deregistering a move
        char mRewinding;
        

        int mLevel;
        
        
        char *mSavedState;
        
                        
    };


#endif
        
        
