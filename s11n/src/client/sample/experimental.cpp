////////////////////////////////////////////////////////////////////////
// A test & demo app for s11n[lite].
// Author: stephan@s11n.net
// License: Do As You Damned Well Please
////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <memory> // auto_ptr
#include <stdexcept>
#include <string.h> // memcpy()
#include <stdlib.h> // malloc()/free()

////////////////////////////////////////////////////////////////////////
#include <s11n.net/s11n/s11nlite.hpp> // s11n & s11nlite frameworks
#if s11n_CONFIG_HAVE_CPP0X
#include <s11n.net/s11n/0x/ohex.hpp>
#endif

////////////////////////////////////////////////////////////////////////
// Proxies we'll need:

#include <s11n.net/s11n/proxy/pod/int.hpp>
#include <s11n.net/s11n/proxy/pod/double.hpp>
#include <s11n.net/s11n/proxy/pod/string.hpp>
#include <s11n.net/s11n/proxy/std/vector.hpp>

#if 0
#define S11N_MAP_TYPE_PROXY ::s11n::map::serialize_streamable_map_f
#define S11N_MAP_TYPE_DESER_PROXY ::s11n::map::deserialize_streamable_map_f
#include <s11n.net/s11n/proxy/std/map.hpp>
// ^^^^ those #defines are consumed and undefined by the map reg supermacro
#else
#include <s11n.net/s11n/proxy/std/map.hpp>
#include <s11n.net/s11n/proxy/std/list.hpp>
#endif

////////////////////////////////////////////////////////////////////////
// misc util stuff
#include <s11n.net/s11n/s11n_debuggering_macros.hpp> // CERR
#include <s11n.net/s11n/functional.hpp>
#include <s11n.net/s11n/micro_api.hpp>
////////////////////////////////////////////////////////////////////////
#define S11N_THROW(X) throw s11n::s11n_exception( S11N_SOURCEINFO, X )


struct my_serialize_fwd
{
	mutable int count;
	template <typename NT, typename ST>
	bool operator()( NT & dest, const ST & src ) const
	{
		CERR << "Routed through our custom serialize thingie: count=" << ++this->count << "\n";
		return s11n::serialize<NT,ST>( dest, src );
	}
	template <typename NT, typename ST>
		bool operator()( const NT & src, ST & dest ) const
	{
		CERR << "Routed through our custom deserialize thingie: count=" << ++this->count << "\n";
		return s11n::deserialize<NT,ST>( src, dest );
	}
};


