#ifndef KERNEL_DBG_H
#define KERNEL_DBG_H


#include <kernel/types.h>
#include <stdarg.h>


/*
 * At end of most debug statements semi-colons aren't needed or will
 * cause problems compiling.
 *
 * Multi-statement Debug macros have braces surrounding them in case
 * they're used like below...
 *
 * if (condition)
 *     KPANIC (str);
 *
 *
 * Use -DNDEBUG in the makefile CFLAGS to remove debugging.
 */


#ifndef NDEBUG


#define KLOG( fmt, args...)												\
		KLog (fmt , ##args)


#define KPRINTF( fmt, args...)											\
		KLog (fmt , ##args);											


#define KLOG_ENTER														\
		KLog ("Enter: %s, %s", __FILE__, __FUNCTION__);


#define KLOG_LINE														\
		KLog ("Line: %s, %d, %s", __FILE__, __LINE__, __FUNCTION__);


#define KLOG_LEAVE														\
		KLog ("Leave: %s, %s", __FILE__, __FUNCTION__);

		
#define KASSERT(expr)													\
		{                                                               \
			if (!(expr))												\
			{															\
				KLog("@ %s, %d, %s",__FILE__, __LINE__, __FUNCTION__);	\
				KLog("KASSERT (" #expr  ") failed");					\
				KPanic();												\
			}                                                           \
		}


#define KPANIC(str)														\
		{                                                               \
			KLog("@ %s, %d, %s",__FILE__, __LINE__, __FUNCTION__);		\
			KLog("%s", str);											\
			KPanic();                                                   \
		}



#else	

#define KPRINTF( fmt, args...)
#define KLOG( fmt, args...)
#define KLOG_ENTER
#define KLOG_PLACE
#define KLOG_LEAVE
#define KASSERT( expr)
#define KPANIC( str)	KPanic ();


#endif




/*
 * Kernel debugging functions.  Shouldn't be called directly, instead use the
 * macros above.
 */

void KLog (const char *format, ...);
void KLog2 (const char *format, va_list ap);
void KPanic(void);
void KLogToScreenDisable(void);




#endif
