#include "UnitTest.hpp"

#include <s11n.net/s11n/s11nlite.hpp>
#include <s11n.net/s11n/s11n_debuggering_macros.hpp>

#include <s11n.net/s11n/proxy/pod/string.hpp>


class Thrower : public UnitTest
{
public:
	Thrower() : UnitTest("Thrower") {}
	virtual ~Thrower() {}
	virtual bool run()
	{
		throw s11n::s11n_exception( S11N_SOURCEINFO, "i just wanted to throw!" );
		return true;
	}
};

#define UNIT_TEST Thrower
#define UNIT_TEST_NAME "Thrower"
#include "RegisterUnitTest.hpp"
