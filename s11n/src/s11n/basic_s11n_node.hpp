#ifndef s11n_BASIC_S11N_NODE_HPP_INCLUDED
#define s11n_BASIC_S11N_NODE_HPP_INCLUDED

////////////////////////////////////////////////////////////////////////
// basic_s11n_node.hpp
// A reference implementation for s11n's Data Node concept.
// License: Public Domain
// Author: stephan@s11n.net
////////////////////////////////////////////////////////////////////////
#include <string>
#include <map>

#include <vector>
#include <sstream> // for lexical casting
#include <s11n.net/s11n/export.hpp>
#include <s11n.net/s11n/algo.hpp> // Detail::child_pointer_deep_copier

namespace s11n {

	/**
	   basic_s11n_node is a slightly lighter-weight replacement for
	   the data_node type used in s11n 1.0.x. It became
	   the standard node type for s11nlite in 1.1/1.2.

	   In 1.3.0 it was expanded add support for properties
	   containing wide characters. The name and class_name fields,
	   as well as property keys, are explicitly narrow-char
	   strings because:

	   - If they use variable char types then they cannot interact
	   properly with the classloader layer, and making that layer
	   support variable char types is not realistic.

	   - The name field and property keys must conventionally be
	   usable as XML tag names, so there is no benefit to making
	   the node names support wide charts.

	   Note that as of 1.3.x, none of the i/o handlers
	   (Serializers) can handle wide chars, so all of the wide
	   char support is essentially untested.
        */
	template <typename CharT = char>
        class S11N_EXPORT_API basic_s11n_node
        {
        public:
		/**
		   The CharT parameter used for this instantiation.
		*/
		typedef CharT char_type;

		/**
		   The string type used to store property values.
		*/
		typedef std::basic_string<char_type> val_string_type;

                /**
                   The map type this object uses to store properties.
                 */
		typedef std::map < std::string, val_string_type > map_type;

                 /**
                    A pair type used to store key/value properties
                    internally.
                 */
 		typedef typename map_type::value_type value_type;

                 /** The type used to store property keys. For
                     compatibility with std::map.
                 */
                 typedef typename map_type::key_type key_type; 

                 /** The type used to internally store property
                     values. For compatibility with std::map.
                 */
                 typedef typename map_type::mapped_type mapped_type;

                /**
                   The container type used to store this object's children.
                   It contains (basic_s11n_node *).

                   While the exact container type is not guaranteed,
                   it is guaranteed to obey the most-commonly-used
                   std::list/vector conventions: push_back(), erase(),
                   etc. It is not guaranteed to provide random access
		   iterators.
                */
		typedef std::vector<basic_s11n_node *> child_list_type;


                /**
                   Creates a new node with a default name() and
                   class_name(). Users of this node should re-set the
                   name and class_name, as the default values are not
                   guaranteed to stay the same between libs11n
                   releases, and relying on the defaults may lead to
                   unreadable data. Empty names and class_names are
                   illegal and are not supported by any i/o handlers
                */
                basic_s11n_node();

                /**
                   Creates a new node with the given name() and an
                   class_name() of "s11n_node".
                */
		explicit basic_s11n_node( const std::string & name );

                /**
                   Creates a new node with the given name() and and
                   class_name().

                   Does not throw unless the string copy ctor throws.
                */
                basic_s11n_node( const std::string & name,
				 const std::string implclass );

                /**
                   Destroys all child objects owned by this object, freeing up
                   their resources.

                   Does not throw.
                */
                ~basic_s11n_node();

		/**
		   Swaps all publically-visible internal state with
		   rhs. This includes:

		   - class_name()
		   - name()
		   - children()
		   - properties()

		   Complexity is, in theory, constant time.  For all
		   data we use their member swap() implementations,
		   which should be constant-time for the
		   containers. The C++ Standard apparently guarantees
		   O(1) swap() for strings, too.  (Josuttis, The C++
		   Standard Library, section 11.2.8, page 490.)

		   Added in version 1.1.3.
		*/
		void swap( basic_s11n_node & rhs );

                /**
                   Copies the properties, name, class name and
                   children of rhs. If rhs is this object then this
                   function does nothing.

                   Does not throw.
                */
                basic_s11n_node & operator=( const basic_s11n_node & rhs );

