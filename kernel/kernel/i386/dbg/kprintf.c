#include <stdarg.h>
#include <kernel/types.h>
#include <kernel/dbg.h>
#include <kernel/utility.h>
#include <kernel/proc.h>



static void KClearScreen (void);
static void KScrollScreen (void);
static void KPrintString (char *s);


#define KLOG_ENTRIES	256
#define KLOG_WIDTH   80

char klog_entry[KLOG_ENTRIES][KLOG_WIDTH];
int32 current_log;
uint8 tpen = 0x07;
uint8 *g_dbg_screen_addr = (uint8 *)0xb8000;
int g_dbg_y=0;
int	klog_to_screen = TRUE;
int log_idx = 0;

struct Mutex dbg_mutex;




/*
 * InitDebug();
 */
 
void InitDebug (void)
{
	int32 t;
	
	MutexInit (&dbg_mutex);
	
	klog_to_screen = TRUE;
	KClearScreen();
	
	current_log = 0;
	
	for (t=0; t< KLOG_ENTRIES; t++)
	{
		klog_entry[t][0] = '\0';
	}
}




/*
 * KLogToScreenDisable();
 *
 * Called upon first open() of a console driver.  Stops the kernel
 * from printing the log directly to screen.  Kernel logging continues
 * but can only be seen by pressing the F12 key.
 */

void KLogToScreenDisable(void)
{
	MutexLock (&dbg_mutex);

	klog_to_screen = FALSE;

	MutexUnlock (&dbg_mutex);
}


void KLogToScreenEnable(void)
{
	MutexLock (&dbg_mutex);

	klog_to_screen = TRUE;
	tpen = 0x07;
	KClearScreen();

	MutexUnlock (&dbg_mutex);
}




/*
 * KLog();
 *
 * Used by the macros KPRINTF, KLOG, KASSERT and KPANIC to
 * print with printf formatting to the kernel's debug log buffer.
 * The buffer is a fixed size circular buffer, Once it is full 
 * the oldest entry is overwritten with the newest entry.
 *
 * Cannot be used during interrupt handlers or with interrupts
 * disabled s the dbg_mutex must be acquired.  Same applies
 * to KPRINTF, KLOG, KASSERT and KPANIC.
 *
 * We can get away with using KLog() and above macros before
 * multitasking is enabled due to MutexLock() and MutexUnlock()
 * having a test to see if initialization is complete.
 */
 
void KLog (const char *format, ...)
{
	va_list ap;
	
	va_start (ap, format);
	
	MutexLock (&dbg_mutex);
	
	Vsnprintf (&klog_entry[current_log][0], KLOG_WIDTH, format, ap);
	
	if (klog_to_screen == TRUE)
		KPrintString (&klog_entry[current_log][0]);

	current_log++;
	current_log %= KLOG_ENTRIES;
	
	MutexUnlock (&dbg_mutex);	
	
	va_end (ap);
}



/*
 *
 */
 
void KLog2 (const char *format, va_list ap)
{
	MutexLock (&dbg_mutex);
	
	Vsnprintf (&klog_entry[current_log][0], KLOG_WIDTH, format, ap);
	
	if (klog_to_screen == TRUE)
		KPrintString (&klog_entry[current_log][0]);

	current_log++;
	current_log %= KLOG_ENTRIES;
	
	MutexUnlock (&dbg_mutex);	
}




/*
 * KPanic();
 */

void KPanic(void)
{
	int32 t, log;
	
	DisableInterrupts();

	tpen = 0x1f;
	KClearScreen();
	
	log = ((current_log - 1) + KLOG_ENTRIES) % KLOG_ENTRIES;
	
	KPrintString ("*** KPanic() ***");
	
	for (t=0; t<23; t++)
	{
		if (t < KLOG_ENTRIES)
			KPrintString (&klog_entry[log][0]);
		else
			KPrintString ("");
		
		log --;
		
		log = (log + KLOG_ENTRIES) % KLOG_ENTRIES;
	}


	while(1);
}




/*
 * KConDebug();
 *
 * Called by the console driver when F12 key is pressed.  Acquires
 * the dbg_mutex so no further log entries can be added during
 * the debugger.  Arrow up and down keys scroll through the
 * debug circular buffer.
 */

void KConDebug(int32 idx)
{
	int32 t, log;


	MutexLock (&dbg_mutex);


	if (idx == 0)
		log_idx = 0;
	else if (idx == 1)
		log_idx ++;
	else
		log_idx --;

	if (log_idx < 0) log_idx = 0;
	
	if (log_idx > KLOG_ENTRIES - 23) log_idx = KLOG_ENTRIES - 23;


	tpen = 0x1f;
	KClearScreen();
	
	log = ((current_log - 1) - log_idx + KLOG_ENTRIES) % KLOG_ENTRIES;
	
	KPrintString ("*** KConDebug ***");
	
	for (t=0; t<23; t++)
	{
		if (t < KLOG_ENTRIES)
			KPrintString (&klog_entry[log][0]);
		else
			KPrintString ("");
		
		log --;
		
		log = (log + KLOG_ENTRIES) % KLOG_ENTRIES;
	}
	
	MutexUnlock (&dbg_mutex);
}




/*
 * KPrintString();
 */

static void KPrintString (char *s)
{
	char *dst;

	if (g_dbg_y >= 25)
	{
		KScrollScreen();
		g_dbg_y = 25-1;
	}

	dst = (char *) (0xb8000 + 160 * g_dbg_y);

	while (*s != '\0')
	{
		*dst++ = *s++;
		*dst++ = tpen;
	}
	g_dbg_y ++;
}




/* 
 * ClearScreen();
 */

static void KClearScreen (void)
{
	char *p; 
	int x,y;
	
	for (y=0; y<25; y++)
	{
		for (x=0; x<80; x++)
		{
			p = (char *) (g_dbg_screen_addr + 2*x + 160 *y);
			*p = ' ';
			*(p+1) = tpen;
		}
	}
	
	g_dbg_y = 0;
}




/*
 * ScrollScreen();
 */

static void KScrollScreen (void)
{
	long x,y;
	
	for (y=0; y<24; y++)
	{
		for (x=0; x<80; x++)
		{
	
			*(uint8 *)(g_dbg_screen_addr + y*160 + x*2) =
					*(uint8 *)(g_dbg_screen_addr + (y+1)*160 + x*2);
					
			*(uint8 *)(g_dbg_screen_addr + y*160 + x*2 + 1) =
					*(uint8 *)(g_dbg_screen_addr + (y+1)*160 + x*2 + 1);
		}
	}
	
	for (x=0; x<80; x++)
	{
	
		*(uint8 *)(g_dbg_screen_addr + 24*160 + x*2) = ' ';
					
		*(uint8 *)(g_dbg_screen_addr + 24*160 + x*2 + 1) = 0x07;
	}
}




/*
 * UDebug() syscall
 */

void UDebug (int level, char *str)
{
}




