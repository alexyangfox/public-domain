#ifndef s11n_MEMORY_TPP_INCLUDED
#define s11n_MEMORY_TPP_INCLUDED
// this is the implementation file for the code declared in serialize.hpp


	/**
	********************************************************************
	General API Conventions:

	NodeType should conform to the conventions laid out
	by s11n::s11n_node.

	SerializableTypes/BaseTypes:

	- BaseT must have the following in its interface:

	- bool SerializeFunction( NodeType & dest ) const;

	- bool DeserializeFunction( const NodeType & dest );

	SerializeFunction/DeserializeFunction need not be virtual,
	though they may be.

	Proxy functors:

	Serialization functor must have:

	bool operator()( NodeType & dest, const BaseT & src ) const;

	Deserialization functor must have:

	bool operator()( const NodeType & src, BaseT & dest ) const;

	They may be the same functor type - const resolution will
	determine which s11n uses. Sometimes this might cause an
	ambiguity, and may require 2 functors.

	These signatures apply for all functors designed to work as
	de/serialization proxies.
	*********************************************************************
	*/


#include <s11n.net/s11n/s11n_debuggering_macros.hpp> // COUT/CERR
#include <s11n.net/s11n/traits.hpp> // s11n_traits
#include <s11n.net/s11n/exception.hpp> // s11n_exception and friends

////////////////////////////////////////////////////////////////////////////////
// NO DEPS ON s11n_node.hpp ALLOWED!
////////////////////////////////////////////////////////////////////////////////


template <typename SerializableType>
void s11n::cleanup_serializable( SerializableType & s ) throw()
{
	try
	{
		typename s11n::s11n_traits<SerializableType>::cleanup_functor cf;
		cf(s);
	}
	catch(...)
	{
		using namespace ::s11n::debug;
		S11N_TRACE(TRACE_ERROR) << "Exception thrown during cleanup! INGORING IT! This might mean a mem leak has occurred!\n";
	}
}

template <typename SerializableType>
void s11n::cleanup_serializable( SerializableType * & s ) throw()
{
	using namespace ::s11n::debug;
	S11N_TRACE(TRACE_CLEANUP) << "cleanup_serializable(*&): @ " << std::hex << s << ", s11n_class="<<s11n_traits<SerializableType>::class_name(s)<<"\n";
	if( s )
	{
		s11n::cleanup_serializable<SerializableType>( *s );
		delete s;
		s = 0;
	}
}

#endif // s11n_MEMORY_TPP_INCLUDED
