/*
 * $Header: purify.h,v 1.1 92/07/28 20:15:25 wlott Exp $
 */

#if !defined(_PURIFY_H_)
#define _PURIFY_H_

extern int purify(lispobj static_roots, lispobj read_only_roots);

#endif
