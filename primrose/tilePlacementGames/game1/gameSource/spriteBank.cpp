#include "spriteBank.h"

#include "common.h"


#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/graphics/openGL/SingleTextureGL.h"


const char *spriteNames[ numSprites ] = 
{ 
    "gridLineTop",
    "gridLineBottom",
    "gridLineLeft",
    "gridLineRight",
    "plus",
    "piece",
    "pieceHalo",
    "pieceCenter",
    "pieceBrightHalo",
    "pieceBrightCenter",
    "numerals",
    "numeralsBig",
    "abcdefghijkl",
    "mnopqrstuvwx",
    "yzplus",
    "abcdefghijkl_big",
    "mnopqrstuvwx_big",
    "yzplus_big"
    };



SingleTextureGL *spriteTextures[ numSprites ];


void initSpriteBank() {
    for( int i=0; i<numSprites; i++ ) {
        
        char *fileName = autoSprintf( "%s.tga", spriteNames[ i ] );
        
        Image *image = readTGA( fileName );
        
        
        
        if( image != NULL ) {
                
            spriteTextures[ i ] = new SingleTextureGL( image, true );
            
            delete image;
            }
        else {
            spriteTextures[i] = NULL;
            }

        delete [] fileName;

        }
    }



void freeSpriteBank() {
    for( int i=0; i<numSprites; i++ ) {
        if( spriteTextures[i] != NULL ) {
            delete spriteTextures[i];
            }
        }
    }



// global
extern int spriteDrawCount;



void drawSprite( SpriteHandle inSpriteHandle,
                 float inCenterX, float inCenterY, 
                 float inXRadius, float inYRadius,
                 Color *inColor, float inAlpha,
                 float inSubsectionOffset,
                 float inSubsectionExtent ) {
    

    SingleTextureGL *texture = spriteTextures[ inSpriteHandle ];

    if( texture == NULL ) {
        return;
        }
    
    
    
    
    
    
    if( inColor != NULL  ) {
        glColor4f( inColor->r, inColor->g, inColor->b, inAlpha );
        }
    else {
        glColor4f( 1, 1, 1, inAlpha );
        }

    
    

    texture->enable(); 


    // NOTE:  this won't look good if we do the zoom-to-clear effect
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST ); 
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );


    const GLfloat squareVertices[] = {
        inCenterX - inXRadius, inCenterY - inYRadius,
        inCenterX + inXRadius, inCenterY - inYRadius,
        inCenterX - inXRadius, inCenterY + inYRadius,
        inCenterX + inXRadius, inCenterY + inYRadius,
        };
    /*
    const GLubyte squareColors[] = {
        255, 255,   0, 255,
        0,   255, 255, 255,
        255,     0,   0,   0,
        255,   0, 255, 255,
    };
     */
    const GLfloat squareTextureCoords[] = {
        0, inSubsectionOffset,
        1, inSubsectionOffset,
        0, inSubsectionOffset + inSubsectionExtent,
        1, inSubsectionOffset + inSubsectionExtent
    };

    

    glVertexPointer( 2, GL_FLOAT, 0, squareVertices );
    glEnableClientState( GL_VERTEX_ARRAY );
    
    //glColorPointer( 4, GL_FLOAT, 0, squareColors );
    //glEnableClientState( GL_COLOR_ARRAY );
    
    glTexCoordPointer( 2, GL_FLOAT, 0, squareTextureCoords );
    glEnableClientState( GL_TEXTURE_COORD_ARRAY );
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    /*

    glBegin( GL_QUADS ); {
    
        glTexCoord2f( 0, inSubsectionOffset );
        glVertex2f( inCenterX - inXRadius, inCenterY - inYRadius );
        
        glTexCoord2f( 1, inSubsectionOffset );
        glVertex2f( inCenterX + inXRadius, inCenterY - inYRadius );
        
        glTexCoord2f( 1, inSubsectionOffset + inSubsectionExtent );
        glVertex2f( inCenterX + inXRadius, inCenterY + inYRadius );
        
        glTexCoord2f( 0, inSubsectionOffset + inSubsectionExtent );
        glVertex2f( inCenterX - inXRadius, inCenterY + inYRadius );        
        }
    glEnd();
    */
    texture->disable();

    spriteDrawCount ++;

    }