void do_some_tests()
{
	using namespace s11n;
	typedef s11n_node NodeT;
	typedef node_traits<NodeT> NTR;
	typedef int BogoT;

	BogoT bogo = 7;
	serializable_f<BogoT,my_serialize_fwd> sr( bogo );

	s11n_node node;
	sr( node );
	sr( node );
	ser_f( bogo )(node);

	int unbogo;
	deser_f( unbogo, sr.functor /* a copy :( */ )( node );
	/** ^^^ same as this, but won't work: const-ambiguous:
	    sr.functor( node, unbogo )

	    i don't LIKE the .functor member and may look to get rid
	    of it. Maybe it's useful, though. Dunno yet.
	 */
	s11nlite::save( unbogo, std::cout );
	node.clear();
	typedef std::vector<BogoT> BV;
	NTR::class_name( node, s11n_traits<BV>::class_name(0) );
	BV vec(0);
	for( int i = 100; i >90 ; --i ) vec.push_back(i);

	std::for_each( vec.begin(),
		       vec.end(),
		       ser_to_subnode_unary_f( node, "child" )
		       );

	CERR << "object using standard approach:\n";
	s11nlite::save( vec, std::cout );
	CERR << "object using functor approach:\n";
	s11nlite::save( node, std::cout );


	BV unvec;
	deserializable_f<BV> devec( unvec );
	/** ^^^^ This only works because the "algo" we used to
	    serialize node is structurally compatible with the default
	    vector<> proxy.
	*/
	devec( node );
	CERR << "Deserialized from functor-approach data:\n";
	s11nlite::save( unvec, std::cout );


	// An odd, but interesting, technique for deserialization:
	CERR << "And re-deserialized through a somewhat strange technique...\n";
	std::vector<std::string> sunvec;
	std::for_each( NTR::children(node).begin(),
		       NTR::children(node).end(),
		       deser_to_outiter_f<std::string>( std::back_inserter(sunvec) ) );
	s11nlite::save( sunvec, std::cout );
	if( sunvec.size() != unvec.size() )
	{
		throw s11n::s11n_exception( S11N_SOURCEINFO, "Compare of vectors failed." );
	}
	unvec.clear();



	typedef std::map<int,std::string> MapT;
	MapT map;
	int at = 0;
	map[at++] = "one";
	map[at++] = "two";
	map[at++] = "three";

	node.clear();
	ser_f( map )( node );
	CERR << "Map starts out like this:\n";
	s11nlite::save( node, std::cout );

	CERR << "Map using ser_to_subnode_f():\n";
	node.clear();
	std::for_each( map.begin(),
		       map.end(),
		       ser_to_subnode_unary_f( node, "child", s11n::map::serialize_streamable_pair_f() )
		       );
	s11nlite::save( node, std::cout );

	MapT unmap;
	typedef std::pair< MapT::key_type, MapT::mapped_type > NCPair; // drop the const part of MapT::value_type.first
	std::for_each( NTR::children(node).begin(),
		       NTR::children(node).end(),
		       deser_to_outiter_f<NCPair>( std::inserter(unmap, unmap.begin()),
						    s11n::map::deserialize_streamable_pair_f() )
		       );
	CERR << "Deserialized using deser_from_subnode_f():\n";
	s11nlite::save( unmap, std::cout );

	pointer_f<MapT> pf( unmap );
	CERR << "pointer_f() == " << std::hex << pf() << ", map size="<<pf->size()<<"\n";
	pointer_f<const MapT *> pf2( &unmap );
	CERR << "pf2() == " << std::hex << pf2() << "\n";

}



void do_some_more_tests()
{
	using namespace s11n;
	typedef s11n_node NT;
	typedef node_traits<NT> NTR;


	typedef std::map<int,std::string> MapT;
	MapT map;
	int at = 0;
	map[at++] = "one";
	map[at++] = "two";
	map[at++] = "three";

	NT node;

	typedef s11n::map::serialize_streamable_map_f MapSer;
	typedef s11n::map::deserialize_streamable_map_f MapDeser;
	MapSer mapser;
	MapDeser mapdeser;

	serialize_to_subnode_f<MapSer> algo( "child" );

	if( ! ser_nullary_f( node, map, algo )() )
	{
		throw s11n::s11n_exception( S11N_SOURCEINFO, "ser_nullary_f test failed :(" );
	}
	CERR << "ser_nullary_f():\n";
	s11nlite::save( node, std::cout );

	deserialize_from_subnode_f<MapDeser> dealgo( "child" );
	MapT demap;
	if( ! deser_nullary_f( node, demap, dealgo )() )
	{
		throw s11n::s11n_exception( S11N_SOURCEINFO, "deser_nullary_ test failed :(" );
	}
	CERR << "deser_nullary_f():\n";
	s11nlite::save( demap, std::cout );

	node.clear();
	if( ! ser_nullary_f(node, demap, mapser )() )
	{
		throw s11n::s11n_exception( S11N_SOURCEINFO, "ser_nullary_f test failed :(" );
	}
	CERR << "ser_nullary_f():\n";
	s11nlite::save( node, std::cout );

	MapT unmap;
	if( ! deser_nullary_f( node, unmap, mapdeser )() )
	{
		throw s11n::s11n_exception( S11N_SOURCEINFO, "deser_nullary_f test failed :(" );
	}
	CERR << "deser_nullary_f():\n";
	s11nlite::save( unmap, std::cout );

	node.clear();
	unmap.clear();
	if( !
	    s11n::logical_and( ser_nullary_f( node, map, mapser ),
			       deser_nullary_f( node, unmap, mapdeser )
			       )()
	    )
	{
		throw s11n::s11n_exception( S11N_SOURCEINFO, "logical_and() test failed :(" );
	}
	CERR << "s11n_cast via logical_and():\n";
	s11nlite::save( unmap, std::cout );

}

