// iPhone network goes down after a period of inactivity

char isNetworkAlwaysOn() {
    // for testing
    return false;
    }


#import <SystemConfiguration/SCNetworkReachability.h>

#include <netinet/in.h>


BOOL connectedToNetwork() {
    // Create zero addy
    struct sockaddr_in zeroAddress;
    bzero( &zeroAddress, sizeof( zeroAddress ) );
    zeroAddress.sin_len = sizeof( zeroAddress );
    zeroAddress.sin_family = AF_INET;
	
    // Recover reachability flags
    SCNetworkReachabilityRef defaultRouteReachability = 
        SCNetworkReachabilityCreateWithAddress( 
            NULL, 
            (struct sockaddr *)&zeroAddress);
    SCNetworkReachabilityFlags flags;
	
    BOOL didRetrieveFlags = 
        SCNetworkReachabilityGetFlags( defaultRouteReachability, &flags );
    CFRelease( defaultRouteReachability );
	
    if( !didRetrieveFlags ) {
        printf( "Error. Could not recover network reachability flags\n" );
        return 0;
        }
	
    BOOL isReachable = flags & kSCNetworkFlagsReachable;

    BOOL needsConnection = flags & kSCNetworkFlagsConnectionRequired;

    return ( isReachable && !needsConnection ) ? YES : NO;
    }



// does nothing
char makeSureNetworkIsUp( char *inTestURL ) {
    
    // create a new pool here in case we are called from a new thread
	NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    
    
	if( connectedToNetwork() ) {
        printf( "Network is up.\n" );
        
        [pool release];
        
        return true;
        }
    else {
        printf( "Network not up... trying to bring it up\n" );
        
        // try to force the network connection to come up
        //char *testURL = "http://www.google.com";
    
        NSString *urlString = [ NSString stringWithCString:inTestURL 
                                encoding:NSASCIIStringEncoding ];
    
        NSURL *url = [NSURL URLWithString:urlString];
        NSError *error;
        
        NSString *source = [ NSString stringWithContentsOfURL:url
                             encoding:NSASCIIStringEncoding error:&error];
        
        if( source == nil ) {
            [pool release];
            
            return false;
            }
        else {
            
            const char *sourceString = 
                [ source cStringUsingEncoding:NSASCIIStringEncoding ];
        
            printf( "Fetch the following from %s when bringing up"
                    " network connection:\n%s\n", inTestURL, sourceString );
        
            [pool release];
            
            return true;
            }
        
        /*
        // try to force the network connection to come up
        NSURL *OTURL = [NSURL URLWithString:@"http://www.google.com"];
        NSError *error;
        
        NSString *source = [ NSString stringWithContentsOfURL:OTURL
                             encoding:NSASCIIStringEncoding error:&error];
        
        const char *sourceString = 
            [ source cStringUsingEncoding:NSASCIIStringEncoding ];

        printf( "%s,\n", sourceString );
        */
        }
    

    }

