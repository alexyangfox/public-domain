// This defines the random.h files missing from SYS 5
extern "C" {
	void Berkeley_srandom(int);
	char *Berkeley_initstate(unsigned, char*, int);
	long Berkeley_random();
	char *Berkeley_setstate(char*);
     }