template <typename NodeT>
class node_deser_helper// : public ::s11n::deserialize_unary_serializable_f_tag
{
public:
	typedef NodeT node_type;
	typedef ::s11n::node_traits<NodeT> node_traits;
	typedef typename node_traits::child_list_type child_list_type;

	node_deser_helper( node_type const & n ) : m_node(n)
	{
	}

	node_type const & node() const
	{
		return this->m_node;
	}

	const child_list_type & children() const
	{
		return node_traits::children(this->m_node);
	}

	template <typename SerializableT>
	SerializableT * deserialize( std::string const & nodename ) const
	{
		return ::s11n::deserialize_subnode<node_type,SerializableT>( this->node(), nodename );
	}

	/** Lame, but true. Avoids forcing user to explicitely std::string-ize string identifiers. */
	template <typename SerializableT>
	SerializableT * deserialize( char const * nodename ) const
	{
		return ::s11n::deserialize_subnode<node_type,SerializableT>( this->node(), nodename );
	}

	template <typename SerializableT>
	SerializableT * deserialize() const
	{
		return ::s11n::deserialize<node_type,SerializableT>( this->node() );
	}

	template <typename StreamableT>
	StreamableT get( std::string const & key, StreamableT const & default_val = StreamableT() ) const
	{
		return node_traits::get( this->node(), key, default_val );
	}

private:
	node_type const & m_node;
};

template <typename NodeT>
class node_ser_helper
{
public:
	typedef NodeT node_type;
	typedef ::s11n::node_traits<NodeT> node_traits;
	typedef typename node_traits::child_list_type child_list_type;

	node_ser_helper( node_type & n ) : m_node(n)
	{
	}

	node_type & node()
	{
		return this->m_node;
	}

	child_list_type & children()
	{
		return node_traits::children(this->m_node);
	}

	template <typename SerializableT>
	bool serialize( SerializableT const & src )
	{
		return ::s11n::serialize<node_type,SerializableT>( this->node(), src );
	}

	template <typename SerializableT>
	bool serialize( std::string const & nodename, SerializableT const & src )
	{
		return ::s11n::serialize_subnode<node_type,SerializableT>( this->node(), nodename, src );
	}

	/** Lame, but true. Avoids forcing user to explicitely std::string-ize string identifiers. */
	template <typename SerializableT>
	bool serialize( char const * nodename, SerializableT const & src )
	{
		return this->serialize<SerializableT>( std::string(nodename), src );
	}

	template <typename StreamableT>
	void set( std::string const & key, StreamableT const & val )
	{
		return node_traits::set( this->node(), key, val );
	}

private:
	node_type & m_node;
};


void do_even_more_tests()
{
	typedef std::map<int,std::string> MapT;
	MapT map;
	int at = 0;
	map[at++] = "value #one";
	map[at++] = "value #two";
	map[at++] = "value #three";


	typedef s11nlite::node_type NT;
	typedef node_ser_helper<NT> NHT;
	NT node;
	NHT h(node);

	h.set( "foo", 1 );
	h.set( "bar", 2 );
	h.set( "barre", "hello, world" );

	h.serialize( "mymap", map );

	s11nlite::serializer_class("parens");
	if( ! s11nlite::save( h.node(), "/dev/null" ) )
	{
		throw s11n::s11n_exception( S11N_SOURCEINFO, "Save of MapT failed :(" );
	}

	typedef node_deser_helper<NT> CNHT;
	CNHT const ch(node);
	CERR << "foo == " << ch.get<int>("foo") << '\n';
	CERR << "bar == " << ch.get<int>("bar") << '\n';
	CERR << "barre == " << ch.get<std::string>("barre") << '\n';
	s11n::cleanup_ptr<MapT> clean( ch.deserialize<MapT>("mymap") );
	if( ! clean.get() )
	{
		throw s11n::s11n_exception( S11N_SOURCEINFO, "Deser of MapT failed :(" );
	}
	if( ! s11nlite::save( *clean, std::cout ) )
	{
		throw s11n::s11n_exception( S11N_SOURCEINFO, "Save of MapT failed :(" );
	}

	return;
}

