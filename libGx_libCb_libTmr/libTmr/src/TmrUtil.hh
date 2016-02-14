#ifndef TMRUTIL_INCLUDED
#define TMRUTIL_INCLUDED

#include <sys/time.h>

//non-public shared components of libTmr
bool operator<(const timeval &rValOne, const timeval &rValTwo);

#endif //TMRUTIL_INCLUDED
