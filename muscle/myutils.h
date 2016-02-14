#ifndef myutils_h
#define myutils_h

#include <stdio.h>
#include <sys/types.h>
#include <string>
#include <vector>
#include <math.h>
#include <stdarg.h>

using namespace std;

#ifdef _MSC_VER
#include <crtdbg.h>
#pragma warning(disable: 4996)	// deprecated functions
#define _CRT_SECURE_NO_DEPRECATE	1
#endif

#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG	1
#endif

#if defined(DEBUG) && !defined(_DEBUG)
#define _DEBUG	1
#endif

#ifndef NDEBUG
#define	DEBUG	1
#define	_DEBUG	1
#endif

typedef unsigned char byte;
typedef unsigned short uint16;
typedef unsigned uint32;
typedef int int32;
typedef double float32;
typedef signed char int8;
typedef unsigned char uint8;

#ifdef _MSC_VER

typedef __int64 int64;
typedef unsigned __int64 uint64;

#define INT64_PRINTF		"lld"
#define UINT64_PRINTF		"llu"

#define SIZE_T_PRINTF		"u"
#define OFF64_T_PRINTF		"lld"

#define INT64_PRINTFX		"llx"
#define UINT64_PRINTFX		"llx"

#define SIZE_T_PRINTFX		"x"
#define OFF64_T_PRINTFX		"llx"

#elif defined(__x86_64__)

typedef long int64;
typedef unsigned long uint64;

#define INT64_PRINTF		"ld"
#define UINT64_PRINTF		"lu"

#define SIZE_T_PRINTF		"lu"
#define OFF64_T_PRINTF		"ld"

#define INT64_PRINTFX		"lx"
#define UINT64_PRINTFX		"lx"

#define SIZE_T_PRINTFX		"lx"
#define OFF64_T_PRINTFX		"lx"

#else

typedef long long int64;
typedef unsigned long long uint64;

#define INT64_PRINTF		"lld"
#define UINT64_PRINTF		"llu"

#define SIZE_T_PRINTF		"u"
#define OFF64_T_PRINTF		"lld"

#define INT64_PRINTFX		"llx"
#define UINT64_PRINTFX		"llx"

#define SIZE_T_PRINTFX		"x"
#define OFF64_T_PRINTFX		"llx"
#endif

#define d64		INT64_PRINTF
#define	u64		UINT64_PRINTF
#define	x64		UINT64_PRINTFX

// const uint64 UINT64_MAX			= (~((uint64) 0));

void myassertfail(const char *Exp, const char *File, unsigned Line);
#undef  assert
#ifdef  NDEBUG
#define assert(exp)     ((void)0)
#define myassert(exp)     ((void)0)
#else
#define assert(exp) (void)( (exp) || (myassertfail(#exp, __FILE__, __LINE__), 0) )
#define myassert(exp) (void)( (exp) || (myassertfail(#exp, __FILE__, __LINE__), 0) )
#endif
#define asserta(exp) (void)( (exp) || (myassertfail(#exp, __FILE__, __LINE__), 0) )

#define ureturn(x)	return (x)

#define NotUsed(v)	((void *) &v)

// pom=plus or minus, tof=true or false
static inline char pom(bool Plus)	{ return Plus ? '+' : '-'; }
static inline char tof(bool x)		{ return x ? 'T' : 'F';	}
static inline char yon(bool x)		{ return x ? 'Y' : 'N';	}
unsigned GetElapsedSecs();

void *mymalloc(unsigned bytes);
void myfree(void *p);
template<class t> t *myalloc(unsigned n) { return (t *) mymalloc(n*sizeof(t)); }

#define SIZE(c)	unsigned((c).size())

bool myisatty(int fd);

FILE *OpenStdioFile(const string &FileName);
FILE *CreateStdioFile(const string &FileName);
bool CanSetStdioFilePos(FILE *f);
void CloseStdioFile(FILE *f);
void SetStdioFilePos(FILE *f, off_t Pos);
void ReadStdioFile(FILE *f, off_t Pos, void *Buffer, unsigned Bytes);
void ReadStdioFile(FILE *f, void *Buffer, unsigned Bytes);
void WriteStdioFile(FILE *f, off_t Pos, const void *Buffer, unsigned Bytes);
void WriteStdioFile(FILE *f, const void *Buffer, unsigned Bytes);
bool ReadLineStdioFile(FILE *f, char *Line, unsigned Bytes);
bool ReadLineStdioFile(FILE *f, string &Line);
byte *ReadAllStdioFile(FILE *f, unsigned &FileSize);
byte *ReadAllStdioFile(const string &FileName, unsigned &FileSize);
void AppendStdioFileToFile(FILE *fFrom, FILE *fTo);
void FlushStdioFile(FILE *f);
bool StdioFileExists(const string &FileName);
off_t GetStdioFilePos(FILE *f);
off_t GetStdioFileSize(FILE *f);
void LogStdioFileState(FILE *f);
void RenameStdioFile(const string &FileNameFrom, const string &FileNameTo);
void DeleteStdioFile(const string &FileName);

void myvstrprintf(string &Str, const char *szFormat, va_list ArgList);
void myvstrprintf(string &Str, const char *szFormat, ...);

void SetLogFileName(const string &FileName);
void Log(const char *szFormat, ...);

void Die(const char *szFormat, ...);
void Warning(const char *szFormat, ...);

void ProgressStep(unsigned i, unsigned N, const char *Format, ...);
void Progress(const char *szFormat, ...);
void ProgressLog(const char *szFormat, ...);
void ProgressExit();

// Are two floats equal to within epsilon?
const double epsilon = 0.01;
inline bool feq(double x, double y, double epsilon)
	{
	if (fabs(x) > 10000)
		epsilon = fabs(x)/10000;
	if (fabs(x - y) > epsilon)
		return false;
	return true;
	}

inline bool feq(double x, double y)
	{
	double e = epsilon;
	if (fabs(x) > 10000)
		e = fabs(x)/10000;
	if (fabs(x - y) > e)
		return false;
	return true;
	}

#define asserteq(x, y)	assert(feq(x, y))
#define assertaeq(x, y)	asserta(feq(x, y))

void Split(const string &Str, vector<string> &Fields, char Sep = 0);

void MyCmdLine(int argc, char **argv);

#define FLAG_OPT(LongName, ShortName, Help)							extern bool opt_##LongName;
#define TOG_OPT(LongName, ShortName, Default, Help)							extern bool opt_##LongName;
#define INT_OPT(LongName, ShortName, Default, Min, Max, Help)		extern int opt_##LongName;
#define UNS_OPT(LongName, ShortName, Default, Min, Max, Help)		extern unsigned opt_##LongName;
#define FLT_OPT(LongName, ShortName, Default, Min, Max, Help)		extern double opt_##LongName;
#define STR_OPT(LongName, ShortName, Default, Help)					extern string opt_##LongName;
#define ENUM_OPT(LongName, ShortName, Default, Values, Help)		extern int opt_##LongName;
#include "myopts.h"
#undef FLAG_OPT
#undef TOG_OPT
#undef INT_OPT
#undef UNS_OPT
#undef FLT_OPT
#undef STR_OPT
#undef ENUM_OPT

#endif	// myutils_h
