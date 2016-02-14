#ifndef asm_h
#define asm_h

#include "common.h"
#include "symbol.h"

/*
** 64- and 128-bit assembly support:
*/


extern void compactType
( 
	void *data, 
	enum PrimType *pType, 
	struct SymNode **type 
);

extern void UnsToStr( char *buf, void *unsval );
extern void IntToStr( char *buf, void *intval );

extern void notval
( 
	void *dest,
	void *src,
	enum PrimType vpt, 
	enum PrimType *vpta,
	struct SymNode **vt
);


extern void negval
( 
	void *dest,
	void *src,
	enum PrimType vpt, 
	enum PrimType *vpta,
	struct SymNode **vt
);



extern void addval
( 
	void *dest,
	void *src,
	enum PrimType *vpt,
	struct SymNode **typ
	
);


extern void subval
( 
	void *dest,
	void *src,
	enum PrimType *vpt,
	struct SymNode **typ
	
);


extern void mulval
( 
	void *dest,
	void *src,
	enum PrimType *vpt,
	struct SymNode **typ
	
);


extern void divval
( 
	void *dest,
	void *src,
	enum PrimType *vpt,
	struct SymNode **typ
	
);


extern void modval
( 
	void *dest,
	void *src,
	enum PrimType *vpt,
	struct SymNode **typ
	
);


extern void shlval
( 
	void *dest,
	void *src,
	enum PrimType *vpt,
	struct SymNode **typ
	
);


extern void shrval
( 
	void *dest,
	void *src,
	enum PrimType *vpt,
	struct SymNode **typ
	
);



extern void bigmaxUns( void *dest, void *left, void *right );
extern void bigmaxInt( void *dest, void *left, void *right );
extern void bigminUns( void *dest, void *left, void *right );
extern void bigminInt( void *dest, void *left, void *right );



extern int UnsLT( void *left, void *right );
extern int UnsLE( void *left, void *right );
extern int UnsGT( void *left, void *right );
extern int UnsGE( void *left, void *right );
extern int IntLT( void *left, void *right );
extern int IntLE( void *left, void *right );
extern int IntGT( void *left, void *right );
extern int IntGE( void *left, void *right );

extern int InRange
( 
	void *value, 
	enum PrimType tpt, 
	enum PrimType vpt, 
	enum PrimType *vpta,
	struct SymNode **vt
);

extern void DecStrToInt
( 
	char *numstr, 
	void *dest,
	struct SymNode **Type,
	enum PrimType *pType
);

extern void BinStrToInt
( 
	char *numstr, 
	void *dest,
	struct SymNode **Type,
	enum PrimType *pType
);

extern void HexStrToInt
( 
	char *numstr, 
	void *dest,
	struct SymNode **Type,
	enum PrimType *pType
);


extern int MakeCompAsm
( 
	enum PrimType *lpType, 
	enum PrimType *rpType, 
	struct SymNode **lType, 
	struct SymNode **rType, 
	void *lData, 
	void *rData
);



/*
** 80-bit floating point support
*/


extern void fadd80( struct flt80 *dest, struct flt80 l, struct flt80 r );
extern void fsub80( struct flt80 *dest, struct flt80 l, struct flt80 r );
extern void fmul80( struct flt80 *dest, struct flt80 l, struct flt80 r );
extern void fdiv80( struct flt80 *dest, struct flt80 l, struct flt80 r );

extern void fmax80( struct flt80 *dest, struct flt80 l, struct flt80 r );
extern void fmin80( struct flt80 *dest, struct flt80 l, struct flt80 r );

extern void fneg80( struct flt80 *dest, struct flt80 l );
extern void fabs80( struct flt80 *dest, struct flt80 l );

extern int  feq80 ( struct flt80 l, struct flt80 r );
extern int  fne80 ( struct flt80 l, struct flt80 r );
extern int  flt80 ( struct flt80 l, struct flt80 r );
extern int  fle80 ( struct flt80 l, struct flt80 r );
extern int  fgt80 ( struct flt80 l, struct flt80 r );
extern int  fge80 ( struct flt80 l, struct flt80 r );

extern void f80int( struct flt80 src, void *dest );
extern void unsf80( void *src, struct flt80 *dest );

extern void atold( struct flt80 *dest, char *a );
extern void e80Str( char *dest, struct flt80 value );
 
extern void ceil80( struct flt80 *dest, struct flt80 x );
extern void floor80( struct flt80 *dest, struct flt80 x );
extern void cos80( struct flt80 *dest, struct flt80 x );
extern void sin80( struct flt80 *dest, struct flt80 x );
extern void tan80( struct flt80 *dest, struct flt80 x );
extern void exp80( struct flt80 *dest, struct flt80 x );
extern void log80( struct flt80 *dest, struct flt80 x );
extern void log1080( struct flt80 *dest, struct flt80 x );
extern void sqrt80( struct flt80 *dest, struct flt80 x );




#endif
