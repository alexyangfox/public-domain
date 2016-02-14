
extern void cpass(complex *,const complex *,const complex *,unsigned int);
extern void upass(complex *,const complex *,const complex *,unsigned int);
extern void rpass(real *,const complex *,unsigned int);
extern void vpass(real *,const complex *,unsigned int);

extern const complex d16[];
extern const complex d32[];
extern const complex d64[];
extern const complex d128[];
extern const complex d256[];
extern const complex d512[];
extern const complex d1024[];
extern const complex d2048[];
extern const complex d4096[];
extern const complex d8192[];
extern const complex d16384[];
extern const complex d32768[];
extern const complex d65536[];

#define sqrthalf (d16[1].re)
