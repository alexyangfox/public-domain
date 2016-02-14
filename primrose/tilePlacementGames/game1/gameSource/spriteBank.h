#ifndef SPRITE_BANK_INCLUDED
#define SPRITE_BANK_INCLUDED


#include "minorGems/graphics/Color.h"


#define numSprites 18

enum SpriteHandle {
    gridLineTop = 0,
    gridLineBottom,
    gridLineLeft,
    gridLineRight,
    plus,
    piece,
    pieceHalo,
    pieceCenter,
    pieceBrightHalo,
    pieceBrightCenter,
    numerals,
    numeralsBig,
    abcdefghijkl,
    mnopqrstuvwx,
    yzplus,
    abcdefghijkl_big,
    mnopqrstuvwx_big,
    yzplus_big
    };




void initSpriteBank();


void freeSpriteBank();



// subsection selects a particular y region of the underlying sprite texture
void drawSprite( SpriteHandle inSpriteHandle,
                 float inCenterX, float inCenterY, 
                 float inXRadius, float inYRadius,
                 Color *inColor = NULL, float inAlpha = 1.0,
                 float inSubsectionOffset = 0,
                 float inSubsectionExtent = 1 );



#endif

