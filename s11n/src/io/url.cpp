#include <sstream>
#include <fstream>
#include <stdexcept>

#include <s11n.net/s11n/s11n_config.hpp>
#include <s11n.net/s11n/memory.hpp>
#include <s11n.net/s11n/io/url.hpp>
#include <s11n.net/s11n/io/strtool.hpp>
#include <s11n.net/s11n/s11n_debuggering_macros.hpp>

#include <s11n.net/s11n/classload.hpp>
#include <s11n.net/s11n/export.hpp> // unfortunate kludge

#if s11n_CONFIG_HAVE_ZFSTREAM
#include <s11n.net/zfstream/zfstream.hpp>
#endif

namespace s11n { namespace io {

	url_parser::impl::impl()
	{
		good = false;
		port = 0;
	}


	url_parser::url_parser( std::string const & _url )
		: pimpl( 0 )
	{
		this->parse(_url);
	}
	url_parser::url_parser()
		: pimpl( new impl )
	{
		// reminder: we use 'new impl' here to ensure that funcs
		// like good() and port() work before parse() is called.
	}

	url_parser::~url_parser()
	{
	}
	url_parser & url_parser::operator=( std::string const & _url )
	{
		this->parse(_url);
		return *this;
	}

	bool url_parser::good() const
	{
		return this->pimpl->good;
	}

	static size_t parse_args( std::string const & in, url_parser::args_map_type & out )
	{
		size_t ret = 0;

		s11n::io::strtool::stdstring_tokenizer toker;
		std::string tok;
		std::string val;
		toker.tokenize( in, "&" );
		std::string::size_type pos;
		while( toker.has_tokens() )
		{
			tok = toker.next_token();
			pos = tok.find( '=' );
			if( std::string::npos != pos )
			{
				val = (pos == (tok.size()-1))
					? std::string()
					: tok.substr( pos+1 );
				tok = tok.substr( 0, pos );
				out[tok] = val;
			}			
			else
			{
				out[tok] = "";
			}
		}
		return ret;
	}

	bool url_parser::parse( std::string const & _url )
	{
		// TODO: reimplement this as a recursive-descent parser.
		// It certainly has some corner cases which are not properly
		// handled.
		if( ! this->pimpl.get() )
		{
			this->pimpl.take( new impl );
		}
		else
		{ // copy on write
			this->pimpl.take( new impl );
		}
		this->pimpl->url = _url;
		if( _url.empty() ) return false;

		const std::string::size_type usz = _url.size();
		std::string::size_type p1, p2,p3;

		/**
		   Treat file:// specially, to allow for a missing //
		   after the scheme, as this is commonly accepted in
		   many apps.
		*/
		if( 0 == _url.find("file:") )
		{
			this->pimpl->proto = "file";
			if( std::string::npos != (p1 = _url.find("://")) )
			{
				this->pimpl->path = _url.substr( p1+3 );
			}
			else
			{
				this->pimpl->path = _url.substr( 5 );
			}
			return this->pimpl->good = !pimpl->path.empty();
		}

		p1 = _url.find( "://" );
		p3 = p1+3;
		if( (std::string::npos == p1)
		    || (p3 >= usz)
		    )
		{
			return false;
		}
		this->pimpl->proto = _url.substr( 0, p1 );

		char ch = _url[p3];
		if( ('/' == ch) || ('.' == ch) )
		{ // if it has a leading slash or dot, assume it is a local path
			this->pimpl->path = _url.substr( p3 );
			return this->pimpl->good = true;
		}

		p2 = _url.find('@', p3 );
		if( std::string::npos != p2 )
		{ // user[:password]@host
			std::string up = _url.substr( p3, p2 - p3);
			std::string::size_type p4 = up.find(':');
			if( std::string::npos == p4 )
			{
				if( up.size()-1 == p4 )
				{ // trailing semicolon
					this->pimpl->user = up.substr(0,up.size()-1);
				}
				else
				{
					this->pimpl->user = up;
				}
			}
			else
			{
				this->pimpl->user = up.substr(0,p4);
				if( up.size() <= (p4+1)  )
				{
					// nothing: empty password.
				}
				else
				{
					this->pimpl->pass = up.substr(p4+1);
				}
			}
			p2 += 1;
			p3 = p2;
		}
		else
		{
			p2 = p3;
		}

		std::string const hostchars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_-.");
		p1 = p2;
		p2 = _url.find_first_not_of( hostchars, p2 );
		if( std::string::npos == p2 )
		{
			if( p1 != p2 )
			{
				this->pimpl->host = _url.substr( p1, p2 - p1 );
				return this->pimpl->good = true;
			}
			return false;
		}
// 		CERR << "p1=="<<p1<<", p2=="<<p2<<", p3=="<<p3<<'\n'
// 		     << _url << '\n'
// 		     << "0123456789+123456789+123456789+123456789+123456789+123456789+123456789";
		this->pimpl->host = _url.substr( p3, p2 - p3 );
		p1 = p2;
		if( (usz<=p2) )
		{
			return this->pimpl->good = true; // assume scheme handles empty requests
		}
 		if( ':' == _url[p1] )
 		{ // possibly a port number or ssh-like delimiter
 			p1 += 1;
			std::string num;
			if( p1 < usz )
			{
				char ch;
				while( (p1 < usz)
				       && (std::isdigit(ch=_url[p1])) )
				{
					num += ch;
					++p1;
				}
			}
			if( ! num.empty() )
			{
				std::istringstream ns( num );
 				if( ! (ns >> this->pimpl->port) )
 				{
 					return false;
 				}
			}
 		}

		// Workaround to allow for ssh-style URLs to include
		// a port number.
		p2 = _url.find_first_not_of(":", p1 );

		this->pimpl->path = _url.substr( p2 );
		p1 = this->pimpl->path.find('?');
		if( std::string::npos != p1 )
		{
			this->pimpl->args_str =
				(p1 == (this->pimpl->path.size() -1))
				? std::string()
				: this->pimpl->path.substr( p1+1 );
			this->pimpl->path = this->pimpl->path.substr( 0, p1 );
			if( ! this->pimpl->args_str.empty() )
			{
				parse_args( this->pimpl->args_str, this->pimpl->args_map );
			}

		}
		return this->pimpl->good = true;
	}

