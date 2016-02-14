#include <kernel/types.h>
#include <kernel/vm.h>
#include <kernel/proc.h>
#include <kernel/timer.h>
#include <kernel/dbg.h>
#include <kernel/error.h>


static int days_in_month_n[12] =
					 	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


/*
 * LocalTime();
 */

int32 LocalTime (struct Tm *tm, struct TimeVal *tv)
{
	uint32 days_since_epoch;
	uint32 month;
	uint32 year;
	uint32 day_of_year;
	uint32 days_in_month;
	uint32 month_day_cnt;
	
	
	days_since_epoch = tv->seconds / 86400;
		
	year = DaysToYear ((int32) days_since_epoch + DAYS_TO_1970, (int32 *) &day_of_year);
	
		
	for (month_day_cnt = 0, month = 0; month < 12; month ++)
	{
		days_in_month = days_in_month_n[month];
		
		if (month == 1 && IsLeap(year))
			days_in_month +=1;

		if ((month_day_cnt + days_in_month) > day_of_year)
			break;
		
		month_day_cnt += days_in_month;
	}
	
	
	tm->year = year - 1900;	
	tm->yday = day_of_year;
	tm->mon = month;
	tm->mday = (day_of_year - month_day_cnt) + 1;

	tm->sec = tv->seconds % 60;
	tm->min = (tv->seconds/60) % 60;
	tm->hour = (tv->seconds/3600) % 24;
	tm->wday = (days_since_epoch + 4) % 7;

	return 0;
}




/*
 * MakeTime();
 */
 
uint32 MakeTime (struct Tm *tm)
{
	uint32 month;
	uint32 total_years_seconds, total_months_seconds;
	uint32 last_month_seconds, last_day_seconds;
	uint32 seconds_since_epoch;
	

	/* Year seconds */
	
	total_years_seconds = (YearToDays(tm->year + 1900) - DAYS_TO_1970) * 86400;
	
	
	/* Month seconds */
	
	total_months_seconds = 0;
		

	month = tm->mon;
	
	if (month > 1)
	{
		total_months_seconds = 86400 * (((month-2) * 367)/12);
			
		if (IsLeap(tm->year + 1900))
			total_months_seconds += 86400 * 60;
		else
			total_months_seconds += 86400 * 59;
	}
	else
	{
		total_months_seconds = month * 31 * 86400;
	}
	
		
		
	/* Days of month seconds (day-1 as no zero day) */
	
	last_month_seconds = (tm->mday - 1) * 86400;

	
	/* Last day seconds */
	
	last_day_seconds = (tm->hour * 3600)  + (tm->min * 60) + tm->sec;
	
	seconds_since_epoch = total_years_seconds + total_months_seconds +
								last_month_seconds + last_day_seconds;

	return seconds_since_epoch;
}




/*
 * IsLeap();
 */

int IsLeap (uint32 year)
{
	return (((year%4) == 0) && (((year%100) != 0) || ((year%400) == 0)));
}




/*
 * YearToDays()  Since 1/1/0000  (start of 1 BC)
 */

int32 YearToDays (int32 year)
{
	int32 days;
	int32 centuries;
	int32 four_years;
	int32 y;
	
	y = year;
	
	

	days = (year / 400) * DAYS_IN_4CENTURIES;

	/* Handle remaining centuries */
	
	year = year % 400;
	centuries = year / 100;

	if (centuries != 0)
	{
		days += DAYS_IN_FIRST_CENTURY;
		days += DAYS_IN_CENTURY * (centuries - 1);
	}
		
	
	/* Handle Last century, first as groups of 4 years */
		
	year = year % 100;
	four_years = year / 4;
		
	if (four_years != 0)
	{
		if (IsLeap ((y/100)*100))
			days += DAYS_IN_4YEARS + 1;
		else
			days += DAYS_IN_4YEARS;
		
		days += DAYS_IN_4YEARS * (four_years - 1);
	}
	
	
	/* Handle last 4 years */
	
	year = year % 4;
		
	if (year != 0)
	{
		if (IsLeap ((y/4)*4))
			days += 366;
		else
			days += 365;
		
		days += 365 * (year - 1);		
	}
	
	
	return days;
}




/*
 * DaysToYear();
 */
 
int32 DaysToYear (int32 days, int32 *days_remaining)
{
	int32 year;
	
	year = (days / DAYS_IN_4CENTURIES) * 400;
	days %= DAYS_IN_4CENTURIES;
	
	
	if (days != 0)
	{
		if (days >= DAYS_IN_FIRST_CENTURY)
		{
			year += 100;
			days -= DAYS_IN_FIRST_CENTURY;
		}
		
		year += (days / DAYS_IN_CENTURY) * 100;
		days %= DAYS_IN_CENTURY;
	}
	
	
	if (days != 0)
	{
		if (IsLeap (year) && days >= DAYS_IN_4YEARS + 1)
		{
			year +=4;
			days -= (DAYS_IN_4YEARS + 1);
		}
		else if (days >= DAYS_IN_4YEARS)
		{
			year +=4;
			days -= DAYS_IN_4YEARS;
		}
		
		year += (days / DAYS_IN_4YEARS) * 4;
		days %= DAYS_IN_4YEARS;
	}
	
	
	if (days != 0)
	{
		if (IsLeap (year) && days >= 366)
		{
			year += 1;
			days -= 366;
		}
		else if (days >= 365)
		{
			year += 1;
			days -= 365;
		}
		
		year += (days / 365);
		days %= 365;
		
	}
	
	if (days_remaining != NULL)
		*days_remaining = days;
	
	return year;
}




/*
 * ValidateTime();
 */

int32 ValidateTime (struct Tm *tm)
{
	SetError (ENOSYS);
	return -1;
}




/*
 * DiffTime();
 */
 
int32 DiffTime (struct TimeVal *start, struct TimeVal *end, struct TimeVal *result)
{
	result->seconds = end->seconds - start->seconds;
	result->microseconds = end->microseconds - start->microseconds;
	
	if (result->microseconds < 0)
	{
		result->seconds --;
		result->microseconds += 1000000;
	}
	
	return 0;
}




/*
 * AddTime();
 */
 
int32 AddTime (struct TimeVal *start, struct TimeVal *end, struct TimeVal *result)
{
	result->seconds = end->seconds + start->seconds;
	result->microseconds = end->microseconds + start->microseconds;
	
	if (result->microseconds > 1000000)
	{
		result->seconds ++;
		result->microseconds -= 1000000;
	}

	return 0;
}




/*
 * CompareTime();
 *
 * return  0  if start == end
 * return -1  if start > end
 * return +1  if start < end
 */

int32 CompareTime (struct TimeVal *start, struct TimeVal *end)
{
	if (start->seconds == end->seconds)
	{
		if (start->microseconds == end->microseconds)
			return 0;
		else if (start->microseconds > end->microseconds)
			return -1;
		else
			return +1;
	}
	else if (start->seconds > end->seconds)
		return -1;
	else
		return +1;
}
