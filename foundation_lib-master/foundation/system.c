/* system.c  -  Foundation library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
 * 
 * This library provides a cross-platform foundation library in C11 providing basic support data types and
 * functions to write applications and games in a platform-independent fashion. The latest source code is
 * always available at
 * 
 * https://github.com/rampantpixels/foundation_lib
 * 
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#include <foundation/foundation.h>

#if FOUNDATION_PLATFORM_POSIX
#  include <foundation/posix.h>
#endif

#if FOUNDATION_PLATFORM_ANDROID
#  include <foundation/android.h>
#  include <cpu-features.h>
#endif

#if FOUNDATION_PLATFORM_APPLE
extern unsigned int _system_process_info_processor_count( void );
extern int _system_show_alert( const char*, const char*, int );
#endif

static event_stream_t* _system_event_stream = 0;

typedef struct _foundation_platform_info
{
	platform_t      platform;
	architecture_t  architecture;
	byteorder_t     byteorder;
} platform_info_t;

static platform_info_t _platform_info = {

#if FOUNDATION_PLATFORM_WINDOWS
	PLATFORM_WINDOWS,
#elif FOUNDATION_PLATFORM_ANDROID
	PLATFORM_ANDROID,
#elif FOUNDATION_PLATFORM_LINUX
	PLATFORM_LINUX,
#elif FOUNDATION_PLATFORM_MACOSX
	PLATFORM_MACOSX,
#elif FOUNDATION_PLATFORM_IOS
	PLATFORM_IOS,
#else
#  error Unknown platform
#endif

#if FOUNDATION_PLATFORM_ARCH_X86_64
ARCHITECTURE_X86_64,
#elif FOUNDATION_PLATFORM_ARCH_X86
ARCHITECTURE_X86,
#elif FOUNDATION_PLATFORM_ARCH_PPC_64
ARCHITECTURE_PPC_64,
#elif FOUNDATION_PLATFORM_ARCH_PPC
ARCHITECTURE_PPC,
#elif FOUNDATION_PLATFORM_ARCH_ARM8
ARCHITECTURE_ARM8,
#elif FOUNDATION_PLATFORM_ARCH_ARM7
ARCHITECTURE_ARM7,
#elif FOUNDATION_PLATFORM_ARCH_ARM6
ARCHITECTURE_ARM6,
#else
#  error Unknown architecture
#endif

#if FOUNDATION_PLATFORM_ENDIAN_LITTLE
BYTEORDER_LITTLEENDIAN
#else
BYTEORDER_BIGENDIAN
#endif

};


platform_t system_platform()
{
	return _platform_info.platform;
}


architecture_t system_architecture()
{
	return _platform_info.architecture;
}


byteorder_t system_byteorder()
{
	return _platform_info.byteorder;
}


#if FOUNDATION_PLATFORM_WINDOWS

#include <foundation/windows.h>

object_t _system_library_iphlpapi = 0;


int _system_initialize( void )
{
	_system_event_stream = event_stream_allocate( 128 );
	return 0;
}


void _system_shutdown( void )
{
	if( _system_library_iphlpapi )
		library_unload( _system_library_iphlpapi );
	_system_library_iphlpapi = 0;

	event_stream_deallocate( _system_event_stream );
	_system_event_stream = 0;
}


const char* system_error_message( int code )
{
	static THREADLOCAL char errmsg[256];

	if( !code )
		code = GetLastError();
	if( !code )
		return "";

	errmsg[0] = 0;
	FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, code & 0xBFFFFFFF, 0/*LANG_SYSTEM_DEFAULT*//*MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT )*/, errmsg, 255, 0 );
	string_strip( errmsg, STRING_WHITESPACE );

	return errmsg;
}


const char* system_hostname( void )
{
	unsigned long size = 255;
	static char hostname[256] = {0};
	if( hostname[0] )
		return hostname;
	strcpy( hostname, "<unknown>" );
	GetComputerNameA( hostname, &size );
	hostname[255] = 0;
	return hostname;
}


