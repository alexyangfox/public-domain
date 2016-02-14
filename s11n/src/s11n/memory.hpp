#ifndef S11N_NET_S11N_MEMORY_HPP_INCLUDED
#define S11N_NET_S11N_MEMORY_HPP_INCLUDED 1

#include <s11n.net/s11n/exception.hpp> // s11n_exception and friends

namespace s11n {

	/**
	   Calls s11n_traits<SerializableType>::cleanup_functor()(s).

	   This function is declared as no-throw because of its
	   logical role in the destruction process, and dtors are
	   normally prohibited from throwing. Any exceptions caught by
	   this function are silently ignored (a warning might go out
	   to a debug channel, probably cerr, but don't rely on it).

	   SerializableType requirements:

	   - Must be a Serializable. Specifically, it must have an
	   s11n_traits specialization installed.

	   - s11n_traits<SerializableType>::cleanup_functor must be
	   known to work properly for SerializableType. This is core
	   to the whole cleanup functionality, which is core to
	   protecting against leaks in the face of errors.

	   Technically, if the type can be delete()d without leaking
	   pointers, it's safe for use with this function, but this
	   function SHOULD NOT be used as general cleanup tool. It is
	   ONLY intended to be used with REGISTERED Serializables.

	   This function guarantees not to leak when "cleaning up"
	   containers holding unmanaged pointers as long as the
	   associated cleanup_functors do their part. The model is
	   such that once a cleanup_functor is in place for a given
	   type, this function will inherently walk it and invoke the
	   cleanup rules, which includes freeing any pointers along
	   the way.

	   Added in 1.1.3.
	*/
	template <typename SerializableType>
	void cleanup_serializable( SerializableType & s ) throw();

	/**
	   This overload provides cleanup handling for pointer
	   types. This simplifies many algorithms over using
	   s11n_traits<SerializableType>::cleanup_functor directly, as
	   the algorithms do not need to care if they're using
	   pointer-qualified types or not in order to clean them up
	   properly.

	   SerializableType requirements are as for the non-pointered
	   variant of this function, plus:

	   - delete aSerializableTypeInstance; must be well-formed and
	   must neither throw nor invoke undefined behaviour.  (Did
	   you realize that "neither" is an exception to English's
	   "i-before-e" rule?)

	   This function does nothing if s is null, otherwise it calls
	   cleanup_serializable(*s), deletes s, then assigns it to 0.

	   Postcondition: (0 == s)

	   Added in 1.1.3.
	*/
	template <typename SerializableType>
	void cleanup_serializable( SerializableType * & s ) throw();


        /**
	   Intended for use with for_each(), this type cleans up
	   Serializables using cleanup_serializable().

	   Usage:

	   std::for_each( container.begin(), container.end(), cleaner_upper() );

	   where the container is parameterized to hold Serializables.

	   Provided that the contained type(s) conform to
	   cleanup_ptr's requirements, this will recursively clean up
	   sub-sub-...subcontainers.

	   Note that Serializable containers should have a cleanup
	   functor installed as part of their registration, making
	   this class unnecessary for most cases: simply calling
	   cleanup_serializable() will recursively walk/clean such
	   containers. The underlying cleanup algos might use this
	   type, however (at least one of them does).

	   Added in 1.1.3.

	   This type is usable as a Finalizer for
	   s11n::refcount::rcptr, by the way.
	 */
        struct cleaner_upper
        {
                /**
                   Calls cleanup_serializable<T>(t)
                */
                template <typename T>
                void operator()( T & t ) throw()
                {
                        cleanup_serializable<T>( t );
                }
                /**
                   Calls cleanup_serializable<T>(t).
                */
                template <typename T>
                void operator()( T * & t ) throw()
                {
                        cleanup_serializable<T>( t );
                }
        };


	/**
	   An auto_ptr-like type intended to simplify
	   pointer/exception safety in some deserialization algorithms
	   by providing a way to completely and safely destroy
	   partially-deserialized objects.

	   SerializableT must either have an explicit s11n_traits
	   specialization installed or work properly with the default
	   functor provided by s11n_traits::cleanup_functor.  In
	   practice, this means that types which manage the memory of
	   their contained pointers are safe to work with the default,
	   whereas the cleanup of unmanaged child pointers (e.g., std
	   containers) requires a proper specialization.

	   Note that this type does not have copy/assignment ctors,
	   due to the conventional constness of their right-hand
	   sides: use the swap() or take() members to take over a
	   pointer.

	   Added in 1.1.3.


	   To consider for 1.3.x: reimplement this type on top of
	   rcptr so that we can get sane copy semantics.
	*/
	template <typename SerializableT>
	struct cleanup_ptr
	{
	public:
		typedef SerializableT cleaned_type;
	private:
		cleaned_type * m_ptr;
		cleanup_ptr & operator=( const cleanup_ptr & ); // Not Implemented
		cleanup_ptr( const cleanup_ptr & ); // Not Implemented
		void cleanup() throw()
		{
			if( this->m_ptr )
			{
				cleanup_serializable<cleaned_type>( this->m_ptr );
			}
		}
	public:
		/**
		   Constructs an object pointing to nothing.
		*/
		cleanup_ptr() throw() : m_ptr(0) 
		{
		}
		/**
		   Transfers ownership of p to this object.
		 */
		cleanup_ptr( cleaned_type * p ) throw() : m_ptr(p)
		{
		}

