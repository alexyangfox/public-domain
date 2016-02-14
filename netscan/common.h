#define max(a,b) (((a)>(b))?(a):(b))
#define nelem(x) (sizeof(x)/sizeof((x)[0]))

void	*emalloc(size_t);
void	*emallocz(size_t);
void	*erealloc(void *, size_t);

char	*estrdup(char *);

void	eclose(int);

int	ilen(int);

void	block(int);
void	unblock(int);
