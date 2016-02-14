#ifndef s11n_io_URL_HPP_INCLUDED
#define s11n_io_URL_HPP_INCLUDED 1

#include <string>
#include <s11n.net/s11n/export.hpp>
#include <s11n.net/s11n/refcount.hpp>
#include <s11n.net/s11n/factory.hpp>

// Reminder: RFC1738: http://www.ietf.org/rfc/rfc1738.txt

namespace s11n { namespace io {

	/**
	   url_parser is a basic implementation for
	   parsing a URL string into its atomic components.
	   It is not a full-featured parser, for example it does
	   not parse key=value arguments at the end of a URL.

	   This type uses reference-counted internal data and
	   copy-on-write, so copying it is cheap.
	*/
	class S11N_EXPORT_API url_parser
	{
	public:
		/**
		   Parses the given URL. good() reveals the status
		   of the parse.
		*/
		url_parser( std::string const & );
		/**
		   Creates an empty (!good()) parser.
		*/
		url_parser();
// Rely on default copy/assign ops:
// 		url_parser & url_parser( url_parser const & );
// 		url_parser & operator=( url_parser const & );
		/**
		   Functions the same as the string-argument ctor.
		 */
 		url_parser & operator=( std::string const & );
		~url_parser();
		/**
		   Returns true if the last parse() got a "valid" URL.
		*/
		bool good() const;

		/**
		   Parses URLs of the following forms:

		   scheme://[user[:password]@]host[:[port[:]]][/path/to/resource]

		   Note that host may have an optional ':' after it
		   without a port number, and that a port number may be followed
		   by an optional ':' character. This is to accommodate ssh
		   URLs and the like:

		   ssh://user\@host:/path

		   ssh://user\@host:33:/path

		   This function returns the same as good().

		   If this function returns false then the contents of this
		   objects are in an undefined state. They should not be used
		   before a call to parse() succeeds.
		*/
		bool parse( std::string const & );

		/** Returns the URL most recently passed to parse(). */
		std::string url() const;
		/** Returns the scheme part of url(). */
		std::string scheme() const;
		/** Returns the user name part of url(), which may be empty. */
		std::string user() const;
		/** Returns the user password part of url(), which may be empty. */
		std::string password() const;
		/** Returns the host part of url(). */
		std::string host() const;
		/**
		   Returns the resource path part of url(), which may be empty.

		   Contrary to RFC1738, a leading slash in a URL *is* considered
		   to be part of the path.

		   In some protocols (e.g. http) an empty path can be
		   considered the same as '/', but on others
		   (e.g. file) such interpretation is not appropriate.
		*/
		std::string path() const;

		/**
		   If the URL path has a '?' in it, anything after the '?'
		   is assumed to be a list of arguments, e.g. as those passed
		   to HTTP GET requests. This string does not contain the leading
		   '?'.
		*/
		std::string args_str() const;

		typedef std::map<std::string,std::string> args_map_type;
		args_map_type const & args_map() const;

		/** Returns the port number part of url(), or 0 if no port was specified. */
		unsigned short port() const;
	private:
		/**
		   impl holds the private data for a url_parser.

		   PS: i hate that this has to be in the public
		   header, but rcptr<impl> needs impl to be a complete
		   type.

		   TODO: get rid of the rcptr<> usage and hide the pimpl
		   in the implementation. Also consider storing all the
		   string data in a single (char*), using NULLs to delimit
		   it. That'd save a lot of space.
		*/
		struct impl
		{
			std::string url;
			std::string proto;
			std::string user;
			std::string pass;
			std::string host;
			unsigned short port;
			std::string path;
			std::string args_str;
			bool good;
			args_map_type args_map;
			impl();
		};
		s11n::refcount::rcptr<impl> pimpl;
	};