		/**
		   Uses s11n::cleanup_serializable<cleaned_type>()
		   to free up up this->get().
		*/
		~cleanup_ptr() throw()
		{
			this->cleanup();
		}
		/**
		   Dereferences this object's pointed-to object.  If
		   this object does not point to anything it throws a
		   std::runtime_error with an informative what()
		   message explaining the error.
		 */
		cleaned_type & operator*()
		{
			if( ! this->m_ptr )
			{
				throw s11n_exception( S11N_SOURCEINFO,
						      "Attempt to dereference a null pointer via s11n::cleanup_ptr<>::operator*()" );
			}
			return *this->m_ptr;
		}

		/**
		   Returns the same as get().
		*/
		cleaned_type * operator->() throw()
		{
			return this->m_ptr;
		}

		/**
		   Returns this object's pointed-to object without
		   transfering ownership.
		*/
		cleaned_type * get() throw()
		{
			return this->m_ptr;
		}

		/**
		   Transfers ownership of p to this object. This
		   member takes the place of copy/assign operators,
		   since those conventionally take a const right-hand
		   argument.

		   Destroys the object this object pointed to before
		   taking over ownership. 0 is a legal value for p.

		   If (p == this->get()) then this function does
		   nothing.

		   Postcondition: p == this->get() 
		*/
		void take( cleaned_type * p ) throw()
		{
			if( p != this->m_ptr )
			{
				this->cleanup();
				this->m_ptr = p;
			}
		}

		/**
		   Transfers ownership of this->get() to the
		   caller.

		   Postcondition: 0 == this->get() 
		*/
		cleaned_type * release() throw()
		{
			cleaned_type * x = this->m_ptr;
			this->m_ptr = 0;
			return x;
		}

		/**
		   Cleans up any pointed-to object and points this
		   object at 0. Does nothing if this object points
		   to no object.

		   Postcondition: 0 == this->get() 
		*/
		void clean() throw()
		{
			this->take( 0 );
		}

		/**
		   Swaps ownership of pointers with rhs.
		*/
		void swap( cleanup_ptr & rhs ) throw()
		{
			cleaned_type * x = this->m_ptr;
			this->m_ptr = rhs.m_ptr;
			rhs.m_ptr = x;
		}

		/**
		   Returns the same as (!this->get()).
		*/
		bool empty() const throw()
		{
			return 0 == this->m_ptr;
		}
	};

    namespace Detail {
	/**
	   We use a custom auto_ptr<> work-alike in place of
	   std::auto_ptr so that s11n compiles cleanly under C++0x,
	   where auto_ptr is apparently deprecated.

	   This type is a subset of std::auto_ptr, the main difference being
	   that it's not copyable.

	   Note that this type is functionally very different from
	   s11n::cleanup_ptr, which specifically handles the details
	   of cleaning up Serializables, whereas this type simply uses
	   delete to destroy its contents.
	*/
	template <typename T>
	    struct auto_ptr {
		auto_ptr( T * t ) : m_x(t) {}
		~auto_ptr() { delete m_x; }
		T * get() const { return m_x; }
		T * release() { T * x = m_x; m_x = 0; return x; }
		T * operator->() const { return m_x; }
		void reset( T * x )
		{
		    if( x != m_x ) delete m_x;
		    m_x = x;
		}
		/** Throws if !this->get(). */
		T & operator*() const
		{
		    if( ! m_x ) throw s11n_exception( S11N_SOURCEINFO,
						      "Attempt to dereference a null pointer via s11n::Detail::auto_ptr<>::operator*()" );
		    return *m_x;
		}
	    private:
		T * m_x;
		auto_ptr( auto_ptr const & );
		auto_ptr & operator=(auto_ptr const &);
	    };

    } // namespace Detail
} // namespace s11n
#include <s11n.net/s11n/memory.tpp>
#endif // S11N_NET_S11N_MEMORY_HPP_INCLUDED
