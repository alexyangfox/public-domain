#include "UnitTest.hpp"

// #include <s11n.net/s11n/s11nlite.hpp>
// #include <s11n.net/s11n/s11n_debuggering_macros.hpp>
// #include <s11n.net/s11n/proxy/pod/string.hpp>
#include <s11n.net/s11n/io/url.hpp>


class URLParser : public UnitTest
{
public:
	URLParser() : UnitTest("URLParser") {}
	virtual ~URLParser() {}
	virtual bool run()
	{
		typedef s11n::io::url_parser UP;
		UP u1( "http://s11n.net" );
		UT_ASSERT( u1.good() );
		UT_ASSERT( "http" == u1.scheme() );
		UT_ASSERT( "s11n.net" == u1.host() );

		// make sure copy-on-write works:
		UP u2( u1 );
		UT_ASSERT( u2.host() == u1.host() );
		u2.parse( "http://sourceforge.net/projects/s11n" );
		UT_ASSERT( u2.host() != u1.host() );
		UT_ASSERT( "/projects/s11n" == u2.path() );
		UT_ASSERT( "s11n.net" == u1.host() );

		return true;
	}
};

#define UNIT_TEST URLParser
#define UNIT_TEST_NAME "URLParser"
#include "RegisterUnitTest.hpp"
