/* fs.c  -  Foundation library  -  Public Domain  -  2013 Mattias Jansson / Rampant Pixels
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
#include <foundation/internal.h>

#if FOUNDATION_PLATFORM_WINDOWS
#  include <foundation/windows.h>
#  include <sys/utime.h>
#elif FOUNDATION_PLATFORM_POSIX
#  include <foundation/posix.h>
#  include <time.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <sys/ioctl.h>
#  include <utime.h>
#  include <fcntl.h>
#  include <dirent.h>
#endif

#if FOUNDATION_PLATFORM_LINUX
#  include <sys/inotify.h>
#endif

#include <stdio.h>


typedef struct _foundation_fs_monitor
{
	char*             path;
	object_t          thread;
	mutex_t*          signal;
} fs_monitor_t;

typedef struct ALIGN(8) _foundation_stream_file
{
	FOUNDATION_DECLARE_STREAM;
	void*                  fd;
} stream_file_t;

#define GET_FILE( s ) ((stream_file_t*)(s))
#define GET_FILE_CONST( s ) ((const stream_file_t*)(s))
#define GET_STREAM( f ) ((stream_t*)(f))

static stream_vtable_t _fs_file_vtable;

static void* _fs_monitor( object_t, void* );

static fs_monitor_t* _fs_monitors = 0;
static event_stream_t* _fs_event_stream = 0;


void fs_monitor( const char* path )
{
	int mi, msize;
	fs_monitor_t* monitor = 0;
	for( mi = 0, msize = array_size( _fs_monitors ); mi < msize; ++mi )
	{
		if( string_equal( _fs_monitors[mi].path, path ) )
			return;
	}

	memory_context_push( MEMORYCONTEXT_STREAM );

	monitor = memory_allocate_zero( sizeof( fs_monitor_t ), 0, MEMORY_PERSISTENT );
	monitor->path = string_clone( path );
	monitor->signal = mutex_allocate( "fs_monitor_signal" );
	monitor->thread = thread_create( _fs_monitor, "fs_monitor", THREAD_PRIORITY_BELOWNORMAL, 0 );

	for( mi = 0, msize = array_size( _fs_monitors ); mi < msize; ++mi )
	{
		if( !_fs_monitors[mi].thread && !_fs_monitors[mi].path && !_fs_monitors[mi].signal )
		{
			memcpy( _fs_monitors + mi, monitor, sizeof( fs_monitor_t ) );
			break;
		}
	}

	if( mi == msize )
		array_push_memcpy( _fs_monitors, monitor );

	thread_start( monitor->thread, monitor );

	memory_context_pop();
}


static void _fs_stop_monitor( fs_monitor_t* monitor )
{
	object_t thread = monitor->thread;
	mutex_t* notify = monitor->signal;
	char* localpath = monitor->path;

	memset( monitor, 0, sizeof( fs_monitor_t ) );

	thread_terminate( thread );

	if( notify )
		mutex_signal( notify );

	thread_destroy( thread );

	while( thread_is_running( thread ) )
		thread_yield();

	if( localpath )
		string_deallocate( localpath );

	if( notify )
		mutex_deallocate( notify );
}


void fs_unmonitor( const char* path )
{
	int mi, msize;
	for( mi = 0, msize = array_size( _fs_monitors ); mi < msize; ++mi )
	{
		if( string_equal( _fs_monitors[mi].path, path ) )
			_fs_stop_monitor( _fs_monitors + mi );
	}
}


bool fs_is_file( const char* path )
{
#if FOUNDATION_PLATFORM_WINDOWS

	wchar_t* wpath = wstring_allocate_from_string( path, 0 );
	unsigned int attribs = GetFileAttributesW( wpath );
	wstring_deallocate( wpath );
	if( ( attribs != 0xFFFFFFFF ) && !( attribs & FILE_ATTRIBUTE_DIRECTORY ) )
		return true;

#elif FOUNDATION_PLATFORM_POSIX

	struct stat st; memset( &st, 0, sizeof( st ) );
	stat( path, &st );
	if( st.st_mode & S_IFREG )
		return true;

#else
#  error Not implemented
#endif

	return false;
}


bool fs_is_directory( const char* path )
{
#if FOUNDATION_PLATFORM_WINDOWS

	wchar_t* wpath = wstring_allocate_from_string( path, 0 );
	unsigned int attr = GetFileAttributesW( wpath );
	wstring_deallocate( wpath );
	if( ( attr == 0xFFFFFFFF) || !( attr & FILE_ATTRIBUTE_DIRECTORY ) )
		return false;

#elif FOUNDATION_PLATFORM_POSIX

	struct stat st; memset( &st, 0, sizeof( st ) );
	stat( path, &st );
	if( !( st.st_mode & S_IFDIR ) )
		return false;

#else
#  error Not implemented
#endif

	return true;
}


char** fs_subdirs( const char* path )
{
	char** arr = 0;
#if FOUNDATION_PLATFORM_WINDOWS

	//Windows specific implementation of directory listing
	HANDLE find;
	WIN32_FIND_DATAW data;
	char* pattern = path_append( string_clone( path ), "*" );
	wchar_t* wpattern = wstring_allocate_from_string( pattern, 0 );
	string_deallocate( pattern );

	memory_context_push( MEMORYCONTEXT_STREAM );

	find = FindFirstFileW( wpattern, &data );
	if( find != INVALID_HANDLE_VALUE ) do
	{
		if( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			if( data.cFileName[0] == L'.' )
			{
				if( !data.cFileName[1] || ( data.cFileName[1] == L'.' ) )
					continue; //Don't include . and .. directories
			}
			array_push( arr, string_allocate_from_wstring( data.cFileName, 0 ) );
		}
	} while( FindNextFileW( find, &data ) );
	FindClose( find );

	memory_context_pop();
	
	wstring_deallocate( wpattern );
	
#elif FOUNDATION_PLATFORM_POSIX
	
	//POSIX specific implementation of directory listing
	DIR* dir = opendir( path );
	if( dir )
	{
		struct dirent* entry = 0;
		struct stat st;

		memory_context_push( MEMORYCONTEXT_STREAM );

		while( ( entry = readdir( dir ) ) != 0 )
		{
			if( entry->d_name[0] == '.' )
			{
				if( !entry->d_name[1] || ( entry->d_name[1] == '.' ) )
					continue; //Don't include . and .. directories
			}
			char* found = path_append( string_clone( path ), entry->d_name );
			if( !stat( found, &st ) && S_ISDIR( st.st_mode ) )
				array_push( arr, string_clone( entry->d_name ) );
			string_deallocate( found );
		}
		closedir( dir );

		memory_context_pop();
	}

#else
#  error Not implemented
#endif

	return arr;
}


char** fs_files( const char* path )
{
	char** arr = 0;
#if FOUNDATION_PLATFORM_WINDOWS
	
	//Windows specific implementation of directory listing
	HANDLE find;
	WIN32_FIND_DATAW data;
	char* pattern = path_append( string_clone( path ), "*" );
	wchar_t* wpattern = wstring_allocate_from_string( pattern, 0 );
	string_deallocate( pattern );

	memory_context_push( MEMORYCONTEXT_STREAM );

	find = FindFirstFileW( wpattern, &data );
	if( find != INVALID_HANDLE_VALUE ) do
	{
		if( !( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
			array_push( arr, string_allocate_from_wstring( data.cFileName, 0 ) );
	} while( FindNextFileW( find, &data ) );
	FindClose( find );

	memory_context_pop();

	wstring_deallocate( wpattern );
	
#elif FOUNDATION_PLATFORM_POSIX
	
	//POSIX specific implementation of directory listing
	DIR* dir = opendir( path );
	if( dir )
	{
		//We have a directory, parse and create virtual file system
		struct dirent* entry = 0;
		struct stat st;

		memory_context_push( MEMORYCONTEXT_STREAM );

		while( ( entry = readdir( dir ) ) != 0 )
		{
			char* found = path_append( string_clone( path ), entry->d_name );
			if( !stat( found, &st ) && S_ISREG( st.st_mode ) )
				array_push( arr, string_clone( entry->d_name ) );
			string_deallocate( found );
		}
		closedir( dir );

		memory_context_pop();
	}

#else
#  error Not implemented
#endif

	return arr;
}


bool fs_remove_file( const char* path )
{
	bool result = false;
	char* fpath = path_make_absolute( path );

#if FOUNDATION_PLATFORM_WINDOWS
	wchar_t* wpath = wstring_allocate_from_string( fpath, 0 );
	result = DeleteFileW( wpath );
	wstring_deallocate( wpath );
#elif FOUNDATION_PLATFORM_POSIX
	result = ( unlink( fpath ) == 0 );
#else
#  error Not implemented
#endif
	string_deallocate( fpath );
	
	return result;
}


bool fs_remove_directory( const char* path )
{
	bool result = false;
	char* fpath = path_make_absolute( path );
	char** subpaths;
	char** subfiles;
#if FOUNDATION_PLATFORM_WINDOWS
	wchar_t* wfpath = 0;
#endif
	int i, num;

	if( !fs_is_directory( fpath ) )
		goto end;
	
	subpaths = fs_subdirs( fpath );
	for( i = 0, num = array_size( subpaths ); i < num; ++i )
	{
		char* next = path_merge( fpath, subpaths[i] );
		fs_remove_directory( next );
		string_deallocate( next );
	}
	string_array_deallocate( subpaths );

	subfiles = fs_files( fpath );
	for( i = 0, num = array_size( subfiles ); i < num; ++i )
	{
		char* next = path_merge( fpath, subfiles[i] );
		fs_remove_file( next );
		string_deallocate( next );
	}
	string_array_deallocate( subfiles );
	
#if FOUNDATION_PLATFORM_WINDOWS
	wfpath = wstring_allocate_from_string( fpath, 0 );
	result = RemoveDirectoryW( wfpath );
	wstring_deallocate( wfpath );
#elif FOUNDATION_PLATFORM_POSIX
	result = ( rmdir( fpath ) == 0 );
#else
#  error Not implemented
#endif

	end:

	string_deallocate( fpath );

	return result;
}


bool fs_make_directory( const char* path )
{
	char* fpath;
	char** paths;
	char* curpath;
	int ipath;
	int pathsize;
	bool result = true;

	fpath = path_make_absolute( path );
	paths = string_explode( fpath, "/", false );
	pathsize = array_size( paths );
	curpath = 0;
	ipath = 0;

	memory_context_push( MEMORYCONTEXT_STREAM );

#if FOUNDATION_PLATFORM_WINDOWS
	if( pathsize )
	{
		char* first = paths[ipath];
		unsigned int flen = string_length( first );
		if( flen && ( first[ flen - 1 ] == ':' ) )
		{
			curpath = string_clone( first );
			++ipath; //Drive letter
		}
	}
#endif

	for( ; ipath < pathsize; ++ipath )
	{
		curpath = string_append( curpath, "/" );
		curpath = string_append( curpath, paths[ipath] );

		if( !fs_is_directory( curpath ) )
		{
#if FOUNDATION_PLATFORM_WINDOWS
			wchar_t* wpath = wstring_allocate_from_string( curpath, 0 );
			result = CreateDirectoryW( wpath, 0 );
			wstring_deallocate( wpath );
#elif FOUNDATION_PLATFORM_POSIX
			mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH;
			result = ( mkdir( curpath, mode ) == 0 );
#else
#  error Not implemented
#endif
			if( !result )
			{
				log_warnf( WARNING_SUSPICIOUS, "Failed to create directory: %s", curpath );
				//Unable to create directory
				goto end;
			}

			log_debugf( "Created directory: %s", curpath );
		}
	}
	
	end:
	
	string_deallocate( fpath  );
	string_array_deallocate( paths );
	string_deallocate( curpath );
	
	memory_context_pop();

	return result;
}


void fs_copy_file( const char* source, const char* dest )
{
	stream_t* infile;
	stream_t* outfile;
	void* buffer;

	char* destpath = path_path_name( dest );
	if( string_length( destpath ) )
		fs_make_directory( destpath );
	string_deallocate( destpath );

	infile = fs_open_file( source, STREAM_IN );
	outfile = fs_open_file( dest, STREAM_OUT );
	buffer = memory_allocate( 64 * 1024, 0, MEMORY_TEMPORARY );

	while( !stream_eos( infile ) )
	{
		uint64_t numread = stream_read( infile, buffer, 64 * 1024 );
		if( numread )
			stream_write( outfile, buffer, numread );
	}

	memory_deallocate( buffer );
	stream_deallocate( infile );
	stream_deallocate( outfile );
}


uint64_t fs_last_modified( const char* path )
{
#if FOUNDATION_PLATFORM_WINDOWS

	//This is retarded beyond belief, Microsoft decided that "100-nanosecond intervals since 1 Jan 1601" was
	//a nice basis for a timestamp... wtf? Anyway, number of such intervals to base date for unix time, 1 Jan 1970, is 116444736000000000
	const uint64_t ms_offset_time = 116444736000000000ULL;

	uint64_t last_write_time;
	wchar_t* wpath;
	WIN32_FILE_ATTRIBUTE_DATA attrib;
	BOOL success = 0;
	memset( &attrib, 0, sizeof( attrib ) );

	wpath = wstring_allocate_from_string( path, 0 );
	success = GetFileAttributesExW( wpath, GetFileExInfoStandard, &attrib );
	wstring_deallocate( wpath );

	/*SYSTEMTIME stime;
	memset( &stime, 0, sizeof( stime ) );
	stime.wYear  = 1970;
	stime.wDay   = 1;
	stime.wMonth = 1;
	SystemTimeToFileTime( &stime, &basetime );
	uint64_t ms_offset_time = (*(uint64_t*)&basetime);*/

	if( !success )
		return 0;
	
	last_write_time = ( (uint64_t)attrib.ftLastWriteTime.dwHighDateTime << 32ULL ) | (uint64_t)attrib.ftLastWriteTime.dwLowDateTime;

	return ( last_write_time > ms_offset_time ) ? ( ( last_write_time - ms_offset_time ) / 10000ULL ) : 0;

