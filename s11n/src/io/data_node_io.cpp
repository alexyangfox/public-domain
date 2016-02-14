
#include <memory> // auto_ptr

#include <s11n.net/s11n/s11n_config.hpp>
#include <s11n.net/s11n/memory.hpp>

#if s11n_CONFIG_HAVE_ZFSTREAM
#  include <s11n.net/zfstream/zfstream.hpp>
#else
#  include <fstream>
#endif

#include <s11n.net/s11n/io/data_node_io.hpp>
#include <s11n.net/s11n/io/FlexLexer.hpp>
#include <s11n.net/s11n/io/url.hpp>

namespace s11n {
        namespace io {

		std::ostream * get_ostream( const std::string name )
		{
			std::ostream * os = 0;
			url_parser u(name);
			if( u.good() )
			{
				os = get_url_ostream( u );
			}
			if( ! os )
			{
#if s11n_CONFIG_HAVE_ZFSTREAM
				os = zfstream::get_ostream( name );
#else
				os = new std::ofstream( name.c_str() );
#endif
			}
			if( (!os) || (!os->good()) )
			{
				delete os;
				os = 0;
			}
			return os;
		}

		std::istream * get_istream( const std::string name )
		{
			std::istream * is = 0;
			url_parser u(name);
			if( u.good() )
			{
				is = get_url_istream( u );
			}
			if( ! is )
			{
#if s11n_CONFIG_HAVE_ZFSTREAM
				is = zfstream::get_istream( name );
#else
				is = new std::ifstream( name.c_str() );
#endif
			}
			if( (!is) || (!is->good()) )
			{
				delete is;
				is = 0;
			}
			return is;
		}

                namespace Private {

                        int lex_api_hider_yylex( FlexLexer * f, std::istream & is  )
                        {
                                f->switch_streams( & is );
                                int ret = 0;
                                while( 0 != (ret = f->yylex() ) );
                                return ret;
                        }
                }

                std::string get_magic_cookie( const std::string & src )
                {
                        if( src.empty() ) return src;
                        typedef std::auto_ptr<std::istream> AP;
                        AP is( get_istream( src ) );
                        return is.get()
				? get_magic_cookie( *is )
				: "";
                }

                std::string get_magic_cookie( std::istream & is )
                {
			if( ! is.good() ) return std::string();
                        std::string ret;
			// As of 1.2.1, instead of using getline(),
			// we constrain string to the first non-control
			// chars read. This allows us to pull cookies from
			// some binary formats (e.g. sqlite3).
			int ch;
			do
			{
				ch = is.get();
				if( (ch < 32 /* == SPACE */) 
				    || (ch > 126 /* == TILDE */)
				    || is.eof() /* shouldn't happen? */
				    )
				{
					break;
				}
				ret += ch;
			} while(true);
			// CERR << "get_magic_cookie() == [" << ret<<"]\n";
			return ret;
                }

        } // namespace io
} // namespace s11n
