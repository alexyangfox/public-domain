#ifndef isgap_h
#define isgap_h

static inline bool isgap(char c)
	{
	return c == '-' || c == '.';
	}

static inline bool iswildcard(char c)
	{
	return toupper(c) == 'X' || toupper(c) == 'N';
	}

#endif // isgap_h
