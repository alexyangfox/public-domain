#ifndef __TL_TIME_H
#define	__TL_TIME_H

#ifdef __cplusplus
extern "C"
{
#endif

/* unsigned; so it will overflow sometime in 2106 instead of 2038 */
typedef unsigned long	time_t;

time_t time(time_t *timer);

#ifdef __cplusplus
}
#endif

#endif
