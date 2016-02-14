/* $Header: time.c,v 1.2 93/11/13 01:08:46 wlott Exp $ */

/* Time support routines that are easier to do in C then in lisp. */

#include <stdio.h>
#include <time.h>
#include "lisp.h"

void get_timezone(time_t when, int *minwest, boolean *dst)
{
    struct tm ltm, gtm;
    int mw;

    ltm = *localtime(&when);
    gtm = *gmtime(&when);

    mw = ((gtm.tm_hour*60)+gtm.tm_min) - ((ltm.tm_hour*60)+ltm.tm_min);
    if ((gtm.tm_wday + 1) % 7 == ltm.tm_wday)
	mw -= 24*60;
    else if (gtm.tm_wday == (ltm.tm_wday + 1) % 7)
	mw += 24*60;
    *minwest = mw;
    *dst = ltm.tm_isdst;
}