	/**
	   A factory type intended to be subclassed to provide
	   protocol-specific i/o streams.

	   Subclasses must reimplement the virtual functions and
	   register with the classloader like so:

<pre>
#define S11N_FACREG_TYPE my_subclass_type
#define S11N_FACREG_INTERFACE_TYPE s11n::io::url_stream_factory
#define S11N_FACREG_TYPE_NAME "my_subclass_type"
#include <s11n.net/s11n/factory_reg.hpp>
</pre>

           They may also want to set up classloader aliases during
	   the static initialization phase, as demonstrated for
	   the file:// protocol in url.cpp.

	   Note for subclasser: NEVER EVER call
	   s11n::io::get_i/ostream() from this class, because those
	   functions dispatch to url_stream_factory when possible, and
	   callint those from here can cause an endless loop.

	*/
	class S11N_EXPORT_API url_stream_factory
	{
	protected:
		url_stream_factory() {}

		/**
		   Default implementation returns 0. Subclasses.should return an instance
		   of a stream capable of writing to the given URL. On error they should
		   return 0 or throw an exception.

		   The caller owns the returned pointer, which may be 0.
		*/
		virtual std::ostream * do_get_ostream( url_parser const & url ) const
		{
			return 0;
		}
		/**
		   Default implementation returns 0. Subclasses.should return an instance
		   of a stream capable of reading from the given URL. On error they should
		   return 0 or throw an exception.

		   The caller owns the returned pointer, which may be 0.
		*/
		virtual std::istream * do_get_istream( url_parser const & url ) const
		{
			return 0;
		}

	public:
		virtual ~url_stream_factory() {}

		/**
		   See do_get_ostream().
		*/
		std::ostream * get_ostream( url_parser const & url ) const
		{
			return this->do_get_ostream( url );
		}
		/**
		   See do_get_istream().
		*/
		std::istream * get_istream( url_parser const & url ) const
		{
			return this->do_get_istream( url );
		}

		/**
		   Classloads an instance of url_stream_factory
		   associated with the given scheme. Caller owns the
		   returned pointer, which may be 0.

		   Subclass authors are responsible for registering their
		   subclasses with the url_stream_factory classloader.
		*/
		static url_stream_factory * create_factory_for_scheme( std::string const & scheme );

		/**
		   Registers SubclassT as a subclass of
		   url_stream_factory such that calling
		   create_factory_for_scheme(scheme) will return an
		   instance of SubclassT. SubclassT must be-a
		   url_stream_factory and must be compatible with the
		   s11n::fac factory layer.
		*/
		template <typename SubclassT>
		static void register_factory_for_scheme( std::string const & scheme )
		{
			s11n::fac::register_subtype< url_stream_factory, SubclassT >( scheme );
		}

	};

	/**
	   Convenience overload.
	*/
	std::istream * get_url_istream( std::string const & url );
	/**
	   Classloads an instance of an istream, using a
	   url_stream_factory to create the stream. Caller owns the
	   returned pointer, which may be 0. Failure indicates one of:

	   - !url.good()

	   - no url_stream_factory was mapped to url.scheme().

	   - The factory could not create the required stream.
	*/
	std::istream * get_url_istream( url_parser const & url );

	/**
	   Convenience overload.
	*/
	std::ostream * get_url_ostream( std::string const & url );

	/**
	   See get_url_istream().
	*/
	std::ostream * get_url_ostream( url_parser const & url );

	/**
	   This factory creates streams for URLs in the following format:

	   file:[//]/path/to/file

	   It works for input and output.

	   If your libs11n is configured/built with
	   s11n_CONFIG_HAVE_ZFSTREAM set to true then the zfstream
	   library is used to support bzip2/gzip files.
	*/
	class S11N_EXPORT_API file_stream_factory : public url_stream_factory
	{
	public:
		file_stream_factory();
		virtual ~file_stream_factory();

	protected:
		/**
		   Creates an ostream for a file:// URL. If your
		   s11n is built with zfstream support, then
		   the compressors supported by that library
		   are supported here.

		   The caller owns the returned pointer, which may be
		   0.
		*/
		virtual std::ostream * do_get_ostream( url_parser const & url ) const;
		/**
		   Creates an istream for a file:// URL. If your
		   s11n is built with zfstream support, then
		   the compressors supported by that library
		   are supported here.

		   The caller owns the returned pointer, which may be
		   0.
		*/
		virtual std::istream * do_get_istream( url_parser const & url ) const;
	};

}} // namespaces



#endif // s11n_io_URL_HPP_INCLUDED