/**
   SerializeChainer is the mirror class of DeserializeChainer. It is
   intended to simplify the serialization of member data with a small
   context (e.g., inside a class' serialization operator).

   NoteT must be compatible with S11nNode conventions.

   All significant functions of this object return a reference to
   this object so that the calls can be chained. Those functions
   all throw on error, so if they return then the op succeeded.

   Unfortunately, we cannot design this class as a functor, using
   operator() as the serialization operation, because that greatly
   complicates (on the client side) passing explicit template
   type arguments.
*/
template <typename NodeType>
struct SerializeChainer
{
	typedef NodeType node_type;
	typedef s11n::node_traits<node_type> node_traits;
	explicit SerializeChainer( node_type & dest )
		: m_n(dest) {}

	node_type & node() { return this->m_n; }

	/**
	   SerT must be a Serializable.  SerializeBinaryFTag must be a
	   function conforming to s11n::serialize_binary_f_tag.

	   Creates a new node_type object, serializes src into it, and
	   adds the child to this->node().

	   Returns a reference to this object on success and throws
	   on error. On error this->node() is not modified.
	*/
	template <typename SerT, typename SerializeBinaryFTag>
	SerializeChainer const & ser_f( std::string const & nodename,
					SerT const & src,
					SerializeBinaryFTag const func
					) const
	{
		std::auto_ptr<node_type> dest( node_traits::create( nodename ) );
		if( ! func( *dest, src ) ) // <-- FIXME? this call might not work if SerT is a derived type, not the base.
		{
			throw s11n::s11n_exception(S11N_SOURCEINFO,
						   "{serialize_binary_f_tag}(S11nNode[name=%s],SerializableT[class=%s]) failed.",
						   nodename.c_str(),
						   s11n::s11n_traits<SerT>::class_name(&src).c_str()
						   );
		}
		node_traits::children(this->m_n).push_back( dest.release() );
		return *this;
	}

	/**
	   Serializes src into a subnode (named nodename) of
	   this->node(). If the deserialization fails, this->node() is
	   not modified and an exception is thrown.
	*/
	template <typename SerT>
	SerializeChainer const & ser( std::string const & nodename,
				      SerT const & src ) const
	{
		return this->ser_f<SerT>( nodename, src, s11n::serialize_binary_f() );
	}

	/**
	   Sets key=val in this->node() and returns a reference to this object.

	   StreamableT must be ostreamable.
	*/
	template <typename StreamableT>
	SerializeChainer const & set( std::string const & key,
				      StreamableT const & val ) const
	{
		node_traits::set( this->m_n, key, val );
		return *this;
	}


private:
	node_type & m_n;
};

/**
   DeserializeChainer is the mirror class of SerializeChainer. It is intended
   to simplify the deserialization of member data.

   NoteT must be compatible with S11nNode conventions.

   All significant functions of this object return a reference to
   this object so that the calls can be chained. Those functions
   all throw on error, so if they return then the op succeeded.

   Unfortunately, we cannot design this class as a functor, using
   operator() as the deserialization operation, because that greatly
   complicates (on the client side) passing explicit template
   type arguments.

*/
template <typename NodeT>
class DeserializeChainer
{
public:
	typedef NodeT node_type;
	typedef ::s11n::node_traits<NodeT> node_traits;
	/**
	   Points this object at src. src must outlive this
	   this object.
	*/
	DeserializeChainer( node_type const & src ) : m_node(src)
	{
	}

	/**
	   Returns this object's imutable node.
	*/
	node_type const & node() const
	{
		return this->m_node;
	}