	std::string url_parser::url() const
	{
		return this->pimpl->url;
	}
	std::string url_parser::scheme() const
	{
		return this->pimpl->proto;
	}
	std::string url_parser::host() const
	{
		return this->pimpl->host;
	}
	std::string url_parser::user() const
	{
		return this->pimpl->user;
	}
	std::string url_parser::password() const
	{
		return this->pimpl->pass;
	}
	std::string url_parser::path() const
	{
		return this->pimpl->path;
	}

	std::string url_parser::args_str() const
	{
		return this->pimpl->args_str;
	}

	url_parser::args_map_type const & url_parser::args_map() const
	{
		return this->pimpl->args_map;
	}

	unsigned short url_parser::port() const
	{
		return this->pimpl->port;
	}

	url_stream_factory *
	url_stream_factory::create_factory_for_scheme( std::string const & scheme )
	{
		return ::s11n::fac::create<url_stream_factory>( scheme );
	}



	std::istream * get_url_istream( std::string const & url )
	{
		return get_url_istream( url_parser(url) );
	}
	std::istream * get_url_istream( url_parser const & u )
	{
		if( ! u.good() ) return 0;
		s11n::Detail::auto_ptr<url_stream_factory> fac(  url_stream_factory::create_factory_for_scheme( u.scheme() ) );
		if( ! fac.get() )
		{
			return 0;
		}
		s11n::Detail::auto_ptr<std::istream> st( fac->get_istream( u ) );
		if( (! st.get()) || (!st->good()))
		{
			return 0;
		}
		return st.release();
	}

	std::ostream * get_url_ostream( std::string const & url )
	{
		return get_url_ostream( url_parser(url) );
	}

	std::ostream * get_url_ostream( url_parser const & u )
	{
		if( ! u.good() ) return 0;
		s11n::Detail::auto_ptr<url_stream_factory> fac(  url_stream_factory::create_factory_for_scheme( u.scheme() ) );
		if( ! fac.get() )
		{
			return 0;
		}
		s11n::Detail::auto_ptr<std::ostream> st( fac->get_ostream( u ) );
		if( (! st.get()) || (!st->good()))
		{
			return 0;
		}
		return st.release();
	}

	file_stream_factory::~file_stream_factory()
	{
	}

	file_stream_factory::file_stream_factory() : url_stream_factory()
	{
	}

	std::ostream *
	file_stream_factory::do_get_ostream( url_parser const & url ) const
	{
		if( ! url.good() ) return 0;
		//CERR << "file:// do_get_ostream("<<url.url()<<" ==> " << url.path() << ")\n";
		std::ostream * os = 0;
		std::string name( url.path() );
#if s11n_CONFIG_HAVE_ZFSTREAM
		os = zfstream::get_ostream( name );
#else
		os = new std::ofstream( name.c_str() );
		if( ! os->good() )
		{
			delete os;
			os = 0;
		}
#endif
		return os;
	}

	std::istream *
	file_stream_factory::do_get_istream( url_parser const & url ) const
	{
		if( ! url.good() ) return 0;
		//CERR << "file:// do_get_istream("<<url.url()<<" ==> " << url.path() << ")\n";
		std::istream * is = 0;
		std::string name( url.path() );
#if s11n_CONFIG_HAVE_ZFSTREAM
		is = zfstream::get_istream( name );
#else
		is = new std::ifstream( name.c_str() );
		if( ! is->good() )
		{
			delete is;
			is = 0;
		}
#endif
		return is;
	}



	void url_static_init()
	{
		/******
		       Set up some classloader aliases...
		 ******/
		typedef s11n::fac::factory_mgr<url_stream_factory> FacT;
		FacT::instance().alias( "file", "s11n::io::file_stream_factory" );
	}

	bool url_static_init_placeholder = (url_static_init(),true);

}} // namespaces

/************************************************************************
Now we register the various handlers...
************************************************************************/
#define S11N_FACREG_TYPE s11n::io::url_stream_factory
#define S11N_FACREG_TYPE_NAME "s11n::io::url_stream_factory"
#define S11N_FACREG_TYPE_IS_ABSTRACT /* It's not abstract, really, but
					it can't be instantiated
					directly, so we treat it as if
					it were abstract. */
#include <s11n.net/s11n/factory_reg.hpp>

#define S11N_FACREG_TYPE s11n::io::file_stream_factory
#define S11N_FACREG_INTERFACE_TYPE s11n::io::url_stream_factory
#define S11N_FACREG_TYPE_NAME "s11n::io::file_stream_factory"
#include <s11n.net/s11n/factory_reg.hpp>
