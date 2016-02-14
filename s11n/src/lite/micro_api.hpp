#ifndef s11nlite_MICROAPI_HPP_INCLUDED
#define s11nlite_MICROAPI_HPP_INCLUDED 1

#include <s11n.net/s11n/s11nlite.hpp>
#include <s11n.net/s11n/mutex.hpp>

namespace s11nlite
{

	/**
	   micro_api is of arguable utility, written just to see what
	   happens. It is intended to be used for saving and loading
	   sets of like-typed Serializables. For apps which do lots of
	   loading and saving of homogeonous Serializables, micro_api
	   can cut down on the amount of typing (intentionally
	   ambiguous) needed:

<pre>
s11nlite::micro_api<MyT> micro;
MyT my;
... populate my ...
micro.save( my, "somefile" );
...
MyT * your = micro.load( "somefile" );
</pre>

           Unlike the s11nlite interface, micro_api explicitely hides
           all node-level details. It does allow the user to set an
           output format (serializer class), as it may be desirable to
           save different collections of objects in different formats.
           For loading it uses s11nlite, so it supports any formats
	   supported there.

	   Templatized on:

	   - SerializableType must be the base-most Serializable type
	   in a given Serializable hierarchy (or the Serializable type
	   itself, for monomorphs).

	   Any functions in this class might throw if their s11n[lite]
	   counterparts do.

	   Multi-threading:

	   Using a single micro_api instance from multiple
	   threads is not encouraged, but it just might work.
	   Even so, some operations must be locked from the client side
	   to avoid undesired racing, such as operation sequences
	   like (micro.buffer(Object); string s=micro.buffer())
	   to ensure that the buffer is not hammered by a parallel
	   call to micro.buffer(XXX) just before (s=micro.buffer())
	   is called.
	*/
	template <typename SerializableType>
	class micro_api
	{
	public:
		/**
		   The base Serializable interface associated with
		   this object.
		*/
		typedef SerializableType serializable_type;

	private:
		typedef s11nlite::node_type node_type;
		typedef s11nlite::node_traits node_traits;
		typedef s11nlite::serializer_interface serializer_interface;
		std::string m_serclass;
		std::string m_buffer;
		mutable s11n::mutex m_mutex;
		/**
		   Internal helper to reduce a small amount of code duplication
		   in the two save() variants.

		   Serializes src to dest, returning 0 if that fails.
		   If serialization to the node succeeds, it returns
		   s11nlite::create_serialize(this->serializer_class()). The
		   caller owns the returned pointer, which may be 0.
		*/
		serializer_interface *
		prepare_save( node_type & dest, const serializable_type & src ) const
		{
			if( ! ::s11n::serialize<node_type,serializable_type>( dest, src ) )
			{
				return 0;
			}
			return ::s11nlite::create_serializer(this->m_serclass);
		}
	public:
		/**
		   Constructs an object with the given serializer_class().
		*/
		explicit micro_api( const std::string & serclass )
			: m_serclass(serclass),
			  m_buffer(),
			  m_mutex()
		{
		}

		/** Does nothing. */
		~micro_api() {}
		
		/**
		   Constructs an object with the same
		   serializer_class() as s11nlite::serializer_class().
		*/
		micro_api()
			: m_serclass(::s11nlite::serializer_class()),
			  m_buffer(),
			  m_mutex()
		{
		}

		/** Returns the current serializer class name. */
		std::string serializer_class() const
		{
			return this->m_serclass;
		}

		/**
		   Sets the current serializer class name.
 		*/
		void serializer_class(const std::string & s)
		{
			s11n::mutex_sentry lock(this->m_mutex);
			// ^^^ keep m_serclass from changing during save()
			this->m_serclass = s;
		}

		/**
		   Serializes src to an internal buffer, which may be fetched
		   and cleared with buffer() and clear_buffer(), respectively.
		*/
		bool buffer( const serializable_type & src )
		{
			std::ostringstream os;
			if( ! this->save( src, os ) ) return false;
			// save() locks, so we can't lock until after save() completes.
			s11n::mutex_sentry lock(this->m_mutex);
			this->m_buffer = os.str();
			return true;
		}

		/**
		   Returns this object's buffered data, if any has
		   been set using buffer(serializable_type). The
		   buffer can be deserialized by wrapping it in an
		   istringstream.

		   Note that only Serializers which can write to
		   streams can be used here (e.g., not
		   sqlite3_serializer).
		*/
		std::string buffer() const
		{
			s11n::mutex_sentry lock(this->m_mutex);
			// ^^^ buffer might be changing right now.
			return this->m_buffer;
		}

		/**
		   Clears any buffered data saved using
		   buffer(serializable_type).
		*/
		void clear_buffer()
		{
			s11n::mutex_sentry lock(this->m_mutex);
			// ^^^ buffer might be changing right now.
			this->m_buffer = std::string();
		}

		/**
		   Saves src to dest, returning true on success and false on error.
		   If the underlying call(s) to serialize throw then this function
		   pass on the exception.

		   Note that dest does not get parsed to see if it's a
		   URL. Instead it goes directly to the
		   Serializer. This distinction is important for
		   Serializers which treat filenames and streams
		   differently (e.g., mysql_serializer and
		   sqlite3_serializer). If you want to get the
		   built-in URL support for your filename strings, use
		   s11n::io::get_ostream(). Likewise, for loading, use
		   s11n::io::get_istream().
		*/
		bool save( const serializable_type & src, const std::string & dest ) const
		{
			s11n::mutex_sentry lock(this->m_mutex);
			node_type n;
			s11n::Detail::auto_ptr<serializer_interface> sap( this->prepare_save( n, src ) );
			return sap.get() ? sap->serialize( n, dest ) : false;
		}

		/** Overload taking an ostream. */
		bool save( const serializable_type & src, std::ostream & dest ) const
		{
			s11n::mutex_sentry lock(this->m_mutex);
			node_type n;
			s11n::Detail::auto_ptr<serializer_interface> sap( this->prepare_save( n, src ) );
			return sap.get() ? sap->serialize( n, dest ) : false;
		}

		/**
		   Loads a serializable_type from src, returning 0 on
		   failure and a valid pointer on success (which the
		   caller owns). If the underlying call(s) to
		   serialize throw then this function passes on the
		   exception. In that case the object which was allocated
		   for the process (if any) is destroyed, cleaned up
		   by the s11n_traits::cleanup_functor mechanism, which is
		   believed to be relatively safe from memory leaks.
		*/
		serializable_type * load( const std::string & src ) const
		{
			return ::s11nlite::load_serializable<serializable_type>( src );
		}
	
		/**
		   An overload which takes an istream instead of a string.
		*/
		serializable_type * load( std::istream & src ) const
		{
			return ::s11nlite::load_serializable<serializable_type>( src );
		}

		/**
		   Loads a Serializable from the buffer() content, if any.
		   The caller owns the returned pointer, which may be 0.
		*/
		serializable_type * load_buffer() const
		{
			s11n::mutex_sentry lock(this->m_mutex);
			if( this->m_buffer.empty() ) return 0;
			std::istringstream is( this->m_buffer );
			return ::s11nlite::load_serializable<serializable_type>( is );
		}
		

	};


} // namespace s11nlite


#endif // s11nlite_MICROAPI_HPP_INCLUDED
