#ifndef S11N_NET_S11N_1_3_CPPX0_HPP_INCLUDED
#define S11N_NET_S11N_1_3_CPPX0_HPP_INCLUDED 1

#include <algorithm>
#include <iterator>
#include <tuple>
#include <s11n.net/s11n/algo.hpp> // serialize_subnode()
#include <s11n.net/s11n/variant.hpp>
#include <s11n.net/s11n/s11n_config.hpp>
#if ! s11n_CONFIG_HAVE_CPP0X
#error "s11n_CONFIG_HAVE_CPP0X is not set, which means this code probably won't compile (it needs C++0x support)"
#endif

namespace s11n {
/**

EXPERIMENTAL code based on C++0x features.

Code in this namespace uses C++0x features, as currently supported by
gcc 4.3 (at the time of this writing). It won't compile on any older
compilers.

*/
namespace cpp0x {

    /**
       count<> is a variadic template metafunction to count variadic
       template lists.  Used like: count<int,double,char>::value (==3)
    */
    template <typename ... Args> struct count;
    template <typename T,typename ... Args>
    struct count<T,Args...> {
	static const int value = 1 + count<Args...>::value;
    };
    template <> struct count<> { static const int value = 0; };

    namespace Detail {
	/** 
	    Does nothing and returns true. Exists to give the variadic overload a stopping point.
	*/
	template <typename NodeT>
	inline bool serialize_group_impl( const size_t, NodeT & )
	{
	    return true;
	}

	/**
	   Internal implementation of serialize_group().
	*/
	template <typename NodeT, typename SerT, typename ... SerList>
	inline bool serialize_group_impl( const size_t high, NodeT & dest, SerT const &src, SerList && ... srcN )
	{
	    typedef s11n::s11n_traits<SerT> ST;
	    return s11n::serialize_subnode( dest,
					    s11n::format_string( "i%d", high - s11n::cpp0x::count<SerList...>::value ),
					    src )
		&& serialize_group_impl( high, dest, srcN... );
	}
    }


    /**
       serialize_group() works similarly to s11n::serialize_subnode(), except
       that it can be passed one or more Serializables. A child node is added
       to dest, named groupName, and each serializable is serialized as a child
       of that "group node".

       Quite Significant Points of Interest:

       (Please do refrain from failing to read these notes!)

       #1:

       SerT and all of SerList must be Serializable types. NodeT must
       conform to s11n::node_traits<NodeT> conventions.

       #2:

       To deserialize items call deserialize_group() with the EXACT
       same group name and the same types of arguments (but non-const)
       in the EXACT same order as were passed to the corresponding
       serialize_group() call. Changing the types or order of the
       SerT+SerList list will likely lead to deserialization errors
       when reading data saved using previous calls of
       serialize_group().

       The reason for #2 is because this routine has to synthesize
       node-name-compatible name tokens for each Serializable. The only
       halfway reasonable thing it can do is name them numerically. This
       number, in turn, is used as a lookup key when deserializing, so
       the object in that position in the deser call needs to be the
       same (or s11n-compatible) type as the one passed at that arg position
       to serialize_group().

       #3:

       The string passed for the groupName parameter should be a unique
       name amongst the children of the dest node. If it is not unique
       then deserialization is likely to fail because it might get the
       wrong child node.


       Added in version 1.3.1.
    */
    template <typename NodeT, typename SerT, typename ... SerList>
    inline bool serialize_group( NodeT & dest, std::string const & groupName, SerT const &src, SerList && ... srcN )
    {
	typedef s11n::s11n_traits<SerT> ST;
	NodeT & tgt( s11n::create_child( dest, groupName ) );
	s11n::node_traits<NodeT>::class_name(tgt, "serialize_group");
	return Detail::serialize_group_impl( 1+count<SerList...>::value, tgt, src, std::forward<SerList>(srcN)... );
    }

    namespace Detail {
	/** 
	    Does nothing and returns true. Exists to give the variadic overload a stopping point.
	*/
	template <typename NodeT>
	inline bool deserialize_group_impl( size_t const, NodeT const & )
	{
	    return true;
	}
	/**
	   Internal implementation of deserialize_group().
	*/
	template <typename NodeT, typename SerT, typename ... SerList>
	inline bool deserialize_group_impl( size_t const high, NodeT const & src, SerT & dest, SerList && ... destN )
	{
	    typedef s11n::s11n_traits<SerT> ST;
	    typedef s11n::node_traits<NodeT> NTR;
	    return s11n::deserialize_subnode( src,
					      s11n::format_string( "i%d", high - count<SerList...>::value ),
					      dest )
		&& deserialize_group_impl( high, src, std::forward<SerList>(destN)... );
	}
    }

