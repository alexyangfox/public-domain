
#include "numeral.h"
#include "spriteBank.h"

#include <GL/gl.h>

#include <math.h>
#include <ctype.h>
#include <string.h>



void drawNumeral( int inNumber,
                  float inCenterX, float inCenterY,
                  Color *inColor, float inAlpha ) {


    

    drawSprite( numerals,
                inCenterX, inCenterY,
                8, 10,
                inColor, inAlpha,
                inNumber * 20 / 256.0f,
                20 / 256.0f );

    }



void drawNumeralBig( int inNumber,
                     float inCenterX, float inCenterY,
                     Color *inColor, float inAlpha ) {


    
    drawSprite( numeralsBig,
                inCenterX, inCenterY,
                16, 20,
                inColor, inAlpha,
                inNumber * 40 / 512.0f,
                40 / 512.0f );

    }



void drawLetter( char inLetter,
                 float inCenterX, float inCenterY,
                 Color *inColor, float inAlpha ) {
    
    // default to 'a'
    SpriteHandle handle = abcdefghijkl;
    int indexOffset = 0;
    
    float imageHeight = 256.0f;
    
    

    if( inLetter == '+' ) {
        handle = yzplus;
        indexOffset = 2;
        imageHeight = 64.0f;
        }
    else {
        inLetter = (char)tolower( inLetter );
        
        if( inLetter >= 'a' && inLetter <= 'z' ) {
        
            if( inLetter <= 'l' ) {
                handle = abcdefghijkl;
                indexOffset = inLetter - 'a';
                }
            else if( inLetter <= 'x' ) {
                handle = mnopqrstuvwx;
                indexOffset = inLetter - 'm';
                }
            else {
                handle = yzplus;
                indexOffset = inLetter - 'y';
                imageHeight = 64.0f;
                }
            }
        else {
            printf( "Error:  letter out of 'a-z' or '+' range: %c\n",
                    inLetter );
            }
        }
    
    

    drawSprite( handle,
                inCenterX, inCenterY,
                8, 10,
                inColor, inAlpha,
                indexOffset * 20 / imageHeight,
                20 / imageHeight );
    }



void drawLetterBig( char inLetter,
                    float inCenterX, float inCenterY,
                    Color *inColor, float inAlpha ) {
    
    // default to 'a'
    SpriteHandle handle = abcdefghijkl_big;
    int indexOffset = 0;
    
    float imageHeight = 512.0f;
    
    

    if( inLetter == '+' ) {
        handle = yzplus_big;
        indexOffset = 2;
        imageHeight = 128.0f;
        }
    else {
        inLetter = (char)tolower( inLetter );
        
        if( inLetter >= 'a' && inLetter <= 'z' ) {
        
            if( inLetter <= 'l' ) {
                handle = abcdefghijkl_big;
                indexOffset = inLetter - 'a';
                }
            else if( inLetter <= 'x' ) {
                handle = mnopqrstuvwx_big;
                indexOffset = inLetter - 'm';
                }
            else {
                handle = yzplus_big;
                indexOffset = inLetter - 'y';
                imageHeight = 128.0f;
                }
            }
        else {
            printf( "Error:  letter out of 'a-z' or '+' range: %c\n",
                    inLetter );
            }
        }
    
    

    drawSprite( handle,
                inCenterX, inCenterY,
                16, 20,
                inColor, inAlpha,
                indexOffset * 40 / imageHeight,
                40 / imageHeight );
    }