	/**
	   Deserializes the first SUBNODE of this->node() named
	   nodename into dest using the given functor.

	   SerializableT is a Serializable type and
	   DeserializeBinaryFTag must comply with
	   s11n::deserialize_binary_f_tag's expectations.

	   Assuming the named child is found, func is used
	   on that node to deserialize it into dest.

	   Throws an s11n_exception on error.
	*/
	template <typename SerializableT,typename DeserializeBinaryFTag>
	DeserializeChainer const & des_f( std::string const & nodename,
				    SerializableT & dest,
				    DeserializeBinaryFTag func  ) const
	{
		s11n::deserialize_from_subnode_f<DeserializeBinaryFTag> f2(nodename,func);
		if( ! f2( this->node(), dest ) )
		{
			throw s11n::s11n_exception(S11N_SOURCEINFO,
						   "des_f(%s,Serializable[class=%s],BinaryFunctor) failed.",
						   nodename.c_str(),
						   s11n::s11n_traits<SerializableT>::class_name(&dest).c_str()
						   );
		}
		return *this;		
	}

	/**
	   Deserializes the FIRST child node of this object with the
	   given nodename into dest. Returns true on success, false on
	   error.

	   Throws an s11n_exception on error.
	*/
	template <typename SerializableT>
	DeserializeChainer const & des( std::string const & nodename,
				  SerializableT & dest ) const
	{
		return this->des_f<SerializableT>( nodename, dest, s11n::deserialize_binary_f() );
	}

	/**
	   Deserializes the first child node named nodename into
	   dest by passing them both to func, which must conform to
	   s11n::deserialize_binary_f_tag's expectations.

	   If passed a null pointer and this function fails, target is
	   not modified. If deserialization fails, any
	   internally-created (DeserializableT*) is deallocated using
	   cleanup_serializable(). If passed a non-null pointer and
	   the function fails, behaviour is as for the non-pointer
	   variant of deserialize() - target may or may not be in a
	   defined state, as defined by the specific proxy algorithm.
	*/
 	template <typename SerializableT,typename DeserializeBinaryFtor>
 	DeserializeChainer const & des_p_f( std::string const & nodename,
					    SerializableT * & dest,
					    DeserializeBinaryFtor func ) const
 	{
		const node_type * ch = ::s11n::find_child_by_name( this->node(), nodename );
		if( ! ch )
		{
 			throw s11n::s11n_exception(S11N_SOURCEINFO,
 						   "des_p_f(%s,Serializable *[class=%s],Functor): named node not found",
						   nodename.c_str(),
 						   s11n::s11n_traits<SerializableT>::class_name(dest).c_str()
 						   );
		}
		typedef s11n::cleanup_ptr<SerializableT> CP;
		CP x(0);
		if( ! dest )
		{ // classload it...
			x.take( ::s11n::cl::classload<SerializableT>( node_traits::class_name( this->node() ) ) );
			if( ! x.get() ) // try harder...
			{
				x.take( ::s11n::cl::classload<SerializableT>( s11n::s11n_traits<SerializableT>::class_name( 0 ) ) );
			}
			if( ! x.get() )
			{
				throw s11n::s11n_exception(S11N_SOURCEINFO,
							   "des_p_f(%s,Serializable *[class=%s],Functor): classload of Serializable failed!",
							   nodename.c_str(),
							   s11n::s11n_traits<SerializableT>::class_name(dest).c_str()
							   );
			}
		}
		if( ! func( *ch, x.empty() ? *dest : *x ) )
		{
			throw s11n::s11n_exception(S11N_SOURCEINFO,
						   "des_p_f(%s,Serializable *[class=%s],Functor): functor failed",
						   nodename.c_str(),
						   s11n::s11n_traits<SerializableT>::class_name(dest).c_str()
						   );
		}
		if( ! x.empty() )
		{
			dest = x.release();
		}
		return *this;
	}

	/**
	   Deserializes the first child node named nodename into
	   dest.get() using the given functor, which must conform
	   to s11n::deserialize_binary_f_tag's conventions.

	   If dest.empty() is true when this function returns and this
	   function fails, target is not modified. If deserialization
	   fails, any internally-created (DeserializableT*) is
	   deallocated using cleanup_serializable(). If passed a
	   non-null pointer and the function fails, behaviour is as
	   for the non-pointer variant of deserialize() - target may
	   or may not be in a defined state, as defined by the
	   specific proxy algorithm.
	*/
 	template <typename SerializableT,typename DeserializeBinaryFtor>
 	DeserializeChainer const & des_p_f( std::string const & nodename,
					    s11n::cleanup_ptr<SerializableT> & dest,
					    DeserializeBinaryFtor func ) const
 	{
		SerializableT * d = dest.get();
		this->des_p_f( nodename, d, func );
		dest.take(d);
		return *this;
	}

