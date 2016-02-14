#ifndef s11n_S11N_H_INCLUDED
#define s11n_S11N_H_INCLUDED 1

////////////////////////////////////////////////////////////////////////
// s11n.hpp:
// Author: stephan beal <stephan@s11n.net>
// License: Public Domain
//
// This header includes "everything most clients would want" regarding
// the core s11n library. It does not include any i/o- or s11nlite-
// related headers.
//
// The s11n namespace docs go here so doxygen will pick them up nicely.
////////////////////////////////////////////////////////////////////////

#include <s11n.net/s11n/s11n_config.hpp> // configure/build-time config vars
#include <string>

/**
   The s11n serialization (s11n) framework is an object serialization
   framework for C++ modelled heavily off work by Rusty Ballinger
   (bozo@users.sourceforge.net http://libfunutil.sourceforge.net). 
   It provides a framework off of which a wide variety of C++ objects
   can be serialized and deserialized (saved and loaded) with a
   trivial amount of effort.

   As far as i know Rusty was the one to coin the phrase "s11n" (short
   for "serialization", in the same way that i18n is short for
   "internationalization").

   In addition to the API documentation, please see the library
   manual, available in the source tree, in the docs subdir.
   It is also available for download on our web site in other
   formats (i.e., not LyX).

   The most important concepts for clients to understand:

   - Data Node (a.k.a. S11n Node or SNode), as described in the
   library manual and demonstrated by the reference implementation,
   s11n::s11n_node.

   - the serialize() and deserialize() family of free functions.

   - Serializable type registration, as covered in the library manual.

   See the source tree, under <tt>src/client/sample</tt>, for
   many examples of client code.
*/
namespace s11n
{

        /**
           Returns the string form of the s11n library version.
        */
        std::string library_version();

        /**
           The Private namespace holds internal library types: these
           should not be used in client code. See also the Detail
           namespace, which is similar but subtly different.
        */
        namespace Private
        {
        }

        /**
           The s11n::io namespace defines some i/o-related types which
           conform to the conventions expected by the
           <code>s11n::de/serialize()</code> core functions. This
           namespace deals with the de/serialization of Data Nodes at
           the stream/file level, leaving the s11n core to only deal
           with de/serialization of containers.

           Clients can swap out these types and still use the core
           s11n interface. The core has no dependencies on this
           namespace. s11nlite combines the s11n core and i/o
           interfaces into a single, easy-to-use API, and users who
           don't <em>need</em> to directly act with the Serializers
           are strongly encouraged to stick to using s11nlite for
           their save/load needs.

           s11n's default Serializer implementations all live
           in the s11n::io namespace, and are derived from
           <code>serializer<NodeT></code>. Clients
           who subclass this type and follow the conventions
           laid out by the reference implementations can plug
           their own Serializers into the framework with very
           little effort. For an example of a plug-in Serializer
           see the <a href="http://s11n.net/mysql/">mysql_serializer</a>.
        */
        namespace io {
        }


	/**
	   The Detail namespace holds types which aren't strictly
	   internal/private, but are nonetheless considered to be
	   "implementation details." Unless specifically documented
	   otherwise, clients are discouraged from using the
	   Detail-namespace API from client code, as any part of it
	   may change significantly or be replaced/removed without any
	   warning.

	   Clients wishing to use Detail-namespace code are instead
	   encouraged to make their own copy, rename the namespace,
	   and tweak to suit.

	   The main difference between the Private and Detail
	   namespaces is... well, vague. As a "general guideline",
	   most Private code is specific to one certain area of this
	   library, where as Detail code might have utility in
	   throughout the library, or even play a fundamental role in
	   the implementation. An example of a Detail is the phoenix<>
	   class: it plays no direct part in serialization proper, and
	   thus was moved out of the top-level s11n namespace, but is
	   used throughout the framework to provide shared object
	   instances for many purposes.
	*/
	namespace Detail {
	}

}

#include <s11n.net/s11n/classload.hpp> // classloader API
#include <s11n.net/s11n/traits.hpp> // node_traits and s11n_traits
#include <s11n.net/s11n/exception.hpp> // exception types
#include <s11n.net/s11n/serialize.hpp> // serialize() and friends
#include <s11n.net/s11n/algo.hpp> // generic algos used by the framework
#include <s11n.net/s11n/s11n_node.hpp> // Reference Data Node implementation
#include <s11n.net/s11n/tags.hpp> // de/serialize operator tags

#endif // s11n_S11N_H_INCLUDED
