#include "ScoreBundle.h"

#include "minorGems/util/stringUtils.h"

#include <string.h>



ScoreBundle::ScoreBundle( char *inName, 
                          unsigned int inScore, unsigned int inSeed,
                          char *inMoveHistory )
    : mScore( inScore ), mSeed( inSeed ), 
      mMoveHistory( stringDuplicate( inMoveHistory ) ),
      mNumMoves( strlen( inMoveHistory ) ) {
    

    int nameLength = strlen( inName );

    if( nameLength > 8 ) {
        nameLength = 8;
        }
    
    memcpy( mName, inName, nameLength );
    mName[ nameLength ] = '\0';
    }



ScoreBundle::ScoreBundle( char *inEncoded )
        : mScore( 0 ), mSeed( 0 ), mMoveHistory( NULL ),
          mNumMoves( 0 ) {
    
    mName[0] = '\0';
    

    int numParts;
    
    char **parts = split( inEncoded, "#", &numParts );
    

    if( numParts == 4 ) {
        
        char *name = parts[0];
        

        int nameLength = strlen( name );
        
        if( nameLength > 8 ) {
            nameLength = 8;
            }
        
        memcpy( mName, name, nameLength );
        mName[ nameLength ] = '\0';
        
        
        int numRead = sscanf( parts[1],
                              "%u", &mScore );
        if( numRead != 1 ) {
            printf( "Failed to decode score bundle: %s\n", inEncoded );
            }
        
        numRead = sscanf( parts[2],
                          "%u", &mSeed );
        
        if( numRead != 1 ) {
            printf( "Failed to decode score bundle: %s\n", inEncoded );
            }
        

        mMoveHistory = stringDuplicate( parts[3] );
        mNumMoves = strlen( mMoveHistory );
        }
    else {
        printf( "Failed to decode score bundle: %s\n", inEncoded );
        }
    
    

    for( int i=0; i<numParts; i++ ) {
        delete [] parts[i];
        }
    delete [] parts;
    }

    
        
ScoreBundle::~ScoreBundle() {
    if( mMoveHistory != NULL ) {
        delete [] mMoveHistory;
        }
    }



ScoreBundle *ScoreBundle::copy() {
    return new ScoreBundle( mName, mScore, mSeed, mMoveHistory );
    }

