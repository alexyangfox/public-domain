////////////////////////////////////////////////////////////////////////
// Test/demo code contributed by Rattlemouse <rattlemouse@apeha.ru>.
////////////////////////////////////////////////////////////////////////
#include <s11n.net/s11n/s11nlite.hpp> // s11n & s11nlite frameworks
#include <s11n.net/s11n/s11n_debuggering_macros.hpp>
#include <s11n.net/s11n/proxy/pod/int.hpp>
#include <s11n.net/s11n/proxy/pod/string.hpp>
#include <s11n.net/s11n/proxy/pod/double.hpp>
#include <functional>
#include <iterator>
#include <s11n.net/s11n/functional.hpp>
#include <s11n.net/s11n/0x/ohex.hpp>

#include "coord.hpp"
#include "coord_s11n.hpp"
/**
   Some experimentation with the C++0x features.
*/

void do_first()
{
    int vi = 42;
    double vd = 42.42;
    std::string vs("i'm a std::string");
    COUT << "serialize_group():\n";
    s11nlite::node_type node;
    bool ret = s11n::cpp0x::serialize_group(node, "pods", vi, vd, vs);
    if( ret )
    {
	s11nlite::save( node, std::cout );
    }
    else
    {
	throw s11n::s11n_exception( S11N_SOURCEINFO, "serialize_group() failed" );
    }

    int di = -1;
    double dd = -1;
    std::string ds = "error";

    if(1)
    {
	ret = s11n::cpp0x::deserialize_group(node, "pods", di, dd, ds);
	if( ret )
	{
	    //COUT << "Deserialized group: ("<<di<<", "<<dd<<", ["<<ds<<"])\n";
	    COUT << "Deserialized group: (";
	    std::cout <<  di << '|' << dd << '|' << ds
		      << ")\n";
	}
	else
	{
	    throw s11n::s11n_exception( S11N_SOURCEINFO, "deserialize_group() failed" );
	}
    }

    if(1)
    {
	s11nlite::node_traits::clear(node);
	coord cord(42,24);
	COUT << "serialize_subnodes():\n";
	if( ! s11n::cpp0x::serialize_subnodes( node,
					       "myInt", di,
					       "myDouble", dd,
					       "myString", ds,
					       "myxy", cord ) )
	{
	    throw s11n::s11n_exception(S11N_SOURCEINFO);
	}
	s11nlite::save( node, std::cout );
	    
	int myi; double myd; std::string mystr;
	cord = coord();
	if( ! s11n::cpp0x::deserialize_subnodes( node,
						 "myInt", myi,
						 "myString", mystr,
						 "myDouble", myd,
						 "myxy", cord ) )
	{
	    throw s11n::s11n_exception(S11N_SOURCEINFO);
	}
	    
	std::cout << "deserialize_subnodes() got: " <<myi
		  << '|' << myd <<'|' << mystr
		  <<'|' << cord.x() << ','<<cord.y()
		  << '\n';
	    
    }

}


int main(int argc, char *argv[])
{
    s11nlite::serializer_class("parens");
    try
    {
	do_first();
    }
    catch(std::exception const & ex)
    {
	CERR << "EXCEPTION: " << ex.what() << '\n';
	return 1;
    }
    return 0;
}
