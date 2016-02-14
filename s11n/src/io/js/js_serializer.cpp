
#include <s11n.net/s11n/s11n_node.hpp>
#include <s11n.net/s11n/io/serializers.hpp>
#include <s11n.net/s11n/io/js_serializer.hpp>

namespace s11n { namespace io {

	/**
	   Internal. Not part of the public API.

	   Parameters:

	   - ins = source string of the escapes

	   - to_esc = any chars in this list are considered worthy of escaping.

	   - esc = escape sequence to prepend to any char in to_esc.

	   Return value is the number of changes (escapes) made to ins.

	   e.g., to escape apostrophes with a preceding backslash:

	   js_escape_string( src, "\'", "\\" );

	   (Doxygen may ruin that escaping, so be sure to see these sources
	   if you want to be sure!)
	   
	*/
	size_t js_escape_string( std::string & ins, const std::string & to_esc, const std::string & esc )
	{
		std::string::size_type pos;
		pos = ins.find_first_of( to_esc );
		size_t reps = 0;
		while( pos != std::string::npos )
		{
			ins.insert( pos, esc );
			++reps;
			pos = ins.find_first_of( to_esc, pos + esc.size() + 1 );
		}
		return reps;
	}

	std::string quote_js_string( std::string const & s )
	{
		if( s.empty() || (std::string::npos == s.find('\'')) )
		{
			return "'"+s+"'";
		}
		else if( std::string::npos == s.find("\"") )
		{
			return "\""+s+"\"";
		}
		else
		{
			std::string x = s;
			js_escape_string( x, "\'", "\\" );// "Quote it all, let God sort it out!"
			return "'"+x+"'";
		}
	}


} } // namespace s11n::io

namespace {

        void js_serializer_registration_init()
        {

#define SERINST(NodeT)                                  \
                ::s11n::io::register_serializer< ::s11n::io::js_serializer< NodeT > >( "s11n::io::js_serializer", "js" );
                SERINST(s11n::s11n_node);
#undef SERINST
        }

        int js_reg_placeholder = 
                ( js_serializer_registration_init(), 1 );



} // anonymous namespace
