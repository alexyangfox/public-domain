#ifndef BRING_NETWORK_UP_THREAD_CLASS_INCLUDED
#define BRING_NETWORK_UP_THREAD_CLASS_INCLUDED

#include "minorGems/system/FinishedSignalThread.h"
#include "minorGems/system/MutexLock.h"



/**
 * Thread that performs DNS lookup on a host name.
 *
 * @author Jason Rohrer
 */
class BringNetworkUpThread : public FinishedSignalThread {
	
	public:
		/**
         * Constructs and starts a thread.
         *
		 * @param inURL the URL to try reaching.  Destroyed by caller.
		 */
		BringNetworkUpThread( char *inURL );
        
        
        // joins and destroys this thread
        ~BringNetworkUpThread();
        

		

        /**
         * Returns true if network up attempt is done.
         */
        char isNetworkCheckDone();




        /**
         * Called after isNetworkCheckDone returns true.
         *
         * Returns false if network failed to come up.
         */
        char isNetworkUp();



		// override the run method from Thread
		void run();
	
	private:
        MutexLock mLock;
        
        char *mURL;
        
        char mNetworkCheckDone;
                
        char mResult;
        
        
	};

	
#endif