	/**
	   Deserializes the first child node named nodename into
	   dest.

	   If passed a null pointer and this function fails, target is
	   not modified. If deserialization fails, any
	   internally-created (DeserializableT*) is deallocated using
	   cleanup_serializable(). If passed a non-null pointer and
	   the function fails, behaviour is as for the non-pointer
	   variant of deserialize() - target may or may not be in a
	   defined state, as defined by the specific proxy algorithm.
	*/
 	template <typename SerializableT>
 	DeserializeChainer const & des_p( std::string const & nodename,
					  SerializableT * & dest ) const
 	{
		return this->des_p_f( nodename, dest, s11n::deserialize_binary_f() );
 	}

	/**
	   Deserializes the first child node named nodename into
	   dest.get().

	   If dest.empty() is true when this function returns and this
	   function fails, target is not modified. If deserialization
	   fails, any internally-created (DeserializableT*) is
	   deallocated using cleanup_serializable(). If passed a
	   non-null pointer and the function fails, behaviour is as
	   for the non-pointer variant of deserialize() - target may
	   or may not be in a defined state, as defined by the
	   specific proxy algorithm.

	*/
 	template <typename SerializableT,typename DeserializeBinaryFtor>
 	DeserializeChainer const & des_p( std::string const & nodename,
					  s11n::cleanup_ptr<SerializableT> & dest ) const
 	{
		SerializableT * d = dest.get();
		this->des_p_f( nodename, d, s11n::deserialize_binary_f() );
		dest.take(d);
		return *this;
	}


	/**
	   Deserializes this node into dest.

	   If dest is null when this function returns and this
	   function fails, target is not modified. If deserialization
	   fails, any internally-created (DeserializableT*) is
	   deallocated using cleanup_serializable(). If passed a
	   non-null pointer and the function fails, behaviour is as
	   for the non-pointer variant of deserialize() - target may
	   or may not be in a defined state, as defined by the
	   specific proxy algorithm.
	*/
 	template <typename SerializableT>
 	DeserializeChainer const & des_p( SerializableT * & dest ) const
 	{
		if( ! s11n::deserialize<NodeT,SerializableT>( *this, dest ) )
		{
 			throw s11n::s11n_exception(S11N_SOURCEINFO,
 						   "des_p(Serializable *[class=%s]) failed: deserialize() failed",
 						   s11n::s11n_traits<SerializableT>::class_name(&dest).c_str()
 						   );
		}
 		return *this;
 	}

	/**
	   Deserializes this node into dest.get().

	   If dest.get() is null when this function returns and this
	   function fails, target is not modified. If deserialization
	   fails, any internally-created (DeserializableT*) is
	   deallocated using cleanup_serializable(). If passed a
	   non-null pointer and the function fails, behaviour is as
	   for the non-pointer variant of deserialize() - target may
	   or may not be in a defined state, as defined by the
	   specific proxy algorithm.
	*/
 	template <typename SerializableT>
 	DeserializeChainer const & des_p( s11n::cleanup_ptr<SerializableT> & dest ) const
 	{
		SerializableT * d  = dest.get();
		this->des_p<SerializableT>( d );
		dest.take(d);
 		return *this;
 	}


	/**
	   Deserializes this->node() to dest using the given functor,
	   which must comply with s11n::deserialize_binary_f_tag's
	   expectations.

	   Throws an s11n_exception on error.
	*/
	template <typename SerializableT, typename DeserializeBinaryFTag>
	DeserializeChainer const & des_f( SerializableT & dest, DeserializeBinaryFTag func ) const
	{
		if( ! func( this->node(), dest ) )
		{
			throw s11n::s11n_exception(S11N_SOURCEINFO,
						   "des_f(Serializable[class=%s],BinaryFunctor) failed.",
						   s11n::s11n_traits<SerializableT>::class_name(&dest)
						   );
		}
		return *this;
	}

