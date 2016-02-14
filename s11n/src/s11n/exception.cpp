#include <s11n.net/s11n/exception.hpp>
#include <s11n.net/s11n/s11n_config.hpp>
#include <sstream>

#include <stdarg.h> // va_list
#include <stdio.h> // vsnprintf()
#include <vector>
// #include <s11n.net/s11n/s11n_debuggering_macros.hpp> // CERR
// #include <iostream>
#include "vappendf.hpp"

namespace s11n {

	namespace {
		/**
		   s11n_CONFIG_EXCEPTION_BUFFER_SIZE sets the maximum size of expanded format strings
		   for s11n_exception() and format_string().
		*/
		static const int s11n_CONFIG_EXCEPTION_BUFFER_SIZE = (1024*4);
	}


	/**
	   Internal implementation for source_info data.  To be
	   honest, i don't remember why it was implemented this way,
	   instead of just adding these 3 members to source_info. :?
	*/
	struct source_info::pimpl
	{
		unsigned int line;
		char const * file;
		char const * func;
	};

	source_info & source_info::operator=( source_info const & rhs )
	{
		if( this != &rhs )
		{
			*this->m_p = *rhs.m_p;
		}
		return *this;
	}

	source_info::source_info( source_info const & rhs )
		: m_p( new pimpl )
	{
		*this->m_p = *rhs.m_p;
	}

	source_info::source_info( char const * file, unsigned int line, char const * func )
		: m_p(new pimpl)
	{
		static char const * unk = "<unknown_function>()";
		this->m_p->file = file ? file : unk;
		this->m_p->line = line;
		this->m_p->func = func ? func : unk;
	}

	source_info::~source_info()
	{
		delete this->m_p;
	}

	unsigned int source_info::line() const throw()
	{
		return this->m_p->line;
	}

	char const * source_info::file() const throw()
	{
		return this->m_p->file;
	}

	char const * source_info::func() const throw()
	{
		return this->m_p->func;
	}


	std::ostream & operator<<( std::ostream & os, source_info const & si )
	{
		return os << si.file()<<':'<<si.line()<<':'<<si.func();
	}


	s11n_exception::s11n_exception() : m_what()
	{
	}

	void s11n_exception::what( std::string const & w ) throw()
	{
		this->m_what = w;
	}

	const char * s11n_exception::what() const throw()
	{
		return this->m_what.empty()
			? ""
			: this->m_what.c_str();
	}



	std::string format_string( int buffsize,
				   const char *format,
				   va_list vargs ) throw()
	{
		if( buffsize <= 0 )
		{
			buffsize = s11n_CONFIG_EXCEPTION_BUFFER_SIZE;
		}
		// Pedantic note: this code was taken from SpiderApe,
		// which is released under the Mozilla Public
		// License/LPGL. However, i wrote that code, so i may
		// re-use it here (in the Public Domain) however i
		// damned well please. :)
		if( (! format) || (buffsize<=0) )
		{
			return std::string();
		}
		std::vector<char> buffer(buffsize,'\0');
		int size = vsnprintf( &buffer[0], buffsize, format, vargs);
		if( size <= 0 )
		{
			return std::string();
		}
		if (size > (buffsize-1))
		{
			size = buffsize-1;
			int i = buffsize-4;
			if( i < 0 ) i = 0;
			for( ; i < buffsize-1; ++i )
			{
				buffer[i] = '.';
			}
		}
		buffer[size] = '\0';
		return std::string(&buffer[0]); //don't use (buffer.begin(), buffer.end()) b/c nulls interfere!!!
	}

	std::string format_string( int buffsize,
				   const char *format,
				   ... ) throw()
	{
		va_list vargs;
		va_start( vargs, format );
	        std::string ret( format_string( buffsize, format, vargs ) );
		va_end(vargs);
		return ret;
	}

    std::string format_string( source_info const si, 
			       int buffsize,
			       const char *format,
			       va_list vargs ) throw()
    {
	std::string reg = format_string( buffsize, format, vargs );
	std::string pre = format_string( s11n_CONFIG_EXCEPTION_BUFFER_SIZE /* buffsize??? */,
					 "%s:%d:%s: ",
					 si.file(),
					 si.line(),
					 si.func() );
	return pre + reg;
    }


	/**
	   A convenience overload which prefixes si's
	   file/line/function information to the string.
	*/
	std::string format_string(  source_info const si, 
				    int buffsize,
				    const char *format,
				    ... ) throw()
	{
		va_list vargs;
		va_start( vargs, format );
	        std::string ret( format_string( si, buffsize, format, vargs ) );
		va_end(vargs);
		return ret;
	}

    std::string format_string( const char *format,
			       va_list vargs ) throw()
    {
	if( ! format ) return std::string();
	std::ostringstream os;
	vappendf( os, format, vargs );
	return os.str();
    }
    std::string format_string( const char *format,
			       ... ) throw()
    {
	va_list vargs;
	va_start( vargs, format );
	std::string ret( format_string( format, vargs ) );
	va_end(vargs);
	return ret;
    }

    std::string format_string( source_info const si, 
			       const char *format,
			       va_list vargs ) throw()
    {
	std::string reg = format_string( format, vargs );
	std::string pre = format_string( "%s:%d:%s: ",
					 si.file(),
					 si.line(),
					 si.func() );
	return pre + reg;
    }


	std::string format_string(  source_info const si, 
				    const char *format,
				    ... ) throw()
	{
		va_list vargs;
		va_start( vargs, format );
	        std::string ret( format_string( si, format, vargs ) );
		va_end(vargs);
		return ret;
	}



	s11n_exception::s11n_exception( const char *format, ... )
		: m_what()
	{
		va_list vargs;
		va_start( vargs, format );
		this->what( format_string(format, vargs) );
		va_end(vargs);
	}

	factory_exception::factory_exception( const char *format, ... )
	{
		va_list vargs;
		va_start( vargs, format );
		this->what( format_string(format, vargs) );
		va_end(vargs);
	}
	io_exception::io_exception( const char *format, ... )
	{
		va_list vargs;
		va_start( vargs, format );
		this->what( format_string(format, vargs) );
		va_end(vargs);
	}

	s11n_exception::s11n_exception( source_info const & si,
					const char *format,
					... )
		: m_what()
	{
		va_list vargs;
		va_start(vargs,format);
		std::string ret = format_string( si, format, vargs );
		va_end(vargs);
		this->what( ret );
	}

    s11n_exception::s11n_exception( source_info const & si )
	: m_what(format_string(si,""))
    {
    }

} // namespace s11n
