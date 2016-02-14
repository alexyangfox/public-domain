/*
 * Modification History
 *
 * 2003-September-22   Jason Rohrer
 * Created.
 */


#include "encodingUtils.h"

#include <stdio.h>
#include <string.h>



int main() {

    const char *dataString =
        "*#$&$(@KFI#*$(SDBM@#*!(@%"
        "*#$&$(@KFI#*$(SDBM@#*!(@%"
        "*#$&$(@KFI#*$(SDBM@#*!(@F"
        "*#$&$(@KFI#*$(SDBM@#*!(@F"
        "*#$&$(@KFI#*$(SDBM@#*!(@F"
        "*#$&$(@KFI#*$(SDBM@#*!(@%a";

    printf( "base64 encoding the string:  %s\n", dataString );

    char *encoding = base64Encode( (unsigned char *)dataString,
                                   strlen( dataString ),
                                   true );
    
    printf( "Encoded as:\n%s\n", encoding );


    int decodedLength;
    unsigned char *decoding = base64Decode( encoding, &decodedLength );

    char *buffer = new char[ decodedLength + 1 ];
    memcpy( (void *)buffer, (void *)decoding, decodedLength );

    buffer[ decodedLength ] = '\0';
    
    printf( "Decoded as: %s\n", buffer );

    if( strcmp( buffer, dataString ) == 0 ) {
        printf( "Test passed\n" );
        }
    else {
        printf( "Test failed\n" );
        }
    
    delete [] buffer;
    delete [] decoding;
    delete [] encoding;

    return 0;
    }
