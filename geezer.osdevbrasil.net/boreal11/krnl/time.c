/*----------------------------------------------------------------------------
REALTIME CLOCK

EXPORTS:
void rtc_irq(void);
time_t time(time_t *timer);



GLOSSARY:
BC/BCE: Before Christ/Before Current Era

AD/CE: Anno Domini ("Year of the Lord")/Current Era

Leap year: 366 days instead of 365. The extra day is February 29.

Julian calendar: it's a leap year if the year is divisible by 4
(365.25 days per year). OBSOLETE; replaced in the 16th century
by the Gregorian calendar.

Gregorian calendar: it's a leap year if the year is divisible by 4,
UNLESS the year is also divisible by 100, UNLESS the year is also
divisible by 400 (365.2425 days per year)

Proleptic: date expressed in a calendar before that calendar was
invented. Since the Gregorian calendar was introduced on
October 15, 1582 CE, Gregorian dates before this are proleptic.

Julian Day Number (JDN): days since November 24, 4714 BCE Gregorian.
Used in astronomy.

time_t: UNIX format for time; seconds since midnight on Jan 1, 1970
----------------------------------------------------------------------------*/
#include <system.h> /* inportb(), outportb() */
#include <string.h> /* NULL */
#include <time.h> /* time_t */
#include "_krnl.h"

/* IMPORTS
from THREADS.C */
int sleep_on(wait_queue_t *queue, unsigned *timeout);
void wake_up(wait_queue_t *queue);

static wait_queue_t g_rtc_wait_queue;
/*****************************************************************************
*****************************************************************************/
void rtc_irq(void)
{
	unsigned save;

/* if time() is sleeping, wake it up */
	wake_up(&g_rtc_wait_queue);
/* save value at port 0x70 */
	save = inportb(0x70);
/* acknowledge IRQ8 at the RTC by reading register C */
	outportb(0x70, 0x0C);
	(void)inportb(0x71);
/* restore port 0x70 value */
	outportb(0x70, save);
}
/*****************************************************************************
*****************************************************************************/
static unsigned read_cmos(unsigned reg, char bcd)
{
	unsigned high_digit, low_digit;

	outportb(0x70, reg);
	high_digit = low_digit = inportb(0x71);
	if(!bcd)
		return low_digit;
/* convert from BCD to binary */
	high_digit >>= 4;
	high_digit &= 0x0F;
	low_digit &= 0x0F;
	return 10 * high_digit + low_digit;
}
/*****************************************************************************
Finds the number of days between two dates in the Gregorian calendar.
- it's a leap year if the year is divisible by 4,
    - UNLESS the year is also divisible by 100,
	- UNLESS the year is also divisible by 400

To compute Julian Day Number (JDN;
days since Nov 24, 4714 BC/BCE in Gregorian calendar):
	days_between_dates(-4713, 327, curr_day_in_year, curr_year);

To compute days since Jan 1, 1970 (UNIX epoch):
	days_between_dates(1970, 0, curr_day_in_year, curr_year);
or
    days_between_dates(-4713, 327, curr_day_in_year, curr_year) + 2440588L;

This code divides the time between start_day/start_year and end_day/end_year
into "slices": fourcent (400-year) slices in the middle, bracketed on
either end by century slices, fouryear (4-year) slices, and year slices.

When used to compute JDN, this code produces the same results as the
code shown here:
	http://serendipity.magnet.ch/hermetic/cal_stud/jdn.htm

IMHO, it's easier to see how the algorithm for this code works,
versus the code at the URL above.
*****************************************************************************/
static long days_between_dates(unsigned start_year, unsigned start_day,
		int end_year, unsigned end_day)
{
	int fourcents, centuries, fouryears, years;
	long days;

	fourcents = end_year / 400 - start_year / 400;
	centuries = end_year / 100 - start_year / 100 -
/* subtract from 'centuries' the centuries already accounted for by
'fourcents' */
		fourcents * 4;
	fouryears = end_year / 4 - start_year / 4 -
/* subtract from 'fouryears' the fouryears already accounted for by
'fourcents' and 'centuries' */
		fourcents * 100 - centuries * 25;
	years = end_year - start_year -
/* subtract from 'years' the years already accounted for by
'fourcents', 'centuries', and 'fouryears' */
		400 * fourcents - 100 * centuries - 4 * fouryears;
/* add it up: 97 leap days every fourcent */
	days = (365L * 400 + 97) * fourcents;
/* 24 leap days every residual century */
	days += (365L * 100 + 24) * centuries;
/* 1 leap day every residual fouryear */
	days += (365L * 4 + 1) * fouryears;
/* 0 leap days for residual years */
	days += (365L * 1) * years;
/* residual days (need the cast!) */
	days += ((long)end_day - start_day);
/* account for terminal leap year */
	if(end_year % 4 == 0 && end_day >= 60)
	{
		days++;
		if(end_year % 100 == 0)
			days--;
		if(end_year % 400 == 0)
			days++;
	}
/* xxx - err...I forget what's going on here.
The code won't work properly without it */
	if(end_year >= 0)
	{
		days++;
		if(end_year % 4 == 0)
			days--;
		if(end_year % 100 == 0)
			days++;
		if(end_year % 400 == 0)
			days--;
	}
	if(start_year > 0)
		days--;
	return days;
}
/*****************************************************************************
month and date start with 1, not with 0
*****************************************************************************/
#define	EPOCH_YEAR	1970
#define	EPOCH_DAY	0 /* Jan 1 */

