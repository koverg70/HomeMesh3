#ifndef __DAYLIGHT_H__
#define __DAYLIGHT_H__

/*--------------------------------------------------------------------------
  FUNC: 6/11/11 - Returns day of week for any given date
  PARAMS: year, month, date
  RETURNS: day of week (0-7 is Sun-Sat)
  NOTES: Sakamoto's Algorithm
    http://en.wikipedia.org/wiki/Calculating_the_day_of_the_week#Sakamoto.27s_algorithm
    Altered to use char when possible to save microcontroller ram
--------------------------------------------------------------------------*/
char dow(int y, char m, char d)
   {
       static char t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
       y -= m < 3;
       return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
   }

/*--------------------------------------------------------------------------
  FUNC: 6/11/11 - Returns the date for Nth day of month. For instance,
    it will return the numeric date for the 2nd Sunday of April
  PARAMS: year, month, day of week, Nth occurence of that day in that month
  RETURNS: date
  NOTES: There is no error checking for invalid inputs.
--------------------------------------------------------------------------*/
char NthDate(int year, char month, char DOW, char NthWeek){
  char targetDate = 1;
  char firstDOW = dow(year,month,targetDate);
  while (firstDOW != DOW){
    firstDOW = (firstDOW+1)%7;
    targetDate++;
  }
  //Adjust for weeks
  targetDate += (NthWeek-1)*7;
  return targetDate;
}

char lastSunday(int year, char month)
{
	char switchd = NthDate(year, month, 0, 5);
	if (switchd > 31)
	{
		switchd = NthDate(year, month, 0, 4);
	}
}

bool IsDST(int year, int month, int day)
{
	//January, february, and december are out.
	if (month < 3 || month > 10) { return false; }
	//April to October are in
	if (month > 3 && month < 10) { return true; }
	char switchd = lastSunday(year, month);
	if (day >= switchd)
	{
		return month == 3 ? true : false;
	}
	else
	{
		return month == 3 ? false : true;
	}
}

#endif // __DAYLIGHT_H__
