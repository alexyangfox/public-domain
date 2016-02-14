/*
 * Modification History
 *
 * 2001-December-14     Jason Rohrer
 * Created.
 * Fixed a typo.
 * Added a missing include.
 *
 * 2002-March-25    Jason Rohrer
 * Added function for conversion to numerical addresses.
 *
 * 2002-March-26    Jason Rohrer
 * Changed to use strdup.
 * Fixed problem on some platforms where local address does
 * not include domain name.
 * Fixed so that numerical address is actually retrieved by
 * getNumericalAddress().
 *
 * 2002-April-6    Jason Rohrer
 * Replaced use of strdup.
 *
 * 2002-April-8    Jason Rohrer
 * Changed to return numerical address from getLocalAddress,
 * since it is simpler, and we probably want the numerical
 * address anyway.
 * This new implementation also fixes a BSD domain-name lookup bug.
 * Changed to check if address is numerical before
 * performing a lookup on it.
 *
 * 2002-July-10    Jason Rohrer
 * Fixed a bug that occurs when fetching numerical address fails.
 *
 * 2004-January-4   Jason Rohrer
 * Added use of network function locks.
 */



#include "minorGems/network/HostAddress.h"
#include "minorGems/network/NetworkFunctionLocks.h"
#include "minorGems/util/stringUtils.h"

#include <unistd.h>
#include <string.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>


HostAddress *HostAddress::getLocalAddress() {
	int bufferLength = 200;
	char *buffer = new char[ bufferLength ];

	gethostname( buffer, bufferLength );

    
    char *name = stringDuplicate( buffer );

	delete [] buffer;


    // this class will destroy name for us
    HostAddress *nameAddress = new HostAddress( name, 0 );

    HostAddress *fullAddress = nameAddress->getNumericalAddress();

    if( fullAddress != NULL ) {
    
        delete nameAddress;

        return fullAddress;
        }
    else {
        return nameAddress;
        }
    
    }



HostAddress *HostAddress::getNumericalAddress() {

    // make sure we're not already numerical
    if( this->isNumerical() ) {
        return this->copy();
        }


    // need to use two function locks here

    // first, lock for gethostbyname
    NetworkFunctionLocks::mGetHostByNameLock.lock();
    struct hostent *host = gethostbyname( mAddressString );

    if (host != NULL) {

        // this line adapted from the
        // Unix Socket FAQ
        struct in_addr *inetAddress = (struct in_addr *) *host->h_addr_list;


        // now lock for inet_ntoa
        NetworkFunctionLocks::mInet_ntoaLock.lock();
        // the returned string is statically allocated, so copy it
        char *inetAddressString = stringDuplicate( inet_ntoa( *inetAddress ) );

        NetworkFunctionLocks::mInet_ntoaLock.unlock();

        
        // done with returned hostent now, so unlock first lock
        NetworkFunctionLocks::mGetHostByNameLock.unlock();
        
        return new HostAddress( inetAddressString, mPort );
        }
    else {

        // unlock first lock before returning
        NetworkFunctionLocks::mGetHostByNameLock.unlock();
        
        return NULL;
        }

    }



char HostAddress::isNumerical() {
    
    struct in_addr addressStruct;

    int isValid = inet_aton( mAddressString, &addressStruct );


    if( isValid == 0 ) {
        return false;
        }
    else {
        return true;
        }

    }



