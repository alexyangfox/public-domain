////////////////////////////////////////////////////////////////////////
// Win32-specific parts of plugin.{c,h}pp
//
// All Win32 code is 100% untested (i don't have Windows).
////////////////////////////////////////////////////////////////////////
#include <windows.h> // LoadModule()
#include <string>
#include <sstream>

namespace s11n { namespace plugin {
	std::string open( const std::string & basename )
	{
		std::string where = find( basename );
		if( where.empty() )
		{
			dll_error( std::string("s11n::plugin::open(")
				   + basename
				   + std::string( "): No DLL found in s11n::plugin::path().")
				   );
			return std::string();
		}
                DWORD soh = LoadModule( where.c_str(), NULL );
		if( soh <= 31 /* odd convention, but that's what the docs say */ )
		{
			/**
0  The system is out of memory or resources.
  ERROR_BAD_FORMAT
11L  The .exe file is invalid.
  ERROR_FILE_NOT_FOUND
2L  The specified file was not found.
  ERROR_PATH_NOT_FOUND
3L  The specified path was not found.
			*/
			std::ostringstream os;
			os << "s11n::plugin::open("<<basename<<"): Error opening DLL:\n"
			   << "file="<<where<<"\n"
			   << "LoadModule() error code="<<std::dec << soh
			   << "\n";
			dll_error( os.str() );
		}
		else
		{
			where = std::string();
			dll_error("");
		}
                return where;
	}

}} // namespace

