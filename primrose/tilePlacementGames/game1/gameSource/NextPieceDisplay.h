#ifndef NEXT_PIECE_DISPLAY_INCLUDED
#define NEXT_PIECE_DISPLAY_INCLUDED


#include "GridSpace.h"
#include "ColorPool.h"

class NextPieceDisplay {
        
    public:
        
        // constructs w/ center location on screen
        NextPieceDisplay( int inX, int inY, ColorPool *inPool );

        // constructs, forcing two colors
        // inPool can be NULL
        // if inPool is NULL, update() must not be called
        NextPieceDisplay( int inX, int inY, ColorPool *inPool,
                          Color *inA, Color *inB );

        
        ~NextPieceDisplay();
        

        // removes the next piece and updates the display
        // returned color destroyed by caller
        
        // this sets the next piece spot to NULL        
        Color *getNextPiece();
        
        // updates the piece display, sliding the next piece forward
        // or generating 2 new colors
        // must be called between calls to getNextPiece
        void update();
        

        // true if getNextPiece will return the second piece from a pair
        char isSecondPiece();
        
        
        // saves state to return to upon rewind
        void saveState();
        
        void rewindState();


        // gets state that can be saved to disk
        // can be NULL if no saved state
        // copied
        char *getSavedState();
        
        void restoreFromSavedState( char *inSavedState );
        

        // steps animations
        void step();
        

        // draws onto screen
        void draw( float inAlpha = 1.0f );
        

        int mX, mY;



    private:
        ColorPool *mPool;
        
        GridSpace *mSpaces[2];
        

        float mBlinkTime;
        
                        
    };



#endif
        
        