    /**
       deserialize_group() is the counterpart to serialize_group(). See that
       function for important caveats.

       If src contains no child node named groupName then this function
       will throw and s11n_exception, otherwise it will return the
       cumulative success/fail value of deserialization on each item
       in the list [SerT,SerList]. It will propagate any exceptions
       throws by the underlying deserialize() calls.
    */
    template <typename NodeT, typename SerT, typename ... SerList>
    inline bool deserialize_group( NodeT const & src, std::string const & groupName, SerT & dest, SerList && ... destN )
    {
	if( NodeT const * ch = s11n::find_child_by_name( src, groupName ) )
	{
	    return Detail::deserialize_group_impl( 1 + count<SerList...>::value, *ch, dest, std::forward<SerList>(destN)... );
	}
	throw s11n::s11n_exception(S11N_SOURCEINFO,"deserialize_group(node,'%s',...): child node not found.", groupName.c_str() );
    }


    namespace Detail {
	/** 
	    Does nothing and returns true. Exists to give the variadic overload a stopping point.
	*/
	template <typename NodeT>
	inline bool serialize_subnodes( NodeT & ) throw()
	{
	    return true;
	}
	/** 
	    Does nothing and returns true. Exists to give the variadic overload a stopping point.
	*/
	template <typename NodeT>
	inline bool deserialize_subnodes( NodeT const & ) throw()
	{
	    return true;
	}
    }

    /**
       A variadic form of s11n::serialize_subnode(), it takes pairs of
       (string,Serializable) and calls serialize_subnode() for each
       one. If the argument list has an odd number of elements then
       this function will not compile (a static assertion fails).
    */
    template <typename NodeT, typename SerT, typename ... SerList>
    inline bool serialize_subnodes( NodeT & dest, std::string const & name, SerT const & src, SerList && ... srcN )
    {
	static_assert( 0 == (count<SerList...>::value % 2),
		       "Wrong number of template args passed to serialize_subnodes(): args must be passed in pairs of (string,Serializable)");
	using namespace Detail;
	return s11n::serialize_subnode( dest, name, src )
	    && serialize_subnodes( dest, std::forward<SerList>(srcN)... );
    }

    /**
       The counterpart to serialize_subnodes(). See that function for
       how it works.  Unlike serialize_group(), this function does not
       care about the order of the (string,Serializable) pairs because
       they are looked up by name instead of position. (OTOH, this
       function is slightly slower because of that.)
    */
    template <typename NodeT, typename SerT, typename ... SerList>
    inline bool deserialize_subnodes( NodeT const & src, std::string const & name, SerT & dest, SerList && ... destN )
    {
	static_assert( 0 == (count<SerList...>::value % 2),
		       "Wrong number of template args passed to deserialize_subnodes(): args must be passed in pairs of (string,Serializable)");
	using namespace Detail;
	return s11n::deserialize_subnode( src, name, dest )
	    && deserialize_subnodes( src, destN... );
    }

    /**
       A variadic form of s11n::serialize_versioned(), except that it
       bundles multiple Serializables with a single version.

       This function has one notable caveat: it uses serialize_group() to
       do the leg-work, which means that when you call
       deserialize_versioned() to fetch the data the arguments passed
       to it must be of the same type and in the same order as those
       passed to serialized_versioned(). See serialize_group() for
       more information on that requirement.

       TODO: try to re-implement in terms of the non-variadic form to avoid
       duplicate code/key names.
    */
    template <typename NodeT, typename VersionT, typename SerT, typename ... SerList>
    inline bool serialize_versioned( NodeT & dest, VersionT ver, SerT const & src, SerList && ... srcN )
    {
	typedef s11n::node_traits<NodeT> NTR;
	NTR::set( dest, "version", ver );
	return serialize_group( dest, "vdata", src, srcN... );
    }

    /**
       A variadic form of s11n::deserialize_versioned(). See the variadic form
       of serialize_version() for important information.

       TODO: try to re-implement in terms of the non-variadic form, to avoid
       duplicate code/key names.
    */
    template <typename NodeT, typename VersionT, typename SerT, typename ... SerList>
    bool deserialize_versioned( NodeT & src, VersionT ver, SerT & dest, SerList && ... destN )
    {
	typedef s11n::node_traits<NodeT> NTR;
	VersionT gotver( NTR::get( src, "version", VersionT() ) );
	if( ver !=  gotver )
	{
	    using s11n::Detail::variant;
	    variant v1( ver );
	    variant v2( gotver );
	    throw s11n_exception( S11N_SOURCEINFO, "Version mismatch. Expected '%s' but got '%s'.",
				  v1.str().c_str(),
				  v2.str().c_str() );
	}
	return deserialize_group( src, "vdata", dest, destN... );
    }


} } // namespaces

#endif // S11N_NET_S11N_1_3_CPPX0_HPP_INCLUDED