#elif FOUNDATION_PLATFORM_POSIX

	struct stat st; memset( &st, 0, sizeof( st ) );
	if( stat( path, &st ) < 0 )
		return 0;
	return (uint64_t)st.st_mtime * 1000ULL;

#else
#  error Not implemented
#endif
}


uint128_t fs_md5( const char* path )
{
	uint128_t digest = {0};
	stream_t* file = fs_open_file( path, STREAM_IN | STREAM_BINARY );
	if( file )
	{
		digest = stream_md5( file );
		stream_deallocate( file );
	}
	return digest;
}


void fs_touch( const char* path )
{
#if FOUNDATION_PLATFORM_WINDOWS
	wchar_t* wpath = wstring_allocate_from_string( path, 0 );
	_wutime( wpath, 0 );
	wstring_deallocate( wpath );
#elif FOUNDATION_PLATFORM_POSIX
	utime( path, 0 );
#else
#  error Not implemented
#endif
}


char** fs_matching_files( const char* path, const char* pattern, bool recurse )
{
	char** names = 0;
	char** subdirs = 0;
	unsigned int id, dsize, in, nsize;

#if FOUNDATION_PLATFORM_WINDOWS

	//Windows specific implementation of directory listing
	WIN32_FIND_DATAW data;
	char* pathpattern = path_merge( path, pattern );
	wchar_t* wpattern = wstring_allocate_from_string( pathpattern, 0 );
	
	HANDLE find = FindFirstFileW( wpattern, &data );

	memory_context_push( MEMORYCONTEXT_STREAM );

	if( find != INVALID_HANDLE_VALUE ) do
	{
		if( !( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
			array_push( names, path_clean( string_allocate_from_wstring( data.cFileName, 0 ), false ) );
	} while( FindNextFileW( find, &data ) );

	memory_context_pop();

	FindClose( find );

	wstring_deallocate( wpattern );
	string_deallocate( pathpattern );

#else

	char** fnames = fs_files( path );

	memory_context_push( MEMORYCONTEXT_STREAM );

	for( in = 0, nsize = array_size( fnames ); in < nsize; ++in )
	{
		if( string_match_pattern( fnames[in], pattern ) )
		{
			array_push( names, fnames[in] );
			fnames[in] = 0;
		}
	}

	memory_context_pop();

	string_array_deallocate( fnames );

#endif

	if( !recurse )
		return names;

	subdirs = fs_subdirs( path );

	memory_context_push( MEMORYCONTEXT_STREAM );

	for( id = 0, dsize = array_size( subdirs ); id < dsize; ++id )
	{
		char* subpath = path_merge( path, subdirs[id] );
		char** subnames = fs_matching_files( subpath, pattern, true );

		for( in = 0, nsize = array_size( subnames ); in < nsize; ++in )
			array_push( names, path_merge( subdirs[id], subnames[in] ) );

		string_array_deallocate( subnames );
		string_deallocate( subpath );
	}

	memory_context_pop();

	return names;
}


void fs_post_event( foundation_event_id id, const char* path, unsigned int pathlen )
{
	if( !pathlen )
		pathlen = string_length( path );
	event_post( fs_event_stream(), SYSTEM_FOUNDATION, id, pathlen + 1, 0, path, 0 );
}


event_stream_t* fs_event_stream( void )
{
	return _fs_event_stream;
}


#if FOUNDATION_PLATFORM_LINUX

typedef struct _foundation_fs_watch
{
	int      fd;
	char*    path;
} fs_watch_t;

static void _add_notify_subdir( int notify_fd, const char* path, fs_watch_t** watch_arr, char*** path_arr )
{
	char** subdirs = 0;
	char* local_path = 0;
	//log_debugf( "Watching subdir: %s", path );
	int fd = inotify_add_watch( notify_fd, path, IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVE );
	if( fd < 0 )
		return;

	//Include terminating / in paths stored in path_arr/watch_arr
	local_path = string_format( "%s/", path ? path : "" );
	array_push( (*path_arr), local_path );
	
	fs_watch_t watch;
	watch.fd = fd;
	watch.path = local_path;
	array_push( (*watch_arr), watch );	

	//Recurse
	subdirs = fs_subdirs( local_path );
	for( int i = 0, size = array_size( subdirs ); i < size; ++i )
	{
		char* subpath = path_merge( local_path, subdirs[i] );
		_add_notify_subdir( notify_fd, subpath, watch_arr, path_arr );
		string_deallocate( subpath );
	}
	string_array_deallocate( subdirs );
}

static fs_watch_t* _lookup_watch( fs_watch_t* watch_arr, int fd )
{
	//TODO: If array is kept sorted on fd, this could be made faster
	for( int i = 0, size = array_size( watch_arr ); i < size; ++i )
	{
		if( watch_arr[i].fd == fd )
			return watch_arr + i;
	}
	return 0;
}

#endif

void* _fs_monitor( object_t thread, void* monitorptr )
{
	fs_monitor_t* monitor = monitorptr;

#if FOUNDATION_PLATFORM_WINDOWS

	HANDLE handles[2];
	DWORD buffer_size = 63 * 1024;
	DWORD out_size = 0;
	OVERLAPPED overlap;
	BOOL success = FALSE;
	HANDLE dir = 0;
	unsigned int wait_status = 0;
	wchar_t* wfpath = 0;
	void* buffer = 0;
	
	memory_context_push( MEMORYCONTEXT_STREAM );

	buffer = memory_allocate( buffer_size, 8, MEMORY_PERSISTENT );
	
	handles[0] = mutex_event_object( monitor->signal );
	handles[1] = CreateEvent( 0, FALSE, FALSE, 0 );

	if( handles[1] == INVALID_HANDLE_VALUE )
	{
		log_warnf( WARNING_SUSPICIOUS, "Unable to create event to monitor path: %s : %s", monitor->path, system_error_message( GetLastError() ) );
		goto exit_thread;
	}

#elif FOUNDATION_PLATFORM_LINUX

	int notify_fd = inotify_init();
	fs_watch_t* watch = 0;
	char** paths = 0;

	memory_context_push( MEMORYCONTEXT_STREAM );

	array_reserve( watch, 1024 );
	
	//Recurse and add all subdirs
	_add_notify_subdir( notify_fd, monitor->path, &watch, &paths );

#else

	memory_context_push( MEMORYCONTEXT_STREAM );

#endif

	log_debugf( "Monitoring file system: %s", monitor->path );

#if FOUNDATION_PLATFORM_WINDOWS
	{		
		wfpath = wstring_allocate_from_string( monitor->path, 0 );
		dir = CreateFileW( wfpath, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, 0 );
		wstring_deallocate( wfpath );
	}
	if( dir == INVALID_HANDLE_VALUE )
	{
		log_warnf( WARNING_SUSPICIOUS, "Unable to open handle for path: %s : %s", monitor->path, system_error_message( GetLastError() ) );
		goto exit_thread;
	}
	
	memset( &overlap, 0, sizeof( overlap ) );
	overlap.hEvent = handles[1];
#endif

	while( monitor->thread && !thread_should_terminate( monitor->thread ) )
	{
 #if FOUNDATION_PLATFORM_WINDOWS

		memset( &overlap, 0, sizeof( overlap ) );
		overlap.hEvent = handles[1];

		out_size = 0;

		success = ReadDirectoryChangesW( dir, buffer, buffer_size, TRUE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE, &out_size, &overlap, 0 );
		if( !success )
		{
 			log_warnf( WARNING_SUSPICIOUS, "Unable to read directory changes for path: %s : %s", monitor->path, system_error_message( GetLastError() ) );
			goto exit_thread;
		}
 
		//log_debugf( "Read directory changes for path: %s", monitor->path );

		wait_status = WaitForMultipleObjects( 2, handles, FALSE, INFINITE );
 
		switch( wait_status ) 
		{ 
			case WAIT_OBJECT_0:
			{
				//Signal thread
				continue;
			}
 
			case WAIT_OBJECT_0+1:
			{
				DWORD transferred = 0;
			
				//File system change
				//log_debugf( "File system changed: %s", monitor->path );

				success = GetOverlappedResult( dir, &overlap, &transferred, FALSE );
				if( !success )
				{
					log_warnf( WARNING_SUSPICIOUS, "Unable to read directory changes for path: %s : %s", monitor->path, system_error_message( GetLastError() ) );
				}
				else
				{
					//log_debugf( "File system changed: %s (%d bytes)", monitor->path, transferred );

					PFILE_NOTIFY_INFORMATION info = buffer;
					do
					{
						int numchars = info->FileNameLength / sizeof( wchar_t );
						wchar_t term = info->FileName[ numchars ];
						char* utfstr;
						char* fullpath;
						foundation_event_id event = 0;
						
						info->FileName[ numchars ] = 0;
						utfstr = string_allocate_from_wstring( info->FileName, 0 );
						fullpath = path_merge( monitor->path, utfstr );

						switch( info->Action )
						{
							case FILE_ACTION_ADDED:     event = FOUNDATIONEVENT_FILE_CREATED; break;
							case FILE_ACTION_REMOVED:   event = FOUNDATIONEVENT_FILE_DELETED; break;
							case FILE_ACTION_MODIFIED:  event = FOUNDATIONEVENT_FILE_MODIFIED; break;
						
								//Treat rename as delete/add pair
							case FILE_ACTION_RENAMED_OLD_NAME: event = FOUNDATIONEVENT_FILE_DELETED; break;
							case FILE_ACTION_RENAMED_NEW_NAME: event = FOUNDATIONEVENT_FILE_CREATED; break;

							default: break;
						}
						
						//log_debugf( "File system changed: %s (action %d)", utfstr, info->Action );

						if( event )
							fs_post_event( event, fullpath, 0 );
						
						string_deallocate( utfstr );
						string_deallocate( fullpath );

						info->FileName[ numchars ] = term;

						info = info->NextEntryOffset ? (PFILE_NOTIFY_INFORMATION)((char*)info + info->NextEntryOffset) : 0;
					} while( info );
				}
				break;
			}

			case WAIT_TIMEOUT:
			default: 
				break;
		}

#elif FOUNDATION_PLATFORM_LINUX

		//Not ideal implementation, would really want to watch both signal and inotify fd at the same time
		int avail = 0;
		int ret = ioctl( notify_fd, FIONREAD, &avail );
		//log_debugf( "ioctl inotify: %d", avail );
		if( avail > 0 )
		{
			alignedptr64_t buffer = memory_allocate( avail, 8, MEMORY_TEMPORARY );
			int offset = 0;
			avail = read( notify_fd, buffer, avail );
			struct inotify_event* event = (struct inotify_event*)buffer;
			while( offset < avail )
			{
				foundation_event_id eventid = 0;
				fs_watch_t* curwatch = _lookup_watch( watch, event->wd );
				if( !curwatch )
					goto skipwatch;

				//log_debugf( "inotify event: %d %d bytes: %s", event->mask, event->len, event->name );

				char* curpath = string_clone( curwatch->path );
				curpath = string_append( curpath, event->name );

				if( ( event->mask & IN_CREATE ) || ( event->mask & IN_MOVED_TO ) )
				{
					//log_debugf( "  IN_CREATE : %s", curpath );
					fs_post_event( FOUNDATIONEVENT_FILE_CREATED, curpath, 0 );

					if( fs_is_directory( curpath ) )
						_add_notify_subdir( notify_fd, curpath, &watch, &paths );
				}
				if( ( event->mask & IN_DELETE ) || ( event->mask & IN_MOVED_FROM ) )
				{
					//log_debugf( "  IN_DELETE : %s", curpath );
					fs_post_event( FOUNDATIONEVENT_FILE_DELETED, curpath, 0 );
				}
				if( event->mask & IN_MODIFY )
				{
					//log_debugf( "  IN_MODIFY : %s", curpath );
					fs_post_event( FOUNDATIONEVENT_FILE_MODIFIED, curpath, 0 );
				}
				/* Moved events are also notified as CREATE/DELETE with cookies, so ignore for now
				if( event->mask & IN_MOVED_FROM )
				{
					//log_debugf( "  IN_MOVED_FROM : %s", curpath );
					fs_post_event( FOUNDATIONEVENT_FILE_DELETED, curpath, 0 );
				}
				if( event->mask & IN_MOVED_TO )
				{
					//log_debugf( "  IN_MOVED_TO : %s", curpath );
					fs_post_event( FOUNDATIONEVENT_FILE_CREATED, curpath, 0 );
				}*/

				string_deallocate( curpath );

			skipwatch:

				offset += event->len + sizeof( struct inotify_event );
				event = (struct inotify_event*)pointer_offset( buffer, offset );
			}
			memory_deallocate( buffer );
		}
		if( monitor->signal )
		{
			if( mutex_wait( monitor->signal, 100 ) )
				mutex_unlock( monitor->signal );
		}
#else
		log_debugf( "Filesystem watcher not implemented on this platform" );
		//Not implemented yet, just wait for signal to simulate thread
		if( mutex_wait( monitor->signal, 0 ) )
			mutex_unlock( monitor->signal );
#endif
	}

	log_debugf( "Stopped monitoring file system: %s", monitor->path );

#if FOUNDATION_PLATFORM_WINDOWS
	exit_thread:

	CloseHandle( dir );

	if( handles[1] )
		CloseHandle( handles[1] );

	memory_deallocate( buffer );
#elif FOUNDATION_PLATFORM_LINUX
	close( notify_fd );
	string_array_deallocate( paths );
	array_deallocate( watch );
#endif

	memory_context_pop();

	return 0;
}


static FILE* _fs_file_fopen( const char* path, unsigned int mode, bool* dotrunc )
{
#if FOUNDATION_PLATFORM_WINDOWS
	const wchar_t* modestr;
	wchar_t* wpath;
	FILE* fd;
#else
	const char* modestr;
#endif
	if( mode & STREAM_IN )
	{
		if( mode & STREAM_OUT )
		{
			if( mode & STREAM_TRUNCATE )
#if FOUNDATION_PLATFORM_WINDOWS
				modestr = L"w+b";
#else
				modestr = "w+b";
#endif
			else
#if FOUNDATION_PLATFORM_WINDOWS
				modestr = L"r+b";
#else
				modestr = "r+b";
#endif
		}
		else
		{
#if FOUNDATION_PLATFORM_WINDOWS
			modestr = L"rb";
#else
			modestr = "rb";
#endif
			if( ( mode & STREAM_TRUNCATE ) && dotrunc )
				*dotrunc = true;
		}
	}
	else
	{
#if FOUNDATION_PLATFORM_WINDOWS
		modestr = L"wb";
#else
		modestr = "wb";
#endif
	}

#if FOUNDATION_PLATFORM_WINDOWS
	wpath = wstring_allocate_from_string( path, 0 );
	fd = _wfsopen( wpath, modestr, ( mode & STREAM_OUT ) ? _SH_DENYWR : _SH_DENYNO );
	wstring_deallocate( wpath );
	return fd;
#elif FOUNDATION_PLATFORM_POSIX
	return fopen( path, modestr );
#else
#  error Not implemented
	return 0;
#endif
}


static int64_t _fs_file_tell( stream_t* stream )
{
	if( !stream || ( stream->type != STREAMTYPE_FILE ) || ( GET_FILE( stream )->fd == 0 ) )
		return -1;
	return (int64_t)ftell( (FILE*)GET_FILE( stream )->fd );
}


static void _fs_file_seek( stream_t* stream, int64_t offset, stream_seek_mode_t direction )
{
	if( !stream || ( stream->type != STREAMTYPE_FILE ) || ( GET_FILE( stream )->fd == 0 ) )
		return;
	fseek( (FILE*)GET_FILE( stream )->fd, (long)offset, ( direction == STREAM_SEEK_BEGIN ) ? SEEK_SET : ( ( direction == STREAM_SEEK_END ) ? SEEK_END : SEEK_CUR ) );
}


static bool _fs_file_eos( stream_t* stream )
{
	/*stream_file_t* file;
	int64_t cur;
	bool iseos;*/

	if( !stream || ( stream->type != STREAMTYPE_FILE ) || ( GET_FILE( stream )->fd == 0 ) )
		return true;

	return ( feof( GET_FILE( stream )->fd ) != 0 );
	
	/*file = GET_FILE( stream );
	cur = _fs_file_tell( stream );
	fseek( (FILE*)file->fd, 0, SEEK_END );

	iseos = ( cur >= _fs_file_tell( stream ) );
	if( !iseos )
		fseek( (FILE*)file->fd, (long)cur, SEEK_SET );

	return iseos;*/
}


static uint64_t _fs_file_size( stream_t* stream )
{
	int64_t cur;
	int64_t size;

	if( !stream || ( stream->type != STREAMTYPE_FILE ) || ( GET_FILE( stream )->fd == 0 ) )
		return 0;

	cur = _fs_file_tell( stream );
	_fs_file_seek( stream, 0, STREAM_SEEK_END );
	size = _fs_file_tell( stream );
	_fs_file_seek( stream, cur, STREAM_SEEK_BEGIN );

	return size;
}


static void _fs_file_truncate( stream_t* stream, uint64_t length )
{
	int64_t cur;
	stream_file_t* file;
	bool has_protocol;
#if FOUNDATION_PLATFORM_WINDOWS
	HANDLE fd;
	wchar_t* wpath;
#endif

	if( !stream || !( stream->mode & STREAM_OUT ) || ( stream->type != STREAMTYPE_FILE ) || ( GET_FILE( stream )->fd == 0 ) )
		return;

	if( (uint64_t)length >= _fs_file_size( stream ) )
		return;

	cur = _fs_file_tell( stream );
	if( cur > (int64_t)length )
		cur = length;
	
	file = GET_FILE( stream );
	if( file->fd )
	{
		fclose( (FILE*)file->fd );
		file->fd = 0;
	}

	has_protocol = string_equal_substr( file->path, "file://", 7 );

#if FOUNDATION_PLATFORM_WINDOWS
	wpath = wstring_allocate_from_string( has_protocol ? file->path + 7 : file->path, 0 );
	fd = CreateFileW( wpath, GENERIC_WRITE, FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0 );
	wstring_deallocate( wpath );
	if( length < 0xFFFFFFFF )
		SetFilePointer( fd, (LONG)length, 0, FILE_BEGIN );
	else
	{
		LONG high = (LONG)( length >> 32LL );
		SetFilePointer( fd, (LONG)length, &high, FILE_BEGIN );
	}
	SetEndOfFile( fd );
	CloseHandle( fd );
#elif FOUNDATION_PLATFORM_POSIX
	int fd = open( has_protocol ? file->path + 7 : file->path, O_RDWR );
	if( ftruncate( fd, length ) < 0 )
		log_warnf( WARNING_SUSPICIOUS, "Unable to truncate real file: %s", file->path );
	close( fd );
#else
#  error Not implemented
#endif

	file->fd = _fs_file_fopen( has_protocol ? file->path + 7 : file->path, stream->mode, 0 );

	_fs_file_seek( stream, cur, STREAM_SEEK_BEGIN );

	//FOUNDATION_ASSERT( file_size( file ) == length );
}


static void _fs_file_flush( stream_t* stream )
{
	if( !stream || ( stream->type != STREAMTYPE_FILE ) || ( GET_FILE( stream )->fd == 0 ) )
		return;
	fflush( (FILE*)GET_FILE( stream )->fd );
}


static uint64_t _fs_file_read( stream_t* stream, void* buffer, uint64_t num_bytes )
{
	stream_file_t* file;
	int64_t beforepos;
	size_t size;
	uint64_t was_read = 0;

	if( !stream || !( stream->mode & STREAM_IN ) || ( stream->type != STREAMTYPE_FILE ) || ( GET_FILE( stream )->fd == 0 ) )
		return 0;

	file = GET_FILE( stream );
	beforepos = _fs_file_tell( stream );

	size = (size_t)( num_bytes & 0xFFFFFFFFULL );
	was_read = fread( buffer, 1, size, (FILE*)file->fd );

	//if( num_bytes > 0xFFFFFFFFULL )
	//	was_read += fread( (void*)( (char*)buffer + size ), 0xFFFFFFFF, (size_t)( num_bytes >> 32LL ), (FILE*)_fd );

	if( was_read > 0 )
		return was_read;

	if( feof( (FILE*)file->fd ) )
	{
		was_read = ( _fs_file_tell( stream ) - beforepos );
		return was_read;
	}

	return 0;
}


static uint64_t _fs_file_write( stream_t* stream, const void* buffer, uint64_t num_bytes )
{
	stream_file_t* file;
	size_t size;
	uint64_t was_written = 0;

	if( !stream || !( stream->mode & STREAM_OUT ) || ( stream->type != STREAMTYPE_FILE ) || ( GET_FILE( stream )->fd == 0 ) )
		return 0;

	file = GET_FILE( stream );
	size = (size_t)( num_bytes & 0xFFFFFFFFLL );
	was_written = fwrite( buffer, 1, size, (FILE*)file->fd );

	//if( num_bytes > 0xFFFFFFFFLL )
	//	was_written += fwrite( (const void*)( (const char*)buffer + size ), 0xFFFFFFFF, (size_t)( num_bytes >> 32LL ), (FILE*)file->fd );

	if( was_written > 0 )
		return was_written;

	return 0;
}


static uint64_t _fs_file_last_modified( const stream_t* stream )
{
	return fs_last_modified( GET_FILE_CONST( stream )->path );
}


static uint64_t _fs_file_available_read( stream_t* stream )
{
	int64_t size = (int64_t)_fs_file_size( stream );
	int64_t cur = _fs_file_tell( stream );

	if( size > cur )
		return size - cur;

	return 0;
}


static stream_t* _fs_file_clone( stream_t* stream )
{
	stream_file_t* file = GET_FILE( stream );
	return fs_open_file( file->path, file->mode );
}


void _fs_file_close( stream_t* stream )
{
	stream_file_t* file = GET_FILE( stream );
	if( !file || ( stream->type != STREAMTYPE_FILE ) )
		return;

	if( file->mode & STREAM_SYNC )
	{
		_fs_file_flush( stream );
		if( file->fd )
		{
#if FOUNDATION_PLATFORM_WINDOWS
			_commit( _fileno( (FILE*)file->fd ) );
#elif FOUNDATION_PLATFORM_MACOSX
			fcntl( fileno( (FILE*)file->fd ), F_FULLFSYNC, 0 );
#else
			fsync( fileno( (FILE*)file->fd ) );
#endif
		}
	}

	if( file->fd )
	{
		fclose( (FILE*)file->fd );
		file->fd = 0;
	}

	if( file->path )
		file->path[0] = 0;
}


const char* _fs_file_path( const stream_t* stream )
{
	const stream_file_t* file = GET_FILE_CONST( stream );
	if( !file || ( stream->type != STREAMTYPE_FILE ) )
		return 0;
	return file->path;
}


stream_t* fs_open_file( const char* path, unsigned int mode )
{
	stream_file_t* file;
	stream_t* stream;
	char* abspath;
	bool dotrunc, atend, has_protocol;
	unsigned int in_mode = mode;
	unsigned int pathlen;

	if( !path )
		return 0;

	if( !mode )
		mode = STREAM_IN | STREAM_BINARY;
	if( !( mode & STREAM_IN ) && !( mode & STREAM_OUT ) )
		mode |= STREAM_IN;

	pathlen = string_length( path );
	file = memory_allocate_zero_context( MEMORYCONTEXT_STREAM, sizeof( stream_file_t ), 0, MEMORY_PERSISTENT );
	stream = GET_STREAM( file );
	_stream_initialize( stream, BUILD_DEFAULT_STREAM_BYTEORDER );

	stream->type = STREAMTYPE_FILE;

	abspath = path_make_absolute( path );
	dotrunc = false;
	has_protocol = string_equal_substr( abspath, "file://", 7 );

	file->fd = _fs_file_fopen( has_protocol ? abspath + 7 : abspath, mode, &dotrunc );

	if( ( mode & STREAM_OUT ) && !file->fd && !( mode & STREAM_TRUNCATE ) )
	{
		string_deallocate( abspath );
		stream_deallocate( stream );
		return fs_open_file( path, in_mode | STREAM_TRUNCATE );
	}

	atend = ( ( mode & STREAM_ATEND ) != 0 );

	stream->mode   = mode & ( STREAM_OUT | STREAM_IN | STREAM_BINARY | STREAM_SYNC );
	stream->path   = has_protocol ? abspath : string_prepend( abspath, "file://" );
	stream->vtable = &_fs_file_vtable;

	if( !file->fd )
	{
		//log_debugf( "Unable to open file: %s (mode %d): %s", file->path, in_mode, system_error_message( 0 ) );
		stream_deallocate( stream );
		return 0;
	}
	
	if( dotrunc )
		stream_truncate( stream, 0 );

	if( atend )
		stream_seek( stream, 0, STREAM_SEEK_END );

	return stream;
}


FOUNDATION_EXTERN void _ringbuffer_stream_initialize( void );
FOUNDATION_EXTERN void _buffer_stream_initialize( void );
#if FOUNDATION_PLATFORM_ANDROID
FOUNDATION_EXTERN void _asset_stream_initialize( void );
#endif
FOUNDATION_EXTERN void _pipe_stream_initialize( void );


int _fs_initialize( void )
{
	_fs_event_stream = event_stream_allocate( 512 );

	_fs_file_vtable.read = _fs_file_read;
	_fs_file_vtable.write = _fs_file_write;
	_fs_file_vtable.eos = _fs_file_eos;
	_fs_file_vtable.flush = _fs_file_flush;
	_fs_file_vtable.truncate = _fs_file_truncate;
	_fs_file_vtable.size = _fs_file_size;
	_fs_file_vtable.seek = _fs_file_seek;
	_fs_file_vtable.tell = _fs_file_tell;
	_fs_file_vtable.lastmod = _fs_file_last_modified;
	_fs_file_vtable.buffer_read = 0;
	_fs_file_vtable.available_read = 0;
	_fs_file_vtable.deallocate = _fs_file_close;
	_fs_file_vtable.clone = _fs_file_clone;

	_ringbuffer_stream_initialize();
	_buffer_stream_initialize();
#if FOUNDATION_PLATFORM_ANDROID
	_asset_stream_initialize();
#endif
	_pipe_stream_initialize();

	return 0;
}


void _fs_shutdown( void )
{
	int mi, msize;
	for( mi = 0, msize = array_size( _fs_monitors ); mi < msize; ++mi )
		_fs_stop_monitor( _fs_monitors + mi );
	array_deallocate( _fs_monitors );
	_fs_monitors = 0;

	event_stream_deallocate( _fs_event_stream );
	_fs_event_stream = 0;
}