uint64_t system_hostid( void )
{
	unsigned char hostid[8] = {0};
	IP_ADAPTER_INFO adapter_info[16];
	unsigned int status, i, j;
	unsigned long buffer_length;
	DWORD (STDCALL *fn_get_adapters_info)( PIP_ADAPTER_INFO, PULONG ) = 0;

	if( !_system_library_iphlpapi )
		_system_library_iphlpapi = library_load( "iphlpapi" );
	if( _system_library_iphlpapi )
		fn_get_adapters_info = (DWORD (STDCALL *)( PIP_ADAPTER_INFO, PULONG ))library_symbol( _system_library_iphlpapi, "GetAdaptersInfo" );
	if( !fn_get_adapters_info )
		return 0;
	
	buffer_length = sizeof( adapter_info );  // Save memory size of buffer
	memset( adapter_info, 0, sizeof( adapter_info ) );
	status = fn_get_adapters_info( adapter_info, &buffer_length );
	if( status == ERROR_SUCCESS ) for( i = 0; i < 16; ++i )
	{
		if( adapter_info[i].Type == MIB_IF_TYPE_ETHERNET )
		{
			for( j = 0; j < 6; ++j )
				hostid[5-j] = adapter_info[i].Address[j];
			break;
		}
	}
	return *(uint64_t*)hostid;
}


const char* system_username( void )
{
	unsigned long size = 255;
	static char username[256] = {0};
	if( username[0] )
		return username;
	strcpy( username, "<unknown>" );
	GetUserNameA( username, &size );
	username[255] = 0;
	return username;
}


unsigned int system_hardware_threads( void )
{
	SYSTEM_INFO system_info;
	GetSystemInfo( &system_info );
	return system_info.dwNumberOfProcessors;
}


void system_process_events( void )
{
}


bool system_debugger_attached( void )
{
	return IsDebuggerPresent();
}


void system_pause( void )
{
	system( "pause" );
}


static uint32_t _system_default_locale( void )
{
	return LOCALE_DEFAULT;
}


typedef int (STDCALL *fnGetLocaleInfoEx)( LPCWSTR, LCTYPE, LPWSTR, int );

static uint32_t _system_user_locale( void )
{
	fnGetLocaleInfoEx get_locale_info = (fnGetLocaleInfoEx)GetProcAddress( GetModuleHandleA( "kernel32.dll" ), "GetLocaleInfoEx" );
	if( get_locale_info )
	{
		wchar_t locale_sname[128] = {0};
		char locale_string[8] = {0};
		get_locale_info( LOCALE_NAME_USER_DEFAULT, LOCALE_SNAME, locale_sname, 32 );
		string_convert_utf16( locale_string, (uint16_t*)locale_sname, 8, (unsigned int)wcslen( locale_sname ) );
		locale_string[5] = 0;
		if( string_match_pattern( locale_string, "??" "-" "??" ) )
		{
			locale_string[2] = locale_string[3];
			locale_string[3] = locale_string[4];
			locale_string[4] = 0;
			log_infof( "User default locale: %s", locale_string );
			return *(uint32_t*)locale_string;
		}
	}
	
	return _system_default_locale();
}


#elif FOUNDATION_PLATFORM_POSIX

#include <ifaddrs.h>


int _system_initialize( void )
{
	_system_event_stream = event_stream_allocate( 128 );
	return 0;
}


void _system_shutdown( void )
{
	event_stream_deallocate( _system_event_stream );
	_system_event_stream = 0;
}


const char* system_error_message( int code )
{
	if( !code )
		code = errno;
	if( !code )
		return "<no error>";
#if FOUNDATION_PLATFORM_APPLE || FOUNDATION_PLATFORM_ANDROID
	static char buffer[256]; //TODO: Thread safety
	strerror_r( code, buffer, 256 );
	return buffer;
#else
	static THREADLOCAL char buffer[256];
	return strerror_r( code, buffer, 256 );
#endif
}


const char* system_hostname( void )
{
	static char hostname[256] = {0};
	if( hostname[0] )
		return hostname;
	strcpy( hostname, "<unknown>" );
	gethostname( hostname, 256 );
	hostname[255] = 0;
	return hostname;
}


