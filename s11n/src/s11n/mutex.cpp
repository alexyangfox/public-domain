#include <s11n.net/s11n/mutex.hpp>
#include <s11n.net/s11n/s11n_config.hpp> // s11n_CONFIG_SINGLE_THREADED, s11n_CONFIG_HAVE_xxxx

/**

Thread locking classes

Code originally written by Sarah Thompson (sarah at telergy com) in 2002,
taken and adapted somewhat by stephan beal (stephan at s11n net) in 2004.

License: Public Domain. You are free to use this code however you
like, with the proviso that the author takes on no responsibility or
liability for any use.

========================================================================
Hacked and re-released under the s11n.net project by stephan beal
(stephan@s11n.net) September, 2004. That is: the features are Sarah's,
the bugs are mine.

Changes by stephan:

1-4 September, 2004:

- brought typename usage up to date with gcc 3.

18 Sept 2004:

- Significantly changed the way the threading macros
  work.

19 Sept 2004:

- Documentation cleanups.

29 May 2006:

- When pthreads/win32 threads are not used, the various
  win32/pthread mutexes are typedefs which equal mutex_noop.
  This allows code to hard-code win32/pthreads mutexes even if they
  don't/won't use them.

2007.02.24:

- stripped out the thread locking code from the core sigslot library
for inclusion into libs11n.

- improved pthread_global implementation.

- Created GNU Pth-based implementation. Segfaults, for reasons i don't
understand.

2007.02.26:

- Significantly cleaned up the internals of the mutex_PLATFORM
implementations.



========================================================================

QUICK DOCUMENTATION 
       
THREADING MODES:

mutex_noop:

Your program is assumed to be single threaded, and this type is just
a placeholder which does nothing.


mutex_win32 and mutex_pthread:

These implement per-instance mutexes. In practice, this means that
mutex collisions (and hence context switches) only happen if they are
absolutely essential. However, on some platforms, creating a lot of
mutexes can slow down the whole OS, so use this option with care.

The following typedefs get defined so that client code can be
written to be ignorant of the mutex types:

  typedef SOMETHING mutex_impl;

The mutex_impl type will be a type which holds a local mutex (or no
mutex, in the case of mutex_noop policy).
*/

#if s11n_CONFIG_SINGLE_THREADED
#  define S11N_MUTEX_IMPLEMENTATION_NAME "single-threaded"
#elif s11n_CONFIG_HAVE_PTHREADS
#  include "mutex.pthreads.cpp"
#  define S11N_MUTEX_USE_POSIX_THREADS 1
#  define S11N_MUTEX_IMPLEMENTATION_NAME "pthreads"
#elif s11n_CONFIG_HAVE_GNUPTH_THREADS
#  include "mutex.gnupth.cpp"
#  define S11N_MUTEX_USE_GNUPTH_THREADS 1
#  define S11N_MUTEX_IMPLEMENTATION_NAME "gnupth"
#elif defined(WIN32)
//#  define MUTEX_WIN32_ADD_BOGUS_IMPLEMENTATION 1
// set MUTEX_WIN32_ADD_BOGUS_IMPLEMENTATION to 1 and force the compiler
// to include mutex.win32.cpp if you want to try to compile the code
// under Linux. See mutex.win32.cpp for info.
#  include "mutex.win32.cpp"
#  define S11N_MUTEX_USE_WIN32_THREADS 1
#  define S11N_MUTEX_IMPLEMENTATION_NAME "Win32"
#else ///////////////////// fall back to single-threaded:
#  define S11N_MUTEX_IMPLEMENTATION_NAME "single-threaded"
#endif


#ifndef S11N_MUTEX_USE_GNU_PTH_THREADS
#  define S11N_MUTEX_USE_GNU_PTH_THREADS 0
#endif
#ifndef S11N_MUTEX_USE_POSIX_THREADS
#  define S11N_MUTEX_USE_POSIX_THREADS 0
#endif
#ifndef S11N_MUTEX_USE_WIN32_THREADS
#  define S11N_MUTEX_USE_WIN32_THREADS 0
#endif


