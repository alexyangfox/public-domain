#ifndef s11n_JS_SERIALIZER_HPP_INCLUDED
#define s11n_JS_SERIALIZER_HPP_INCLUDED 1

#include <s11n.net/s11n/traits.hpp> // node_traits

/**
MAGIC_COOKIE_JS defines the magic-cookie which prefixes each
file output by the js_serializer. If the magic cookie
associated with a Serializer changes, older versions of
the serializer will not be able to read the file. Since
js_serializer is write-only, this is not a specific problem
for this serializer, but it should not be changed without
careful consideration nonetheless.

Note that this macro is #undef'd at the end of this file,
and is therefore unavailable to client code.
*/
#define MAGIC_COOKIE_JS "// s11n::io::js_serializer"

#include <stdexcept>
#include <sstream>
namespace s11n {

        namespace io {

                namespace sharing {
                        /**
                           Sharing context used by expat_serializer.
                         */
                        struct js_sharing_context {};

                }

		/**
		   An attempt to quote s as a JS string:

		   If s contains no apostrophes, it is returned as 's', with
		   apostrophes as quotes.
		   Else, if it contains no double-quotes, "s" is
		   returned.  Else we backslash-escape the apostrophes
		   and return 's'.
		*/
		std::string quote_js_string( std::string const & s );

                /**
                   js_serializer writes objects to a Javascript dialect.
                */
                template <typename NodeType>
                class js_serializer : public data_node_serializer<NodeType>
                {
                public:
                        typedef NodeType node_type;

                        typedef js_serializer<node_type> this_type; // convenience typedef

                        js_serializer() : m_depth(0)
                        {
                                this->magic_cookie( MAGIC_COOKIE_JS );
                        }

                        virtual ~js_serializer() {}

                        /**
                           Writes src out to dest.
                        */
                        virtual bool serialize( const node_type & src, std::ostream & dest )
			{
				this->m_depth = 0;
				return this->serialize_impl( src, dest );
			}

                        /**
                           Throws an exception: this type doesn't
                           support reading from JS.
                        */
                        virtual node_type * deserialize( std::istream & src )
                        {
				throw ::s11n::s11n_exception( "js_serializer() does not support DEserialization." );
                                return 0; // avoid warning from compiler about implicit return
                        }
 
                private:
                        size_t m_depth;
                        bool serialize_impl( const node_type & src, std::ostream & dest )
                        {
                                typedef ::s11n::node_traits<node_type> NT;

// INDENT() is a helper macro for some serializers.
#define INDENT(LEVEL,ECHO) indent = ""; for( size_t i = 0; i < depth + LEVEL; i++ ) { indent += '\t'; if(ECHO) dest << '\t'; }

                                size_t depth = this->m_depth++;
                                if ( 0 == depth )
                                {
                                        dest << this->magic_cookie() << '\n';
                                }


                                std::string nname = NT::name(src);
                                std::string impl = NT::class_name(src);
                                std::string indent;
				dest << "(function() {\n";
				INDENT(1,0);
				dest << indent << "var self = new Object();\n";
				dest << indent << "self.$name = '" << nname << "';\n";
				dest << indent << "self.$class = '"<< impl <<"';\n";
                                typedef typename NT::property_map_type PMT;
				typedef typename PMT::const_iterator CHIT;
				CHIT cit, cet;
				cit = NT::properties(src).begin();
				cet = NT::properties(src).end();
                                std::string propval;
                                std::string key;


                                if( cet != cit )
                                { // got properties?
					dest << indent << "var p = self.$properties = new Array();\n";
// 					INDENT(2,0);
// 					size_t sz = NT::properties(src).size();
// 					size_t pos = 0;
                                        for ( ; cet != cit; ++cit )
                                        {
						dest << indent << "p["<<quote_js_string((*cit).first) <<"] = "
						     << quote_js_string( ( *cit ).second ) << ";\n";
						continue;
//                                                 key = ( *cit ).first;
//                                                 propval = quote_js_string( ( *cit ).second );
                                                //dest << indent << "$"<<key << ":" << propval;
						// ^^^ we prefix with $ to avoid collisions with JS reserved words.
						// e.g., void='abc' will not work, but $void='abc' will.
// 						if( pos++ < (sz-1) ) { dest << ','; }
// 						dest << '\n';
                                        }
// 					INDENT(1,0);
// 					dest << indent << "};\n";
                                }

				
				typedef typename NT::child_list_type CHLT;
                                typename CHLT::const_iterator chit = NT::children(src).begin(),
                                        chet = NT::children(src).end();
                                if( chet != chit )
                                { // got kids?
                                        INDENT(1,0);
					dest << indent << "self.$children = (function() {\n";
					node_type const * node = 0;
                                        INDENT(2,0);
					dest << indent << "var a = new Array();\n";
					size_t pos = 0;
					for( ; chet != chit; ++chit )
					{
						node = *chit;
						//dest << indent;
						dest << indent << "a["<<pos++<<"] = ";
						try
						{
							++m_depth;
							this->serialize_impl( *node, dest );
							--m_depth;
						}
						catch(...)
						{
							--m_depth;
							throw;
						}
					}
					dest << indent << "return a;\n";
					INDENT(1,0);
					dest << indent << "})();\n"; // end children
                                }
 				INDENT(1,0);
				dest << indent << "return self;\n";
				//if( 0 == depth ) { INDENT(1,0); }
				//dest << indent << "};\n";
				INDENT(((0==depth) ? 0 : 1),0);
				dest << indent << "})();\n"; // close and call function
                                if( 0 == depth )
                                {
					dest << "\n";
                                        dest.flush();
                                        // if we don't do this then the client is possibly forced to flush() the stream :/
                                }
                                --this->m_depth;
                                return true;
#undef INDENT
                        }
			
                };

        } // namespace io
} // namespace s11n

#undef MAGIC_COOKIE_JS
#endif // s11n_JS_SERIALIZER_HPP_INCLUDED
