/* IsLeapYear
 * Tests a given year to see whether it is a leap year.
 * Returns 1 if it is a leap year, or 0 otherwise.
 * A leap year is divisible by 4 but not by 100, except
 * if it is divisible by 400 it is a leap year.
 */

int IsLeapYear (int year)
{
   if((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
      return 1; /* leap */
   else
      return 0; /* no leap */
}