#include <s11n.net/s11n/s11n_debuggering_macros.hpp>
using namespace s11n::debug;
#define MUTEX_TRACE S11N_TRACE(TRACE_MUTEX) << "[mutex_impl @ "<<std::hex<<this << "]: "

/**
   If S11N_MUTEX_OPTIMIZE_SINGLE is true then some operations
   in the mutex class are not done. We *could* leave out this
   optimization, but there's no point in single-threaded
   builds having to create an extra object which they won't
   use.
*/
#define S11N_MUTEX_OPTIMIZE_SINGLE (s11n_CONFIG_SINGLE_THREADED)


#if 1 < (S11N_MUTEX_USE_WIN32_THREADS + S11N_MUTEX_USE_POSIX_THREADS + S11N_MUTEX_USE_GNU_PTH_THREADS)
#error "Somehow this file got confused into thinking it should use more than one threading implementation."
#endif


namespace s11n {

	/**
	   A no-op implementation of a thread locker.
	*/
        class mutex_noop
        {
        public:
		/** Does nothing. */
                mutex_noop();
		/** Does nothing. */
                mutex_noop( const mutex_noop & );
		/** Does nothing. */
                ~mutex_noop() throw();
		/** Does nothing. */
                void lock();
		/** Does nothing. */
		void unlock();
        };

#if S11N_MUTEX_USE_POSIX_THREADS
	typedef mutex_pthread mutex_impl;
#elif S11N_MUTEX_USE_GNUPTH_THREADS
	typedef mutex_gnupth mutex_impl;
#elif S11N_MUTEX_USE_WIN32_THREADS
	typedef mutex_win32 mutex_impl;
#else // single-threaded:
	typedef mutex_noop mutex_impl;
#endif


	/**
	   "A level of indirection" to hide the native mutex
	   implementation from down-stream code. mutex_impl
	   must conform to the API and conventions set out by
	   mutex_noop, mutex_win32, mutex_pthread, etc. In particular,
	   mutex_impl must have a no-throw destructor.
	*/
	struct mutex::impl
	{
		mutex_impl mx;
		impl() : mx() {}
		~impl() throw() {}
	};

	mutex::mutex()
		: m_p(
		      S11N_MUTEX_OPTIMIZE_SINGLE
		      ? 0
		      : new mutex::impl
		      )
	{
	}

	mutex & mutex::operator=( mutex const & )
	{
		// do nothing!
		return *this;
	}
	mutex::mutex( mutex const & )
		: m_p(
		      S11N_MUTEX_OPTIMIZE_SINGLE
		      ? 0
		      : new mutex::impl
		      )
	{
	}

	mutex & mutex::lock()
	{
		if( ! S11N_MUTEX_OPTIMIZE_SINGLE )
		{
			this->m_p->mx.lock();
		}
		return *this;
	}
	void mutex::unlock()
	{
		if( ! S11N_MUTEX_OPTIMIZE_SINGLE )
		{
			this->m_p->mx.unlock();
		}
	}

	mutex::~mutex() throw()
	{
		if( ! S11N_MUTEX_OPTIMIZE_SINGLE )
		{
			delete this->m_p;
		}
	}
	
	mutex_sentry::mutex_sentry( mutex & m )
		: mx(m.lock())
	{
	}

	mutex_sentry::~mutex_sentry() throw()
	{
		this->mx.unlock();
	}

	mutex_noop::mutex_noop()
	{
		;
	}

	mutex_noop::mutex_noop( const mutex_noop & )
	{
		;
	}

        mutex_noop::~mutex_noop() throw()
	{
		;
	}
	
	void mutex_noop::lock()
	{
		MUTEX_TRACE << "mutex_noop::lock()\n";
	}

	void mutex_noop::unlock()
	{
		MUTEX_TRACE << "mutex_noop::unlock()\n";
	}


#undef S11N_MUTEX_OPTIMIZE_SINGLE
#undef S11N_MUTEX_USE_POSIX_THREADS
#undef S11N_MUTEX_USE_GNU_PTH_THREADS
#undef S11N_MUTEX_USE_WIN32_THREADS
#undef S11N_MUTEX_IMPLEMENTATION_NAME
#undef MUTEX_TRACE

} // namespace
