#include <pth.h>
// #warning "This code does not work yet!"
namespace s11n {

	static int placeholder = pth_init();

	/**
	   Each instance of this class contains its own critical
	   section/mutex. You can simulate a global mutex by
	   sharing an instance of this class.
	*/
        class mutex_gnupth
        {
        public:
		/** Initializes the mutex. */
                mutex_gnupth();
		/** Initializes the mutex. The argument is ignored.
		*/
                mutex_gnupth(const mutex_gnupth&);
		/** Destroys the mutex. */
                ~mutex_gnupth() throw();
		/**
		   Locks the mutex using gnupth_mutex_lock().
		 */
                void lock();
		/**
		   Unlocks the mutex using gnupth_mutex_unlock().
		 */
                void unlock();
        private:
                pth_mutex_t m_mutex;
        };

        /**
           Maintains a mutex local to this object.
         */
	mutex_gnupth::mutex_gnupth()
		: m_mutex()
	{
		//m_mutex = PTH_MUTEX_INIT;
		int rc = pth_mutex_init(&m_mutex);
		MUTEX_TRACE << "mutex_gnupth() init == "<<rc<<"\n";
	}

	mutex_gnupth::mutex_gnupth(const mutex_gnupth&)
		: m_mutex()
	{
		//m_mutex = PTH_MUTEX_INIT;
		int rc = pth_mutex_init(&m_mutex);
		MUTEX_TRACE << "mutex_gnupth() init == "<<rc<<"\n";
	}

        mutex_gnupth::~mutex_gnupth() throw()
	{
		// gnupth_mutex_destroy(&m_mutex);
		// How the hell to destroy a mutex in Pth???
	}

        void mutex_gnupth::lock()
	{
		MUTEX_TRACE << "mutex_gnupth::lock()\n";
		pth_mutex_acquire(&m_mutex,FALSE,NULL);
	}

	void mutex_gnupth::unlock()
	{
		MUTEX_TRACE << "mutex_gnupth::unlock()\n";
		pth_mutex_release(&m_mutex);
	}


} // namespace
