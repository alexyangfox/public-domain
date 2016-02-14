

#include "minorGems/graphics/Color.h"


// 10 for comma
void drawNumeral( int inNumber,
                  float inCenterX, float inCenterY,
                  Color *inColor = NULL, float inAlpha = 1.0 );

void drawNumeralBig( int inNumber,
                     float inCenterX, float inCenterY,
                     Color *inColor = NULL, float inAlpha = 1.0 );


// A through Z and +... lowercase only
void drawLetter( char inLetter,
                 float inCenterX, float inCenterY,
                 Color *inColor = NULL, float inAlpha = 1.0 );

void drawLetterBig( char inLetter,
                    float inCenterX, float inCenterY,
                    Color *inColor = NULL, float inAlpha = 1.0 );




void drawScorePip( int inScore,
                   float inCenterX, float inCenterY,
                   Color *inColor = NULL, float inAlpha = 1.0 );



void drawScore( int inScore,
                float inRightX, float inBottomY,
                Color *inColor = NULL, float inAlpha = 1.0 );



void drawScoreBig( int inScore,
                   float inRightX, float inBottomY,
                   Color *inColor = NULL, float inAlpha = 1.0 );



// counter of at most 2 digits
void drawCounter( int inCount,
                  float inCenterX, float inCenterY,
                  Color *inColor = NULL, float inAlpha = 1.0 );

void drawCounterBig( int inCount,
                     float inCenterX, float inCenterY,
                     Color *inColor = NULL, float inAlpha = 1.0 );




enum StringAlign {
    left = 0,
    center,
    right 
    };



// a string consisting of a-z (lowercase only) '+' and ' '

// align is horizontal only
// always centerd on inY
void drawString( char *inString,
                 StringAlign inAlign,
                 float inX, float inY,
                 Color *inColor = NULL, float inAlpha = 1.0 );

void drawStringBig( char *inString,
                    StringAlign inAlign,
                    float inX, float inY,
                    Color *inColor = NULL, float inAlpha = 1.0 );