static unsigned long date_time_to_time_t(unsigned year, unsigned month,
		unsigned date, unsigned hour, unsigned min, unsigned sec)
{
	static const unsigned days_to_date[12] =
	{
/*              jan  feb  mar  apr  may  jun  jul  aug  sep  oct  nov  dec */
		0,
		31,
		31 + 28,
		31 + 28 + 31,
		31 + 28 + 31 + 30,
		31 + 28 + 31 + 30 + 31,
		31 + 28 + 31 + 30 + 31 + 30,
		31 + 28 + 31 + 30 + 31 + 30 + 31,
		31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
		31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
		31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
		31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30
	};
/**/
	unsigned long rv;
	unsigned day;

/* convert month and year to day-in-year */
	if(month < 1 || month > 12 || date < 1 || date > 31)
		return 0;
	month--;
	date--;
	day = date + days_to_date[month];
/* convert to Unix JDN (UJDN) */
	rv = days_between_dates(EPOCH_YEAR, EPOCH_DAY, year, day);
/* convert from days to seconds, adding time as you go */
	rv *= 24;
	rv += hour;
	rv *= 60;
	rv += min;
	rv *= 60;
	rv += sec;
	return rv;
}
/*****************************************************************************
NOTE: this function works only with local time, stored in the CMOS clock.
It knows nothing of GMT or timezones. This is a feature, not a bug :)
*****************************************************************************/
time_t time(time_t *timer)
{
	static signed char bcd = -1;
/**/
	unsigned portval, timeout, month, date, hour, minute, second;
	time_t rv;
	int year;

/* read register B to check for BCD or binary mode */
	if(bcd == -1)
	{
		outportb(0x70, 0x0B);
		if(inportb(0x71) & 0x04)
			bcd = 0;
		else
			bcd = 1;
	}
	while(1)
	{
/* is there an update cycle in progress? */
		outportb(0x70, 0x0A);
		if((inportb(0x71) & 0x80) == 0)
			break;
/* yes, enable end-of-update interrupt */
		outportb(0x70, 0x0B);
		portval = inportb(0x71);
		outportb(0x71, portval | 0x10);
/* sleep until end-of-update interrupt, or for 5 ms
If RTC chip has 32.768 KHz crystal, update should take about 2 ms */
		timeout = 5;
		sleep_on(&g_rtc_wait_queue, &timeout);
/* disable end-of-update interrupt
		outportb(0x70, 0x0B); */
		outportb(0x71, portval & ~0x10);
	}
/* get year/month/date
	year = read_cmos(9, bcd) + 1900;	OH NO, Y2K!
	year = read_cmos(9, bcd) + 2000;
use the Microsoft method -- this should be good from 1970-2069
signed 32-bit time_t will overflow before then, in 2038 */
	year = read_cmos(9, bcd);	/* 0-99 */
	if(year < 70)
		year += 2000;
	else
		year += 1900;
	month = read_cmos(8, bcd);	/* 1-12 */
	date = read_cmos(7, bcd);	/* 1-31 */
/* get time */
	hour = read_cmos(4, bcd);	/* 0-23 */
	minute = read_cmos(2, bcd);	/* 0-59 */
	second = read_cmos(0, bcd);	/* 0-59 */

	rv = date_time_to_time_t(year, month, date, hour, minute, second);
	if(timer != NULL)
		(*timer) = rv;
	return rv;
}