	/**
	   Deserializes THIS node to dest using the default
	   deserialization path.

	   Throws an s11n_exception on error.
	*/
	template <typename SerializableT, typename DeserializeBinaryFTag>
	DeserializeChainer const & des( SerializableT & dest ) const
	{
		this->des_f<SerializableT>( dest, s11n::deserialize_binary_f() );
		return *this;
	}

	/**
	   Gets the value of the property represented by key
	   and stuffs it in dest, using default_val on error.
	*/
	template <typename StreamableT>
	DeserializeChainer const & get( std::string const & key,
					StreamableT & dest,
					StreamableT const & default_val = StreamableT() ) const
	{
		dest = node_traits::get( this->node(), key, default_val );
		return *this;
	}

private:
	node_type const & m_node;
};


void and_yet_MORE_TESTS()
{
	typedef s11nlite::node_type NT;

	typedef std::map<int,std::string> MapT;
	MapT map;
	int at = 0;
	map[at++] = "value #one";
	map[at++] = "value #two";
	map[at++] = "value #three";

	MapT map2;
	map2[at++] = "value #four";
	map2[at++] = "value #five";
	map2[at++] = "value #six";

	NT n;
	typedef SerializeChainer<NT> Chain;
	Chain chain( n );
 	chain.ser( "map", map ).
 		set( "foo", "barre").
 		set( "meaning", 42.42).
		ser( "a_string", std::string("hello, world") ).
		ser_f( "map2", map2, s11n::map::map_serializable_proxy() ).
		ser_f( "seven", 7, s11n::serialize_to_subnode_f<>( "a_subnode") )
 		;
	s11nlite::save( n, std::cout );

 	double meaning = 0.0;
 	std::string a_string = "error";
 	int seven = 0;

	typedef DeserializeChainer<NT> Dechain;
	Dechain dain( n );

	
	MapT demap;
	MapT demap2;
	s11n::cleanup_ptr<MapT> demap3;
	a_string = "error";
	seven = -1;
	meaning = 0.0;
	dain.get("meaning", meaning, 0.0).
		des( "map", demap ).
		des("a_string",a_string).
		des_f("seven", seven, s11n::deserialize_from_subnode_f<>("a_subnode") ).
		des_f("map2", demap2, s11n::map::map_serializable_proxy() ).
		des_p_f("map2", demap3, s11n::map::map_serializable_proxy() )
		;
	COUT << "meaning == "<<meaning<<"\n";
	COUT << "a_string == "<<a_string<<"\n";
	COUT << "seven == "<<seven<<"\n";
	COUT << "demap2.size() == "<<demap2.size()<<'\n';
	COUT << "demap3.get() == "<<demap3.get()<<'\n';
	s11nlite::save( *demap3, std::cout );
}

void do_version()
{
    using namespace s11n;
#if 1
    int v1 = 1;
#else
    std::string v1("abc");
#endif
    int d1 = 42;
    s11n_node node;
    if( ! serialize_versioned( node, v1, d1 ) )  throw s11n_exception(S11N_SOURCEINFO,"");
    s11nlite::save( node, std::cout );

    std::string dval("error");
    if( ! deserialize_versioned( node, std::string("1"), dval ) )  throw s11n_exception(S11N_SOURCEINFO,"");
    COUT << "deser'd versioned data (version " << v1 << ") == " << dval <<'\n';

#if 0
    std::string dval2("error");
    deserialize_versioned( node, std::string("angelbone"), dval2 ); // should throw
    COUT << "deser'd versioned data (version " << v1 << ") == " << dval <<'\n';
#endif


#if s11n_CONFIG_HAVE_CPP0X
    using namespace s11n::cpp0x;
    using namespace s11n;
    double vd = 42.423;
    int vi = 42;
    s11n_node vnode;
    if( ! serialize_versioned( vnode, 1.2, vi, vd ) ) throw s11n_exception(S11N_SOURCEINFO,"");
    s11nlite::save( vnode, std::cout );
    int vi2;
    vd = -1.0;
    if( ! deserialize_versioned( vnode, 1.20, vi2, vd ) ) throw s11n_exception(S11N_SOURCEINFO,"");
    s11nlite::save( vi2, std::cout );
    s11nlite::save( vd, std::cout );
#endif

}

