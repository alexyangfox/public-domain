#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H


#ifndef NULL
#define NULL				(void *)0
#endif

#ifndef null
#define null				(void *)0
#endif

#ifndef TRUE
#define TRUE				1
#endif

#ifndef FALSE
#define FALSE				0
#endif


#define public
#define private 			static
#define import				extern


typedef unsigned char		bool;
typedef char *				strptr;
typedef signed char 		int8;
typedef unsigned char 		uint8;

typedef signed short		int16;
typedef unsigned short		uint16;

typedef signed long			int32;
typedef unsigned long		uint32;

typedef signed long long 	int64;
typedef unsigned long long 	uint64;

typedef signed long			err32;
typedef unsigned long 		uintptr_t;


typedef unsigned int		size_t;
typedef int 				ssize_t;
typedef long				off_t;
typedef unsigned short		mode_t;

typedef	unsigned short		uid_t;
typedef	unsigned short		gid_t;
 
typedef short				dev_t;
typedef	unsigned short		ino_t;

typedef unsigned short		nlink_t;

typedef long				time_t;

typedef long 				fsblkcnt_t;
typedef long				fsfilcnt_t;



#endif

