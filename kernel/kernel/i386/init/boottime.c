#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/i386/i386.h>
#include <kernel/timer.h>
#include <kernel/dbg.h>
#include <kernel/i386/init.h>

/*
**  GetBootTime();
*/

uint32 GetBootTime (void)
{
	uint32 i;
	struct Tm tm;
	 
	for (i=0; i <10000000; i++)
		if ((CMOS_READ (CMOS_CTRL_REG_A) & RTC_UIP) == 0)
			break;

	tm.sec 	= CMOS_READ (CMOS_SECONDS);
	tm.min 	= CMOS_READ (CMOS_MINUTES);
	tm.hour	= CMOS_READ (CMOS_HOURS);
	tm.mday = CMOS_READ (CMOS_DATE_OF_MONTH);
	tm.mon 	= CMOS_READ (CMOS_MONTH);
	tm.year	= CMOS_READ (CMOS_YEAR);
	

	if (!(CMOS_READ (CMOS_CTRL_REG_B) & RTC_DM_BINARY))
	{
		BCD_TO_BIN (tm.sec);
		BCD_TO_BIN (tm.min);
		BCD_TO_BIN (tm.hour);
		BCD_TO_BIN (tm.mday);
		BCD_TO_BIN (tm.mon);
		BCD_TO_BIN (tm.year);
	}
	if ((tm.year += 1900) < 1970)
		tm.year += 100;
	
	tm.year -= 1900;
	tm.mon -=1;
	
	return MakeTime(&tm);
}

