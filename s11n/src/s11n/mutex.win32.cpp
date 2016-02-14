
#if MUTEX_WIN32_ADD_BOGUS_IMPLEMENTATION
// install some stub functions so we can just COMPILE this code
// on Linux, to make sure i don't release un-compileable code
// to the Windows users...
typedef int CRITICAL_SECTION;
void InitializeCriticalSection( int * ) throw() { return; }
void DeleteCriticalSection( int * ) throw() { return; }
void EnterCriticalSection( int * ) throw() { return; }
void LeaveCriticalSection( int * ) throw() { return; }
#else
// We're really on Windows, so...
#include <windows.h>
#endif // MUTEX_WIN32_ADD_BOGUS_IMPLEMENTATION

namespace s11n {

	/**
	   mutex_win32:

	   Your program is assumed to be multi threaded and each
	   instance of this class contains its own critical
	   section/mutex. In practice, this means that mutex
	   collisions (and hence context switches) only happen if they
	   are absolutely essential. However, on some platforms,
	   creating a lot of mutexes can slow down the whole OS, so
	   use this option with care. You can simulate a global mutex
	   by sharing an instance of this class.
	*/
        class mutex_win32
        {
        public:
		/** Initializes a critical section. */
                mutex_win32();
		/**
		   Initializes a critical section. The
		   argument is ignored.
		*/
                mutex_win32(const mutex_win32&);
		/**
		   Closes this object's critical section.
		*/
                ~mutex_win32() throw();
		/** Locks the critical section with
		    EnterCriticalSection().
		*/
                void lock();
		/** Unlocks the critical section using
		    LeaveCriticalSection().
		*/
                void unlock();
        private:
                CRITICAL_SECTION m_critsec;
        };


	mutex_win32::mutex_win32()
	{
		InitializeCriticalSection(&m_critsec);
	}

	mutex_win32::mutex_win32(const mutex_win32&)
	{
		InitializeCriticalSection(&m_critsec);
	}

	mutex_win32::~mutex_win32() throw()
	{
		DeleteCriticalSection(&m_critsec);
	}

        void mutex_win32::lock()
	{
		MUTEX_TRACE << "mutex_win32::lock()\n";
		EnterCriticalSection(&m_critsec);
	}

        void mutex_win32::unlock()
	{
		MUTEX_TRACE << "mutex_win32::unlock()\n";
		LeaveCriticalSection(&m_critsec);
	}


} // namespace