void drawScorePip( int inScore,
                   float inCenterX, float inCenterY,
                   Color *inColor, float inAlpha ) {
    
    int numDigits = 1;

    if( inScore > 0 ) {
        numDigits = (int)( floor( log10( inScore ) ) ) + 1;
        }
    

    // draw up to 9 digits

    if( numDigits > 9 ) {
        printf( "Error:  more than 9 digits passed to drawScorePip: %d\n",
                inScore );
        
        numDigits = 9;
        }
    
    int rowSep = 2;
    int colSep = 3;
    
    int centerOffsetsX[9] = { 
        6 + colSep,
        0,
        -( 6 + colSep ),
        6 + colSep,
        0,
        -( 6 + colSep ),
        6 + colSep,
        0,
        -( 6 + colSep ) 
        };
    
    int centerOffsetsY[9] = {
        10 + rowSep,
        10 + rowSep,
        10 + rowSep,
        0,
        0,
        0,
        -( 10 + rowSep ),
        -( 10 + rowSep ),
        -( 10 + rowSep ) 
        };
    
    // find center of mass for all digits together

    int xSum = 0;
    int ySum = 0;
    
    int i;
    
    for( i=0; i<numDigits; i++ ) {
        xSum += centerOffsetsX[ i ];
        ySum += centerOffsetsY[ i ];
        }
    
    int cX = xSum / numDigits;
    int cY = ySum / numDigits;


    if( numDigits > 3 ) {
        // center of mass doesn't look good for more than one row

        cX = 0;
        
        if( numDigits < 7 ) {
            cY = ( 10 + rowSep ) / 2;
            }
        else {
            cY = 0;
            }
        }
    
    
    
    for( i=0; i<numDigits; i++ ) {
        
        int number = ( inScore / ( (int)( pow( 10, i ) ) ) ) % 10;
        

        drawNumeral( number,
                     inCenterX - cX + centerOffsetsX[i], 
                     inCenterY - cY + centerOffsetsY[i],
                     inColor, inAlpha );
        }

    // commas
    if( numDigits > 3 ) {
        drawNumeral( 10,
                     inCenterX - cX + centerOffsetsX[3] + 3 + colSep,
                     inCenterY - cY + centerOffsetsY[3] + 1,
                     inColor, inAlpha );
        }
    if( numDigits > 6 ) {
        drawNumeral( 10,
                     inCenterX - cX + centerOffsetsX[6] + 3 + colSep,
                     inCenterY - cY + centerOffsetsY[6] + 1,
                     inColor, inAlpha );
        }
    
    }




void drawScore( int inScore,
                float inRightX, float inBottomY,
                Color *inColor, float inAlpha ) {
    
    
    int numDigits = 1;

    if( inScore > 0 ) {
        numDigits = (int)( floor( log10( inScore ) ) ) + 1;
        }
    
    
    int rowSep = 2;
    int colSep = 3;
        
    
    
    // next digit center
    int startCX = (int)inRightX - 3;
    int cX = startCX;
    int cY = (int)inBottomY - 5;
    
    int i;
    
    for( i=0; i<numDigits; i++ ) {
        
        int number = ( inScore / ( (int)( pow( 10, i ) ) ) ) % 10;
        
        if( i % 3 == 0 && i != 0 ) {
            // make room for comma
            if( i % 12 != 0 ) {
                // extra space around comma
                cX -= colSep - 1;
                }
            }
        
        drawNumeral( number,
                     cX, 
                     cY,
                     inColor, inAlpha );

        if( i % 3 == 0 && i != 0 ) {
            // comma after every 4th digit (between 3 and 4 )
                    
            drawNumeral( 10,
                         cX + 3 + colSep,
                         cY + 1,
                         inColor, inAlpha );
            }
        

        // compute next numeral center, twelve digits per line
        if( (i + 1) % 12 != 0 ) {
            cX -= 6 + colSep;
            }
        else {
            cX = startCX;
            cY -= 10 + rowSep;
            }
        }
    
    }







void drawScoreBig( int inScore,
                   float inRightX, float inBottomY,
                   Color *inColor, float inAlpha ) {
    
    
    int numDigits = 1;

    if( inScore > 0 ) {
        numDigits = (int)( floor( log10( inScore ) ) ) + 1;
        }
    
    
    int rowSep = 4;
    int colSep = 6;
        
    
    
    // next digit center
    int startCX = (int)inRightX - 6;
    int cX = startCX;
    int cY = (int)inBottomY - 10;
    
    int i;
    
    for( i=0; i<numDigits; i++ ) {
        
        int number = ( inScore / ( (int)( pow( 10, i ) ) ) ) % 10;
        
        if( i % 3 == 0 && i != 0 ) {
            // make room for comma
            if( i % 6 != 0 ) {
                // extra space around comma
                cX -= colSep - 2;
                }
            }
        
        drawNumeralBig( number,
                        cX, 
                        cY,
                        inColor, inAlpha );

        if( i % 3 == 0 && i != 0 ) {
            // comma after every 4th digit (between 3 and 4 )
                    
            drawNumeralBig( 10,
                            cX + 6 + colSep,
                            cY + 2,
                            inColor, inAlpha );
            }
        

        // compute next numeral center, six digits per line
        if( (i + 1) % 6 != 0 ) {
            cX -= 12 + colSep;
            }
        else {
            cX = startCX;
            cY -= 20 + rowSep;
            }
        }
    
    }