const char* system_username( void )
{
	static char username[64] = {0};
	if( username[0] )
		return username;
	strcpy( username, "<unknown>" );
#if FOUNDATION_PLATFORM_ANDROID
	strncpy( username, getlogin(), 64 );
#else
	getlogin_r( username, 64 );
#endif
	username[63] = 0;
	return username;	
}


#if FOUNDATION_PLATFORM_APPLE

#include <net/if_dl.h>

static uint64_t _system_hostid_lookup( struct ifaddrs* ifaddr )
{
	unsigned int j;
	union
	{
		uint64_t               id;
		unsigned char ALIGN(8) buffer[8];
	} hostid;

	if( ifaddr->ifa_addr && ( ifaddr->ifa_addr->sa_family == AF_LINK ) )
	{
		struct sockaddr_dl* addr_dl = (struct sockaddr_dl*)ifaddr->ifa_addr;
		
		FOUNDATION_ASSERT( addr_dl->sdl_alen == 6 );

		hostid.id = 0;
		for( j = 0; j < 6; ++j )
			hostid.buffer[5-j] = LLADDR(addr_dl)[j];
		
		return hostid.id;
	}
	
	return 0;
}

#else

static uint64_t _system_hostid_lookup( int sock, struct ifreq* ifr )
{
	unsigned int j;
	union
	{
		uint64_t               id;
		unsigned char ALIGN(8) buffer[8];
	} hostid;
	
	if( ioctl( sock, SIOCGIFHWADDR, ifr ) < 0 )
		return 0;
	
	hostid.id = 0;
	for( j = 0; j < 6; ++j )
		hostid.buffer[5-j] = ifr->ifr_hwaddr.sa_data[j];
	
	return hostid.id;
}

#endif


uint64_t system_hostid( void )
{
	struct ifaddrs* ifaddr;
	struct ifaddrs* ifa;
	uint64_t hostid = 0;

#if !FOUNDATION_PLATFORM_APPLE
	struct ifreq buffer;
	int sock = socket( PF_INET, SOCK_DGRAM, 0 );
#endif
	
	if( getifaddrs( &ifaddr ) == 0 )
	{
		for( ifa = ifaddr; ifa && !hostid; ifa = ifa->ifa_next )
		{
			if( string_equal_substr( ifa->ifa_name, "lo", 2 ) )
				continue;
			
#if FOUNDATION_PLATFORM_APPLE
			
			
#else
			memset( &buffer, 0, sizeof( buffer ) );
			string_copy( buffer.ifr_name, ifa->ifa_name, sizeof( buffer.ifr_name ) );

			hostid = _system_hostid_lookup( sock, &buffer );
#endif
		}
		freeifaddrs( ifaddr );
	}
#if !FOUNDATION_PLATFORM_APPLE
	else
	{
		memset( &buffer, 0, sizeof( buffer ) );
		strcpy( buffer.ifr_name, "eth0" );

		hostid = _system_hostid_lookup( sock, &buffer );
	}

	close( sock );
#endif
	
	return hostid;
}


unsigned int system_hardware_threads( void )
{
#if FOUNDATION_PLATFORM_APPLE
	return _system_process_info_processor_count();
#elif FOUNDATION_PLATFORM_ANDROID
	return android_getCpuCount();
#else
	cpu_set_t prevmask, testmask;
	CPU_ZERO( &prevmask );
	CPU_ZERO( &testmask );
	sched_getaffinity( 0, sizeof( prevmask ), &prevmask ); //Get current mask
	sched_setaffinity( 0, sizeof( testmask ), &testmask ); //Set zero mask
	sched_getaffinity( 0, sizeof( testmask ), &testmask ); //Get mask for all CPUs
	sched_setaffinity( 0, sizeof( prevmask ), &prevmask ); //Reset current mask
	unsigned int num = CPU_COUNT( &testmask );
	return ( num > 1 ? num : 1 );
#endif
}


void system_process_events( void )
{
#if FOUNDATION_PLATFORM_ANDROID
	profile_begin_block( "system events" );

	int ident = 0;
	int events = 0;
	int nummsg = 0;
	struct android_poll_source* source = 0;
	struct android_app* app = android_app();

	while( ( ident = ALooper_pollAll( 0, 0, &events, (void**)&source ) ) >= 0 )
	{
		// Process this event.
		if( source )
			source->process( app, source );
		++nummsg;
	}
	
	profile_end_block();
#endif
}


