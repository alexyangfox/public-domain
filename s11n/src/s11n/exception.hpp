#ifndef s11n_net_s11n_v1_3_EXCEPTION_HPP_INCLUDED
#define s11n_net_s11n_v1_3_EXCEPTION_HPP_INCLUDED 1

#include <string>
#include <exception>
#include <stdexcept>
#include <iostream> // i hate this dependency :(
#include <s11n.net/s11n/s11n_config.hpp>

// S11N_CURRENT_FUNCTION is basically the same as the conventional
// __FUNCTION__ macro, except that it tries to use compiler-specific
// "pretty-formatted" names. This macro is primarily intended for use
// with the s11n::source_info type to provide useful information in
// exception strings.  The idea of S11N_CURRENT_FUNCTION and
// S11N_SOURCEINFO is taken from the P::Classes and Pt frameworks.
#ifdef __GNUC__
#  define S11N_CURRENT_FUNCTION __PRETTY_FUNCTION__
#elif defined(__BORLANDC__)
#  define S11N_CURRENT_FUNCTION __FUNC__
#elif defined(_MSC_VER)
#  if _MSC_VER >= 1300 // .NET 2003
#    define S11N_CURRENT_FUNCTION __FUNCDNAME__
#  else
#    define S11N_CURRENT_FUNCTION __FUNCTION__
#  endif
#else
#  define S11N_CURRENT_FUNCTION __FUNCTION__
#endif


// S11N_SOURCEINFO is intended to simplify the instantiation of
// s11n::source_info objects, which are supposed to be passed values
// which are easiest to get via macros.
#define S11N_SOURCEINFO s11n::source_info(__FILE__,__LINE__,S11N_CURRENT_FUNCTION)
namespace s11n {


	/**
	   source_info simplifies the collection of source file
	   information for purposes of wrapping the info into
	   exception strings.
	   
	   This class is normally not instantiated directly,
	   but is instead created using the S11N_SOURCEINFO
	   macro.
	   
	   Added in version 1.3.0.
	*/
	struct source_info
	{
		/**
		   It is expected that this function be passed
		   __FILE__, __LINE__, and
		   S11N_CURRENT_FUNCTION. If file or func are
		   null then "<unknown>" (or something
		   similar) is used.
		*/
		source_info( char const * file, unsigned int line, char const * func );
		~source_info();
		/**
		   Returns the line number passed to the ctor.
		*/
		unsigned int line() const throw();
		/**
		   Returns the file name passed to the ctor.
		*/
		char const * file() const throw();
		/**
		   Returns the function name passed to the ctor.
		*/
		char const * func() const throw();
		
		/** Copies rhs. */
		source_info & operator=( source_info const & rhs );
		/** Copies rhs. */
		source_info( source_info const & rhs );
	private:
		struct pimpl; // opaque internal type
		pimpl * m_p;
	};

	/**
	   Sends si.file():si.line():si.func() to os and returns os.
	*/
	std::ostream & operator<<( std::ostream & os, source_info const & si );

	/**
	   The base-most exception type used by s11n.
	*/
        struct s11n_exception : public std::exception
        {
	public:
		virtual ~s11n_exception() throw() {}

		/**
		   Creates an exception with the given formatted
		   what() string.  Takes a printf-like format
		   string. If the expanded string is longer than some
		   arbitrarily-chosen internal limit [hint: probably
		   2k bytes] then it is truncated.

 		   If you get overload ambiguities with the
 		   std::string-argument ctor, this is because you've
 		   passed a (char const *) string to those ctors and
 		   relied on implicit conversion to std::string.
 		   Simply wrapping those c-strings in std::string
 		   ctors should get around the problem.

		   Historical note:

		   This ctor, introduced in version 1.2.6, conflicted
		   with an older 3-arg ctor taking (std::string,char
		   const *,uint) arguments, but this one is far more
		   flexible, so the older was removed. We also had
		   ctor taking a std::string, but that was removed
		   to avoid additional ambiguities.
		*/
   		explicit s11n_exception( const char *format, ... );


		/**
		   Identical to s11n_exception(format,...) except that
		   file/line/function information from the source_info
		   object is prefixed to the resulting what() string.
		   It is expected that this ctor be passed the
		   S11N_SOURCEINFO macro as its first argument.

		   Added in version 1.3.0.
		*/
   		s11n_exception( source_info const &, const char *format, ... );

	    /**
	       Equivalent to s11n_exception(si,"").

	       Added in 1.3.1.
	    */
   		s11n_exception( source_info const & si );

		/**
		   Returns the 'what' string passed to the ctor.
		*/
                virtual const char * what() const throw();
	protected:
		/**
		   Intended to be used by ctors.
		*/
		void what( std::string const & ) throw();
		s11n_exception();
        private:
                std::string m_what;
        };

	/**
	   An exception type for classloader-related exceptions. These
	   need to be caught separately from s11n_exceptions in some
	   cases because sometimes a classloader can try other
	   alternatives on an error.
	*/
	struct factory_exception : public s11n_exception
	{
	public:
		virtual ~factory_exception() throw() {}
   		explicit factory_exception( const char *format, ... );
	};


	/**
	   Really for use by clients, i/o layers, and s11nlite, not by
	   the s11n core.
	*/
	struct io_exception : public s11n_exception
	{
	public:
		virtual ~io_exception() throw() {}
   		explicit io_exception( const char *format, ... );
	};


} // namespace s11n

#endif // s11n_net_s11n_v1_3_EXCEPTION_HPP_INCLUDED
