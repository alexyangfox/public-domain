////////////////////////////////////////////////////////////
// A demo of serializing arrays with s11n.
// Required linker arguments: -rdynamic -ls11n
#include <s11n.net/s11n/s11nlite.hpp>
#include <s11n.net/s11n/micro_api.hpp>
#include <s11n.net/s11n/functional.hpp>

#include <s11n.net/s11n/variant.hpp>
#include <s11n.net/s11n/proxy/pod/int.hpp>
#include <s11n.net/s11n/proxy/std/vector.hpp>
// #include <s11n.net/s11n/proxy/pod/double.hpp>
// #include <s11n.net/s11n/proxy/pod/char.hpp>
// #include <s11n.net/s11n/proxy/pod/string.hpp>

#define USE_FULL_CHAR_PROXY 1 /* if enabled, register an s11n proxy for char, otherwise not */

#if USE_FULL_CHAR_PROXY
struct char_s11n
{
	template <typename NodeT>
	bool operator()( NodeT & dest, char const & src ) const
	{
		typedef s11n::node_traits<NodeT> NTR;
		NTR::class_name( dest, "char" );
		NTR::set( dest, "v", src );
		return true;
	}
};
#endif

/**
   A deser proxy which looks for a property in src named 'v'. If it is
   not set an exception is thrown. If it is set then:

   - If it is length == then the value is assumed to be a char
   literal and dest is set to that char value.

   - If it is an integer value in the range of [1..255] then dest
   is set to that char value.

   - Otherwise an exception is thrown.

 */
struct char_des11n
{
	template <typename NodeT>
	bool operator()( NodeT const & src, char & dest ) const
	{
		typedef s11n::node_traits<NodeT> NTR;
		std::string val = NTR::get( src, "v", std::string() );
		if( val.empty() )
		{
		    throw s11n::s11n_exception( S11N_SOURCEINFO, "char_des11n::operator(): 'v' element is empty." );
		}
		if( 1 == val.size() )
		{ // treat it as a char literal
		    dest = val[0];
		    return true;
		}
		// try an integer val...
		int iv = NTR::get( src, "v", (int) 0 );
		if( (iv < 1) || (iv > 255) )
		{
		    throw s11n::s11n_exception( S11N_SOURCEINFO, "char_de11n::operator(): 'v' element is not an integer in the range of (1..255): [%d]", iv );
		}
		dest = static_cast<char>( iv );
		return true;
	}
};


#if USE_FULL_CHAR_PROXY
#define S11N_TYPE char
#define S11N_TYPE_NAME "char"
#define S11N_SERIALIZE_FUNCTOR char_s11n
#define S11N_DESERIALIZE_FUNCTOR char_des11n
#include <s11n.net/s11n/reg_s11n_traits.hpp>
#endif

void do_array()
{
	const int asz = 10;
	typedef int ArT;
	ArT arI[asz] = {
	65, 66, 67, 68, 69,
	97, 98, 99, 100, 101
	};

	if( 0 ) { // enable this to see an exception at deser time
	    arI[asz/2] = 300;
	    //       ^^^ a value of <1 or >255 will cause char_des11n() to fail
	}

	using namespace s11n;

	s11nlite::node_type node;

	{ // Here's one way to serialize an aray...
	    std::for_each( arI, arI+asz, ser_to_subnode_unary_f( node, "number" ) );
	    COUT << "integer array:\n";
	    if( ! s11nlite::save( node, std::cout ) )
	    {
		throw s11n::s11n_exception( S11N_SOURCEINFO, "%s:%d: save() of array failed.", __FILE__,__LINE__ );
	    }
	}

	/**
	   Serializing an array of numbers can be done much more efficiently by using the
	   serialize_streamable_{list,map}() routines, or registering a streamable proxy
	   for your POD-containing containers.
	*/

	{ // And here's one way to deserialize it...
	    s11nlite::node_type denode;
	    typedef s11nlite::node_traits NTR;
	    std::vector<char> vec; // note that we deser to an array of char, not int. Only for demo purposes.
	    std::for_each( NTR::children(node).begin(),
			   NTR::children(node).end(),
#if USE_FULL_CHAR_PROXY
			   deser_to_outiter_f<char>( std::back_inserter(vec) )
#else
			   deser_to_outiter_f<char>( std::back_inserter(vec), char_des11n() )
#endif
			   );
	    COUT << "integer array:\n";
	    if( ! s11nlite::save( vec, std::cout ) )
	    {
		throw s11n::s11n_exception( S11N_SOURCEINFO, "%s:%d: save() of vec failed.", __FILE__,__LINE__ );
	    }
	}

}

int
main( int argc, char **argv )
{
	std::string format = (argc>1) ? argv[1] : "parens";
	s11nlite::serializer_class( format ); // define output format
	try
	{
		do_array();
	}
	catch( const std::exception & ex )
	{
		std::cerr << "EXCEPTION: " << ex.what() << "\n";
		return 1;
	}
	return 0;
}
