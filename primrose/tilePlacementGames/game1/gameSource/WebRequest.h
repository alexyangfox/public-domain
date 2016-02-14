#ifndef WEB_REQUEST_INCLUDED
#define WEB_REQUEST_INCLUDED


#include "minorGems/network/Socket.h"
#include "minorGems/network/HostAddress.h"
#include "minorGems/network/LookupThread.h"


#include "BringNetworkUpThread.h"



// a non-blocking web request
class WebRequest {
        

    public:
        
        // inMethod = GET, POST, etc.
        // inURL the url to retrieve
        // inBody the body of the request, can be NULL
        // request body must be in application/x-www-form-urlencoded format
        WebRequest( char *inMethod, char *inURL,
                    char *inBody );
        

        // if request is not complete, destruction cancels it
        ~WebRequest();


        // take anoter non-blocking step
        // return 1 if request complete
        // return -1 if request hit an error
        // return 0 if request still in-progress
        int step();
        

        // gets the response body as a \0-terminated string
        char *getResult();
        

    protected:
        char mError;
        
        char *mURL;

        char *mRequest;
        
        int mRequestPosition;
        
        SimpleVector<char> mResponse;
        
        char mResultReady;
        
        char *mResult;
        

        HostAddress *mSuppliedAddress;
        HostAddress *mNumericalAddress;
        BringNetworkUpThread *mNetworkUpThread;
        LookupThread *mLookupThread;
        
        Socket *mSock;
        
    };



#endif
