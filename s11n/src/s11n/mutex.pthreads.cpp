#include <pthread.h>

namespace s11n {

	/**
	   Each instance of this class contains its own critical
	   section/mutex. However, on some platforms, creating a lot
	   of mutexes can slow down the whole OS, so use this option
	   with care. You can simulat a global mutex by sharing an
	   instance of this class.
	*/
        class mutex_pthread
        {
        public:
		/** Initializes the mutex. */
                mutex_pthread();
		/** Initializes the mutex. The argument is ignored.
		*/
                mutex_pthread(const mutex_pthread&);
		/** Destroys the mutex. */
                ~mutex_pthread() throw();
		/**
		   Locks the mutex using pthread_mutex_lock().
		 */
                void lock();
		/**
		   Unlocks the mutex using pthread_mutex_unlock().
		 */
                void unlock();
		static pthread_mutex_t * shared();
        private:
                pthread_mutex_t m_mutex;
        };


	mutex_pthread::mutex_pthread()
	{
		pthread_mutex_init(&m_mutex, NULL);
	}

	mutex_pthread::mutex_pthread(const mutex_pthread&)
	{
		pthread_mutex_init(&m_mutex, NULL);
	}

        mutex_pthread::~mutex_pthread() throw()
	{
		pthread_mutex_destroy(&m_mutex);
	}

        void mutex_pthread::lock()
	{
		MUTEX_TRACE << "mutex_pthread::lock()\n";
		pthread_mutex_lock(&m_mutex);
	}

	void mutex_pthread::unlock()
	{
		MUTEX_TRACE << "mutex_pthread::unlock()\n";
		pthread_mutex_unlock(&m_mutex);
	}

} // namespace
