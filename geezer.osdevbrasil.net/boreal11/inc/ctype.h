#ifndef __TL_CTYPE_H
#define	__TL_CTYPE_H

#ifdef __cplusplus
extern "C"
{
#endif

/* IMPORTS
from LIB/CTYPE/CTYPE.C */
extern unsigned char g_ctype[];

#define CT_UP	0x01	/* upper case */
#define CT_LOW	0x02	/* lower case */
#define CT_PUN	0x04	/* punctuation */
#define CT_WHT	0x08	/* white space (space/cr/lf/tab) */
/* are any of these likely to be valid outside of ASCII? are there
any Unicode digits, control characters, hex digits, or hard spaces? */
#define CT_DIG	0x10	/* digit */
#define CT_HEX	0x20	/* hex digit */
#define CT_CTL	0x40	/* control */
#define CT_SP	0x80	/* hard space (0x20) */

/* without the cast to unsigned, DJGPP complains (using -Wall) */
#define isalnum(c)	(g_ctype[(unsigned)(c)] & (CT_UP | CT_LOW | CT_DIG))
#define isalpha(c)	(g_ctype[(unsigned)(c)] & (CT_UP | CT_LOW))
#define isascii(c)	((unsigned)(c) <= 0x7F)
#define iscntrl(c)	(g_ctype[(unsigned)(c)] & CT_CTL)
#define isdigit(c)	(g_ctype[(unsigned)(c)] & CT_DIG)
#define isgraph(c)	(g_ctype[(unsigned)(c)] & (CT_PUN | CT_UP | CT_LOW | CT_DIG))
#define islower(c)	(g_ctype[(unsigned)(c)] & CT_LOW)
#define isprint(c)	(g_ctype[(unsigned)(c)] & (CT_PUN | CT_UP | CT_LOW | CT_DIG | CT_SP))
#define ispunct(c)	(g_ctype[(unsigned)(c)] & CT_PUN)
#define isspace(c)	(g_ctype[(unsigned)(c)] & CT_WHT)
#define isupper(c)	(g_ctype[(unsigned)(c)] & CT_UP)
#define isxdigit(c)	(g_ctype[(unsigned)(c)] & (CT_DIG | CT_HEX))

#define toascii(c)	((c) & 0x7F)
#define tolower(c)	(isupper(c) ? ((c) + 'a' - 'A') : (c))
#define toupper(c)	(islower(c) ? ((c) + 'A' - 'a') : (c))

#ifdef __cplusplus
}
#endif

#endif
