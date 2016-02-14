#include "TmrUtil.hh"

bool operator<(const timeval &rValOne, const timeval &rValTwo)
{
  if(rValOne.tv_sec < rValTwo.tv_sec)
    return true;

  if(rValOne.tv_sec == rValTwo.tv_sec)
    {
      if(rValOne.tv_usec < rValTwo.tv_usec)
	return true;
      else
	return false;
    }else
      return false;
}
