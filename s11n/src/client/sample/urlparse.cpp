////////////////////////////////////////////////////////////
// Test app for the URL parser code.
// Required linker arguments: -rdynamic -ls11n
#include <s11n.net/s11n/io/url.hpp>
#include <s11n.net/s11n/s11n_debuggering_macros.hpp> // CERR macro


void dump_url( s11n::io::url_parser const & u )
{
	COUT << "URL: " << u.url() << "\n"
		"scheme:\t\t" << u.scheme() << "\n"
		"user:\t\t" << u.user() << "\n"
		"password:\t\t" << u.password() << "\n"
		"host:\t\t" << u.host() << "\n"
		"port:\t\t" << u.port() << "\n"
		"path:\t\t" << u.path() << "\n"
		"args:\t\t" << u.args_str() << "\n"
		"args count:\t\t" << u.args_map().size() << "\n"
		;
	if( ! u.args_map().empty() )
	{
		typedef s11n::io::url_parser::args_map_type::const_iterator IT;
		IT it = u.args_map().begin();
		IT et = u.args_map().end();
		for( ; et != it; ++it )
		{
			COUT << "\t[" << (*it).first << "]=["<<(*it).second<<"]\n";
		}
	}

}

int
main( int argc, char **argv )
{
	if( 2 > argc )
	{
		CERR << "Usage: " << argv[0] << " url [url2..urlN]\n";
		return 0;
		// ^^^ we return 0 instead of 1 to keep 'make run' from failing.
	}
	s11n::io::url_parser u;
	for( int i = 1; i < argc; ++i )
	{
		if( ! u.parse(argv[i] ) )
		{
			CERR << "Parse failed:\n";
			dump_url( u );
			return i;
		}
		dump_url(u);
	}
        return 0;
}