                /**
                   See copy().

                   Does not throw.
                */
                basic_s11n_node( const basic_s11n_node & rhs );


		/**
                   Returns a list of the basic_s11n_node children of this
                   object. The caller should not delete any pointers
                   from this list unless he also removes the pointers
                   from the list, or else they will get double-deleted
                   later. In practice it is (almost) never necessary
                   for client code to manipulate this list directly.
                */
                child_list_type & children();


                /**
                   The const form of children().
                 */
                const child_list_type & children() const;


                /**
                   Removes all properties and deletes all children from
                   this object, freeing up their resources.
                   
                   Any pointers to children of this object become
                   invalided by a call to this function (they get
                   deleted).
                */
                void clear();

                /**
                   Defines the class name which should be used as the
                   implementation class when this node is
                   deserialize()d.

                   Client Serializable types should call this one time
                   from their serialize() method, <em>after</em> calling
                   the parent class' serialize() method (if indeed that
                   is called at all), passing it the name of their class,
                   <em>using the name expected by the classloader</em>. By convention
                   the class name is the same as it's C++ name, thus Serializable
                   class foo::FooBar should call:

<pre>
node.class_name( "foo::FooBar" );
</pre>

		  from it's serialize() function.


                  If classes to not set this then the serialized data
                  will not have a proper implementation class
                  name. That is likely to break deserialization.

		  TODO: consider returning the old value,
		  to simplify swap() operations.

		  Added in 1.1.3.
                */
		void class_name( const std::string & n );


                /**
                   Returns the implementation class name set via
                   class_name().
                */
                std::string class_name() const;


		/**
                   The name which should be used as the key for
                   storing the node. This is normally translated to
                   something like an XML element name (e.g., &lt;name&gt;),
                   and should not contain spaces or other characters
                   which may not be usable as key names. To be safe,
                   stick to alphanumeric and underscores, starting
                   with a letter or underscore. (This class does no
                   enforce any naming conventions, but your data file
                   parsers very well may.)
                */
		void name( const std::string & n );

		/**
                   Returns this node's name, as set via name(string).
                */
		std::string name() const;

		/**
		   Returns true if this object has no properties
		   and no children. The name() and class_name()
		   are *not* considered.
		*/
		bool empty() const;


		/**
                   Lexically casts val to a string and stores it as a
                   property. If this type conversion is not possible
                   it will fail at compile time. A value-conversion
                   failure, on the other hand, is not caught at
                   compile time. T must support complementary
                   ostream<< and istream>> operators.
                 */
                template < typename T >
                void set( const std::string & key, const T & val );

		/**
                   Tries to get a property named key and lexically
                   cast it to type T. If this type conversion is not
                   possible it will fail at compile time. A
                   value-conversion failure, on the other hand, is not
                   caught at compile time. If value conversion fails,
                   or if the requested property is not set, then
                   defaultval is returned. This can be interpretted as
                   an error value if the client so chooses, and it is
                   often helpful to pass a known-invalid value here
                   for that purpose.

                 */
		template < typename T >
                T get( const std::string & key, const T & defaultval ) const;

                /**
                   Returns true if this object contains the given
                   property, else false.
                */
		bool is_set( const std::string & key ) const;

                /**
                   Removes the given property from this object.
                */
		void unset( const std::string & key );

		/**
		   Returns the map of properties contained by this
		   node.
		*/
                map_type & properties();

		/** Const overload. */
                const map_type & properties() const;



        private:
		std::string m_name; // name of this node
		std::string m_implclass; // class_name name of this node
		map_type m_map; // stores key/value properties.
		child_list_type m_children; // holds child pointers
                /**
                   Copies all properties and child basic_s11n_nodes from
                   rhs into this object, as well as any other details
                   which need to be copied.

                   This can be a very expensive operation, and is rarely
                   necessary.
                */
                void copy( const basic_s11n_node & rhs );
		/**
                   Removes all property entries from this object.
                 */
		void clear_properties();

		/**
                   Removes all children from this object, deleting all
                   child pointers (which recursively destroyed all
		   sub-children).
                 */
                void clear_children();

	}; // class basic_s11n_node

} // namespace s11n


#include "basic_s11n_node.tpp"

#endif // s11n_BASIC_S11N_NODE_HPP_INCLUDED