bool system_debugger_attached( void )
{
	return false;
}


void system_pause( void )
{
}


static uint32_t _system_default_locale( void )
{
	return LOCALE_DEFAULT;
}


static uint32_t _system_user_locale( void )
{
	return _system_default_locale();
}


void system_browser_open( const char* url )
{

}


#endif


uint32_t system_locale( void )
{
	uint32_t localeval = 0;
	char localestr[4];
	
	const char* locale = config_string( HASH_USER, HASH_LOCALE );
	if( ( locale == LOCALE_BLANK ) || ( string_length( locale ) != 4 ) )
		locale = config_string( HASH_APPLICATION, HASH_LOCALE );
	if( ( locale == LOCALE_BLANK ) || ( string_length( locale ) != 4 ) )
		locale = config_string( HASH_FOUNDATION, HASH_LOCALE );
	if( ( locale == LOCALE_BLANK ) || ( string_length( locale ) != 4 ) )
		return _system_user_locale();
	
#define _LOCALE_CHAR_TO_LOWERCASE(x)   (((unsigned char)(x) >= 'A') && ((unsigned char)(x) <= 'Z')) ? (((unsigned char)(x)) | (32)) : (x)
#define _LOCALE_CHAR_TO_UPPERCASE(x)   (((unsigned char)(x) >= 'a') && ((unsigned char)(x) <= 'z')) ? (((unsigned char)(x)) & (~32)) : (x)
	localestr[0] = _LOCALE_CHAR_TO_LOWERCASE( locale[0] );
	localestr[1] = _LOCALE_CHAR_TO_LOWERCASE( locale[1] );
	localestr[2] = _LOCALE_CHAR_TO_UPPERCASE( locale[2] );
	localestr[3] = _LOCALE_CHAR_TO_UPPERCASE( locale[3] );

	memcpy( &localeval, localestr, 4 );
	return localeval;
}


const char* system_locale_string( void )
{
	static char localestr[5] = {0};
	uint32_t locale = system_locale();
	memcpy( localestr, &locale, 4 );
	return localestr;
}


uint16_t system_language( void )
{
	return (uint16_t)( ( system_locale() >> 16 ) & 0xFFFF );
}


uint16_t system_country( void )
{
	return (uint16_t)( system_locale() & 0xFFFF );
}


event_stream_t* system_event_stream( void )
{
	return _system_event_stream;
}


void system_post_event( foundation_event_id event )
{
	event_post( _system_event_stream, SYSTEM_FOUNDATION, event, 0, 0, 0, 0 );
}


bool system_message_box( const char* title, const char* message, bool cancel_button )
{
	log_infof( "%s\n\n%s", title, message );
	if( environment_application()->flags & APPLICATION_UTILITY )
		return true;
#if FOUNDATION_PLATFORM_WINDOWS
	return ( MessageBoxA( 0, message, title, cancel_button ? MB_OKCANCEL : MB_OK ) == IDOK );
#elif FOUNDATION_PLATFORM_APPLE
	return _system_show_alert( title, message, cancel_button ? 1 : 0 ) > 0;
#elif 0//FOUNDATION_PLATFORM_LINUX
	char* buf = string_format( "%s\n\n%s\n", title, message );
	pid_t pid = fork();

	switch( pid )
	{
		case -1:
			//error
			string_deallocate( buf );
			break;

		case 0:
			execlp( "xmessage", "xmessage", "-buttons", cancel_button ? "OK:101,Cancel:102" : "OK:101", "-default", "OK", "-center", buf, (char*)0 );
			_exit( -1 );
			break;

		default:
		{
			string_deallocate( buf );
			int status;
			waitpid( pid, &status, 0 );
			if( ( !WIFEXITED( status ) ) || ( WEXITSTATUS( status ) != 101 ) )
				return false;
			return true;
		}
	}

	return false;
#else
	//Not implemented
	return false;
#endif
}
