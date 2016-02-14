/* gddcalc.h
 * Defines some externals used by GDDCALC subroutines.
 * 
 *********************************************************************
 * ALL MONTH QUANTITIES THAT APPEAR BELOW RUN FROM 1 TO 12.
 * SO WHENEVER THE VARIABLE MONTH IS USED AS AN INDEX, AS IN, SAY,
 * dayspermonth[month], WE MUST SUBTRACT 1 FROM month TO CREATE A
 * VARIABLE THAT RUNS FROM 0 TO 11. THE SAME HOLDS TRUE WHEN REFERING
 * TO THE ITEMS IN THE MONTHS COMBOBOX.
 *********************************************************************
 */

#define LOGSTART    1
#define LOGSAVE     2

int dayspermonth[12];
int active_year, active_month, active_day, active_daynum;
int active_startmonth, active_startday, active_stopmonth, active_stopday;
int active_dt, active_hour, active_minute, active_second;
float active_latitude, active_longitude;
float active_aspect, active_tilt, active_elev;
float active_sunrise, active_sunset;
float active_temp, active_press, active_timezone;

/* The month_days array is used by solpos and several routines that call
 * solpos. The latter use it to calculate daynumber from day, month, and
 * year.
 */
 
static int  month_days[2][13] = {
   {0,   0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334 },
   {0,   0,  31,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335 }};

/* northsouth = 1 means that calculations are underway for a site
 * whose latitude is greater than 43.7 degrees, which lies a little
 * south of Cottage Grove. northsouth = 2 for more southerly sites.
 */

int northsouth;