// internal 
void drawCounter( int inColSep, int inNumeralWidth,
                 void (*inNumeralDrawingFunction)( 
                     int, float, float,
                     Color *, float ), 
                  int inCount,
                  float inCenterX, float inCenterY,
                  Color *inColor, float inAlpha ) {
    
    int numDigits = 1;

    if( inCount > 0 ) {
        numDigits = (int)( floor( log10( inCount ) ) ) + 1;
        }
    

    // draw up to 2 digits

    if( numDigits > 2 ) {
        printf( "Error:  more than 2 digits passed to drawCounter: %d\n",
                inCount );
        
        numDigits = 2;
        }
    
    int numeralRadius = inNumeralWidth / 2;
    
    int centerOffsetsX[2] = { 
        numeralRadius,
        -( numeralRadius + inColSep ) };
        
    // find center of mass for all digits together

    int xSum = 0;
    
    int i;
    
    for( i=0; i<numDigits; i++ ) {
        xSum += centerOffsetsX[ i ];
        }
    
    int cX = xSum / numDigits;    
    
    
    for( i=0; i<numDigits; i++ ) {
        
        int number = ( inCount / ( (int)( pow( 10, i ) ) ) ) % 10;
        

        (*inNumeralDrawingFunction)( number,
                                     inCenterX - cX + centerOffsetsX[i], 
                                     inCenterY,
                                     inColor, inAlpha );
        }
    
    }



void drawCounter( int inCount,
                  float inCenterX, float inCenterY,
                  Color *inColor, float inAlpha ) {
    
    drawCounter( 3, 6, drawNumeral,
                 inCount,
                 inCenterX, inCenterY,
                 inColor, inAlpha );
    }



void drawCounterBig( int inCount,
                  float inCenterX, float inCenterY,
                  Color *inColor, float inAlpha ) {
    
    drawCounter( 6, 12, drawNumeralBig,
                 inCount,
                 inCenterX, inCenterY,
                 inColor, inAlpha );
    }



// internal
void drawString( int inColSep, int inLetterW,
                 void (*inLetterDrawingFunction)( 
                     char ,float, float,
                     Color *, float ), 
                 char *inString,
                 StringAlign inAlign,
                 float inX, float inY,
                 Color *inColor, float inAlpha ) {
    
    int numChars = strlen( inString );
    

    int colSep = inColSep;
    int letterW = inLetterW;

    int stringW = numChars * letterW + (numChars-1) * colSep;
    

    
    int startXC = (int)inX;
    
    switch( inAlign ) {
        case left:
            // no change
            break;
        case center:
            startXC -= stringW / 2;
            break;
        case right:
            startXC -= stringW;
            break;
        }
    
    startXC += letterW / 2;
    


    int currentXC = startXC;
    

    for( int i=0; i<numChars; i++ ) {
        
        char c = inString[i];
        
        // skip drawing anything for space
        if( c != ' ' ) {
            (*inLetterDrawingFunction)( c, currentXC, inY, inColor, inAlpha );
            }
        

        // advance
        currentXC += colSep + letterW;
        }
    }



void drawString( char *inString,
                 StringAlign inAlign,
                 float inX, float inY,
                 Color *inColor, float inAlpha ) {
    
    drawString( 3, 6,
                &drawLetter,
                inString,
                inAlign,
                inX, inY,
                inColor, inAlpha );
    }


void drawStringBig( char *inString,
                    StringAlign inAlign,
                    float inX, float inY,
                    Color *inColor, float inAlpha ) {
    
    drawString( 6, 12,
                &drawLetterBig,
                inString,
                inAlign,
                inX, inY,
                inColor, inAlpha );
    }
