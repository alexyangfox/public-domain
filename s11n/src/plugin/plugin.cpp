#include <s11n.net/s11n/plugin/plugin.hpp>
#include <s11n.net/s11n/plugin/plugin_config.hpp>

#include <iostream>
#ifndef CERR
#define CERR std::cerr << __FILE__ << ":" << std::dec << __LINE__ << " : "
#endif

#if defined(WIN32)
#    define PLUGIN_USE_NOOP 0
#else
#  if s11n_CONFIG_HAVE_LIBLTDL || s11n_CONFIG_HAVE_LIBDL
#    define PLUGIN_USE_NOOP 0
#  else
#    define PLUGIN_USE_NOOP 1
#  endif // libdl/ltdl?
#endif // WIN32?

namespace s11n { namespace plugin {

// 	/** Internal marker type. */
// 	struct default_path_context {};

 	struct path_initer
 	{
 		void operator()( path_finder & p )
 		{
 			p.add_path( s11n_CONFIG_PLUGINS_PATH );
 			p.add_extension( s11n_CONFIG_DLL_EXTENSION );
 		}
 	};

	path_finder & path()
	{
		//return ::s11n::Detail::phoenix<path_finder,default_path_context,path_initer>::instance();
		static path_finder bob;
		static bool donethat = false;
		if( (!donethat) && (donethat=true) )
		{
 			path_initer()( bob );
		}
		return bob;
	}

	std::string find( const std::string & basename )
	{
		// CERR << "find("<<basename<<")... path="<<path().path_string()<<"\nextensions="<<path().extensions_string()<<"\n";
		return path().find( basename );
	}

	static std::string m_dll_error; // internal holder for value returned by dll_error().

        std::string dll_error()
        {
		if( m_dll_error.empty() ) return m_dll_error;
		std::string ret = m_dll_error;
		m_dll_error = std::string();
		return ret;
        }

	/**
	   Internal, used by native open() functions to set the error
	   string.
	*/
        void dll_error( std::string const & s )
        {
		m_dll_error = s;
        }

#if PLUGIN_USE_NOOP
	std::string open( const std::string & basename )
	{
		dll_error( std::string("s11n::plugin::open(")
			   + basename
			   + std::string( "): not implemented on this platform." )
			   );
		return std::string();
	}
#endif // PLUGIN_USE_NOOP


} } // namespace

#if ! PLUGIN_USE_NOOP
#    if s11n_CONFIG_HAVE_LIBLTDL || s11n_CONFIG_HAVE_LIBDL
#      include "plugin.dl.cpp"
#    elif defined(WIN32)
#      include "plugin.win32.cpp"
#    endif
#endif // WIN32?

#undef PLUGIN_USE_NOOP