#include <s11n.net/s11n/base64.hpp>

void do_bindata()
{
    using namespace s11nlite;
    using namespace s11n::base64;
    serializer_class( "simplexml" );
    node_type dest;
    char const * cData = "123456789aBC";
    size_t const cLen = strlen(cData);
    bindata_ser bin( cData, cLen );

    serialize( dest, bin );
    save( dest, std::cout );

    bindata_deser debin;
    deserialize( dest, debin );
    COUT << "debin.length = " << debin.length << '\n';
    if( debin.length )
    {
	COUT << "deser'd bin data:\n";
	std::copy( debin.data, debin.data + debin.length,
		   std::ostream_iterator<char>(std::cout) );
    }
    std::cout << '\n';
    bin = bindata_ser( bin.data, debin.length );
    save( bin, std::cout );
    bindata_deser::deallocate( debin );
}

#include <s11n.net/s11n/s11n_node_query.hpp>
void do_query()
{
#if 1
    using namespace s11nlite;
    typedef s11nlite::node_type NT;
    typedef s11nlite::node_traits NTR;
    
    NT n;

    typedef std::list<double> ListT;
    typedef std::map<int,ListT> MapT;

    MapT map;
    for( int i = 0; i < 3; ++i )
    {
        ListT & l = map['a'+i];
        for( int x = 0; x < 3; ++x )
        {
            l.push_back( x * i );
        }
    }
    s11nlite::serialize( n, map );
    s11nlite::save( n, std::cout );
    typedef s11n::node_query<NT> QT;
    QT q(&n);

#if 0
    q = q.copy()
        .children()
        .where( s11n::nq_name_is("pair") )
        .children()
        .where( s11n::nq_name_is("first") )
        //.where( s11n::nq_has_prop("second") )
        ;
#else
    q.children()
        .where( s11n::nq_name_is("pair") )
        .children()
        .where( s11n::nq_name_is("first") )
        //.where( s11n::nq_has_prop("second") )
        ;

#endif
    QT q2 = QT(&n)
        .children()
        .where( s11n::nq_name_is("pair") )
        .children()
        //.where( s11n::nq_not( s11n::nq_name_is("first") ) )
        //.where( s11n::nq_name_is("second") )
        //.where( s11n::nq_class_name_is("list" ) )
        //.where( s11n::nq_class_name_is( (ListT const *)0 ) )
        //.where( s11n::nq_class_name_is::class_name<ListT>() )
        .where( s11n::nq_or(
                            //s11n::nq_class_name_is::class_name<ListT>(),
                            s11n::nq_name_is("second"),
                            s11n::nq_class_name_is::class_name<int>()
                            ) )
        //.copy()
        //.children()
        .append(q)
        //.sort()
        //.where( s11n::nq_has_prop("v") )
        ;
    //q.intersect( q2 );
    q.onion( q2 );
    //q.append( q2 )
    //q = q2;
    //q.sort();
    q.unique();
    QT::iterator it = q.results().begin();
    QT::iterator et = q.results().end();
    size_t count = 1;
    COUT << "Matches:\n";
    for( ; et != it; ++it, ++count )
    {
        NT const * N = *it;
        COUT << "matching node #"<<count
             << ": @"<< N
             <<": " << NTR::name(*N)
             << ':'<<NTR::class_name(*N)<< '\n';
    }
#endif
}

int
main( int argc, char **argv )
{
//  	using namespace s11n::debug;
//  	trace_mask( trace_mask() | TRACE_CLEANUP );
        s11nlite::serializer_class( "parens" );
	try
	{
	    if(0) and_yet_MORE_TESTS();
	    if(0) do_some_tests();
	    if(0) do_some_more_tests();
	    if(0) do_even_more_tests();
	    if(0) do_version();
	    if(0) do_bindata();
	    if(1) do_query();
	}
	catch( const std::exception & ex )
	{
		CERR << "EXCEPTION: " << ex.what() << "\n";
		return 1;
	}
	catch( ... )
	{
		CERR << "UNKNOWN EXCEPTION!!!\n";
		return 2;
	}
        return 0;
}
