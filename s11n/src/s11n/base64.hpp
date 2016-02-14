#ifndef s11n_BIN64_H_INCLUDED
#define s11n_BIN64_H_INCLUDED 1
////////////////////////////////////////////////////////////////////////
// s11n.hpp:
// Author: stephan beal <stephan@s11n.net>
// License: Public Domain
////////////////////////////////////////////////////////////////////////

#include <s11n.net/s11n/base64enc.hpp>
#include <s11n.net/s11n/base64dec.hpp>

namespace s11n {
/**
   The base64 namespace encapsulates code for de/serializing binary
   data using base64 encoding/decoding.

   Added in version 1.3.1.
*/
namespace base64 {

    /**
       This is a helper for serializing binary data.  It is a Serializable
       type, but does not meet all requirements for Serializables (namely,
       it is not DefaultConstructable). It is to be initialized with a
       pointer to some binary data (which is limited to a (char const *))
       and the length of that data.

       The intended usage goes something like this:

       \code
       bindata_ser bin( myPtr, lengthOfMyPtr );
       serialize( anS11nNode, bin );
       \endcode

       To deserialize it, use bindata_deser.

       IMPORTANT WARNINGS:

       Serializing binary data this way is not terribly efficient, due
       to the whole encoding/decoding process. Also most s11n data
       formats do some sort of entity translation when setting
       properties, and may peform very poorly when given huge
       inputs. Some formats may not like newlines in properties
       (simplexml_serializer comes to mind). Base64-encoded data is
       also larger than the original binary data. Thus the following
       recommendations:

       - When serializing binary data using s11n, be nice to your CPU
       and restrict it to data of reasonable sizes (no 2GB blobs).

       - Try the compact_serializer as your output format. It does not
       character translation and thus should perform much better than
       the other serializers on encoded data.
    */
    struct bindata_ser
    {
	typedef std::string::value_type char_type;
	/** The binary data we want to serialize. */
	char_type const * data;
	/** The length of this->data. */
	size_t length;

	/**
	   Sets this object to point to begin, which is required to be
	   at least n bytes long.
	*/
	bindata_ser( char_type const * begin, size_t n )
	    : data(begin), length(n)
	{}

	/**
	   Stores this->length and the base64-encoded form of this->data.
	*/
	template <typename NodeT>
	bool operator()( NodeT & dest ) const
	{
	    if( ! this->length ) return false;
	    typedef s11n::node_traits<NodeT> NTR;
	    NTR::class_name( dest, "bindata_ser" );
	    std::ostringstream obuf;
	    {
		std::istringstream ibuf( std::string( this->data, this->data + this->length ) );
		s11n::base64::encoder E;
		E.encode( ibuf, obuf );
	    }
	    NTR::set( dest, "base64", obuf.str() );
	    NTR::set( dest, "length", this->length );
	    return true;
	}
    };


    /**
       bindata_deser is a helper to deserialize base64-encoded
       binary data to a (char *). It is intended to be used
       in conjunction with bindata_ser.

       The intended usage goes something like this:

       \code
       bindata_deser bin;
       deserialize( anS11nNode, bin );
       // ... either take over bin.data or deallocate it ...
       \endcode

    */
    struct bindata_deser
    {
	typedef std::string::value_type char_type;
	/**
	   The raw binary data. It is set by the deserialize
	   operator.
	*/
	char_type * data;
	/**
	   The length of the raw binary data. It is set by the deserialize
	   operator.
	*/
	size_t length;
	bindata_deser()
	    : data(0), length(0)
	{}

	/**
	   Uses malloc() to allocate count bytes. Returns 0 on error, at
	   least theoretically (depends on the OS's allocator - some
	   always return non-0 with the assumption that memory will become
	   free at some point).
	*/
	static char * allocate( size_t count )
	{
	    return static_cast<char *>( malloc( count ) );
	}

	/**
	   Deallocates tgt.data using free() and sets
	   tgt's values to 0.
	*/
	static void deallocate( bindata_deser & tgt )
	{
	    free( tgt.data );
	    tgt.data = 0;
	    tgt.length = 0;
	}

	/**
	   Decodes base64-encoded data from src and sets this->data to
	   that data, and this->length to the length of that data. It
	   allocates the memory using malloc(). The caller owns the
	   deserialized data and must deallocate it at some point using
	   free() or deallocate(thisObject).

	   If src does not appear to contain any data, false is
	   returned.  If it contains data but does not appear to be
	   consistent (e.g.  decoded data length does not match
	   recorded length) or we cannot allocate enough memory to
	   deserialize then an s11n_exception is thrown.

	   This is not a terribly efficient routine, requiring several
	   copies of the binary data. Thus it should not be used with
	   very large data sets.
	*/
	template <typename NodeT>
	bool operator()( NodeT const & src )
	{
	    typedef s11n::node_traits<NodeT> NTR;
	    this->data = 0;
	    this->length = NTR::get( src, "length", 0 );
	    if( !length ) return false;
	    std::string cdata( NTR::get( src, "base64", std::string() ) );
	    if( cdata.empty() ) return false;

	    {
		std::ostringstream obuf;
		std::istringstream ibuf( cdata );
		s11n::base64::decoder D;
		D.decode( ibuf, obuf );
		cdata = obuf.str();
	    }

	    if( this->length != cdata.size() )
	    {
		throw s11n::s11n_exception( S11N_SOURCEINFO,
					    "bindata_deser::deserialize: 'length' property [%u] does not correspond to actual data length [%u].",
					    this->length,
					    cdata.size() );
	    }
	    // We could do the allocation before decoding, but then we'd take up yet another copy.
	    this->data = allocate( this->length );
	    if( ! this->data )
	    {
		throw s11n::s11n_exception( S11N_SOURCEINFO,
					    "bindata_deser::deserialize: could not allocate [%u] bytes for binary data.",
					    this->length );
	    }
	    memcpy( this->data, cdata.c_str(), this->length );
	    return true;
	}
    };


}}

#endif // s11n_BIN64_H_INCLUDED
