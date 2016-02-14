#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <signal.h>
#include <float.h>

#ifdef _MSC_VER
#include <process.h>
#include <windows.h>
#include <psapi.h>
#include <io.h>
#else
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#endif

#include "myutils.h"

#define	TEST_UTILS			0

using namespace std;

const unsigned MY_IO_BUFSIZ = 32000;
const unsigned MAX_FORMATTED_STRING_LENGTH = 64000;

static char *g_IOBuffers[256];
static time_t g_StartTime = time(0);
static vector<string> g_Argv;
static double g_PeakMemUseBytes;

#if	TEST_UTILS
void TestUtils()
	{
	const int C = 100000000;
	for (int i = 0; i < C; ++i)
		ProgressStep(i, C, "something or other");

	Progress("\n");
	Progress("Longer message\r");
	Sleep(1000);
	Progress("Short\r");
	Sleep(1000);
	Progress("And longer again\r");
	Sleep(1000);
	Progress("Shrt\n");
	Sleep(1000);
	const unsigned N = 10;
	unsigned M = 10;
	for (unsigned i = 0; i < N; ++i)
		{
		ProgressStep(i, N, "Allocating 1MB blocks");
		for (unsigned j = 0; j < M; ++j)
			{
			ProgressStep(j, M, "Inner loop"); 
			malloc(100000);
			Sleep(500);
			}
		}
	}
#endif // TEST_UTILS

static void AllocBuffer(FILE *f)
	{
	int fd = fileno(f);
	if (fd < 0 || fd >= 256)
		return;
	myfree(g_IOBuffers[fd]);
	g_IOBuffers[fd] = myalloc<char>(MY_IO_BUFSIZ);
	setvbuf(f, g_IOBuffers[fd], _IOFBF, MY_IO_BUFSIZ);
	}

static void FreeBuffer(FILE *f)
	{
	int fd = fileno(f);
	if (fd < 0 || fd >= 256)
		return;
	if (g_IOBuffers[fd] == 0)
		return;
	myfree(g_IOBuffers[fd]);
	g_IOBuffers[fd] = 0;
	}

unsigned GetElapsedSecs()
	{
	return (unsigned) (time(0) - g_StartTime);
	}

void *mymalloc(unsigned bytes)
	{
	void *p = malloc(bytes);
	if (0 == p)
		{
		fprintf(stderr, "Out of memory allocmem(%u)", (unsigned) bytes);
		Die("Out of memory, allocmem(%u)\n", (unsigned) bytes);
		}
	return p;
	}

void myfree(void *p)
	{
	if (p == 0)
		return;
	free(p);
	}

bool StdioFileExists(const string &FileName)
	{
	struct stat SD;
	int i = stat(FileName.c_str(), &SD);
	return i == 0;
	}

void myassertfail(const char *Exp, const char *File, unsigned Line)
	{
	Die("%s(%u) assert failed: %s", File, Line, Exp);
	}

bool myisatty(int fd)
	{
	return isatty(fd) != 0;
	}

#ifdef _MSC_VER
#include <io.h>
int fseeko(FILE *stream, off_t offset, int whence)
	{
	off_t FilePos = _fseeki64(stream, offset, whence);
	return (FilePos == -1L) ? -1 : 0;
	}
#define ftello(fm) _ftelli64(fm)
#endif

void LogStdioFileState(FILE *f)
	{
	long tellpos = ftell(f);
	long fseek_pos = fseek(f, 0, SEEK_CUR);
	int fd = fileno(f);
	Log("FILE *     %p\n", f);
	Log("fileno     %d\n", fd);
	Log("feof       %d\n", feof(f));
	Log("ferror     %d\n", ferror(f));
	Log("ftell      %ld\n", tellpos);
	Log("fseek      %ld\n", fseek_pos);
#if	!defined(_GNU_SOURCE) && !defined(__APPLE_CC__)
	fpos_t fpos;
	int fgetpos_retval = fgetpos(f, &fpos);
	Log("fpos       %ld (retval %d)\n", (long) fpos, fgetpos_retval);
//	Log("eof        %d\n", _eof(fd));
#endif
#ifdef _MSC_VER
	__int64 pos64 = _ftelli64(f);
	Log("_ftelli64  %lld\n", pos64);
#endif
	}

FILE *OpenStdioFile(const string &FileName)
	{
	const char *Mode = "rb";
	FILE *f = fopen(FileName.c_str(), Mode);
	if (0 == f)
		Die("Cannot open %s, errno=%d %s",
		  FileName.c_str(), errno, strerror(errno));
	AllocBuffer(f);
	return f;
	}

FILE *CreateStdioFile(const string &FileName)
	{
	FILE *f = fopen(FileName.c_str(), "wb+");
	if (0 == f)
		Die("Cannot create %s, errno=%d %s",
		  FileName.c_str(), errno, strerror(errno));
	AllocBuffer(f);
	return f;
	}

void SetStdioFilePos(FILE *f, off_t Pos)
	{
	if (0 == f)
		Die("SetStdioFilePos failed, f=NULL");
	int Ok = fseeko(f, Pos, SEEK_SET);
	off_t NewPos = ftell(f);
	if (Ok != 0 || Pos != NewPos)
		{
		LogStdioFileState(f);
		Die("SetStdioFilePos(%d) failed, Ok=%d NewPos=%d",
		  (int) Pos, Ok, (int) NewPos);
		}
	}

void ReadStdioFile(FILE *f, off_t Pos, void *Buffer, unsigned Bytes)
	{
	if (0 == f)
		Die("ReadStdioFile failed, f=NULL");
	SetStdioFilePos(f, Pos);
	unsigned BytesRead = fread(Buffer, 1, Bytes, f);
	if (BytesRead != Bytes)
		{
		LogStdioFileState(f);
		Die("ReadStdioFile failed, attempted %d bytes, read %d bytes, errno=%d",
		  (int) Bytes, (int) BytesRead, errno);
		}
	}

void ReadStdioFile(FILE *f, void *Buffer, unsigned Bytes)
	{
	if (0 == f)
		Die("ReadStdioFile failed, f=NULL");
	unsigned BytesRead = fread(Buffer, 1, Bytes, f);
	if (BytesRead != Bytes)
		{
		LogStdioFileState(f);
		Die("ReadStdioFile failed, attempted %d bytes, read %d bytes, errno=%d",
		  (int) Bytes, (int) BytesRead, errno);
		}
	}

// Return values from functions like lseek, ftell, fgetpos are
// "undefined" for files that cannot seek. Attempt to detect
// whether a file can seek by checking for error returns.
bool CanSetStdioFilePos(FILE *f)
	{
// Common special cases
	if (f == stdin || f == stdout || f == stderr)
		return false;

	fpos_t CurrPos;
	int ok1 = fgetpos(f, &CurrPos);
	if (ok1 < 0)
		return false;
	int ok2 = fseek(f, 0, SEEK_END);
	if (ok2 < 0)
		return false;
	fpos_t EndPos;
	int ok3 = fgetpos(f, &EndPos);
	int ok4 = fsetpos(f, &CurrPos);
	if (!ok3 || !ok4)
		return false;
	return true;
	}

byte *ReadAllStdioFile(FILE *f, unsigned &FileSize)
	{
	const unsigned BUFF_SIZE = 1024*1024;

	if (CanSetStdioFilePos(f))
		{
		off_t Pos = GetStdioFilePos(f);
		off_t FileSize = GetStdioFileSize(f);
		if (FileSize > UINT_MAX)
			Die("ReadAllStdioFile: file size > UINT_MAX");
		SetStdioFilePos(f, 0);
		byte *Buffer = myalloc<byte>(unsigned(FileSize));
		ReadStdioFile(f, Buffer, unsigned(FileSize));
		SetStdioFilePos(f, Pos);
		FileSize = unsigned(FileSize);
		return Buffer;
		}

// Can't seek, read one buffer at a time.
	FileSize = 0;

// Just to initialize so that first call to realloc works.
	byte *Buffer = (byte *) malloc(4);
	if (Buffer == 0)
		Die("ReadAllStdioFile, out of memory");
	for (;;)
		{
		Buffer = (byte *) realloc(Buffer, FileSize + BUFF_SIZE);
		unsigned BytesRead = fread(Buffer + FileSize, 1, BUFF_SIZE, f);
		FileSize += BytesRead;
		if (BytesRead < BUFF_SIZE)
			{
			Buffer = (byte *) realloc(Buffer, FileSize);
			return Buffer;
			}
		}
	}

byte *ReadAllStdioFile(const std::string &FileName, unsigned &FileSize)
	{
	FILE *f = OpenStdioFile(FileName.c_str());
	byte *Buffer = ReadAllStdioFile(f, FileSize);
	CloseStdioFile(f);
	return Buffer;
	}

void WriteStdioFile(FILE *f, off_t Pos, const void *Buffer, unsigned Bytes)
	{
	if (0 == f)
		Die("WriteStdioFile failed, f=NULL");
	SetStdioFilePos(f, Pos);
	unsigned BytesWritten = fwrite(Buffer, 1, Bytes, f);
	if (BytesWritten != Bytes)
		{
		LogStdioFileState(f);
		Die("WriteStdioFile failed, attempted %d bytes, wrote %d bytes, errno=%d",
		  (int) Bytes, (int) BytesWritten, errno);
		}
	}

void WriteStdioFile(FILE *f, const void *Buffer, unsigned Bytes)
	{
	if (0 == f)
		Die("WriteStdioFile failed, f=NULL");
	unsigned BytesWritten = fwrite(Buffer, 1, Bytes, f);
	if (BytesWritten != Bytes)
		{
		LogStdioFileState(f);
		Die("WriteStdioFile failed, attempted %d bytes, wrote %d bytes, errno=%d",
		  (int) Bytes, (int) BytesWritten, errno);
		}
	}

// Return false on EOF, true if line successfully read.
bool ReadLineStdioFile(FILE *f, char *Line, unsigned Bytes)
	{
	if (feof(f))
		return false;
	if ((int) Bytes < 0)
		Die("ReadLineStdioFile: Bytes < 0");
	char *RetVal = fgets(Line, (int) Bytes, f);
	if (NULL == RetVal)
		{
		if (feof(f))
			return false;
		if (ferror(f))
			Die("ReadLineStdioFile: errno=%d", errno);
		Die("ReadLineStdioFile: fgets=0, feof=0, ferror=0");
		}

	if (RetVal != Line)
		Die("ReadLineStdioFile: fgets != Buffer");
	unsigned n = strlen(Line);
	if (n < 1 || Line[n-1] != '\n')
		Die("ReadLineStdioFile: line too long or missing end-of-line");
	if (n > 0 && (Line[n-1] == '\r' || Line[n-1] == '\n'))
		Line[n-1] = 0;
	if (n > 1 && (Line[n-2] == '\r' || Line[n-2] == '\n'))
		Line[n-2] = 0;
	return true;
	}

// Return false on EOF, true if line successfully read.
bool ReadLineStdioFile(FILE *f, string &Line)
	{
	Line.clear();
	for (;;)
		{
		int c = fgetc(f);
		if (c == -1)
			{
			if (feof(f))
				{
				if (!Line.empty())
					return true;
				return false;
				}
			Die("ReadLineStdioFile, errno=%d", errno);
			}
		if (c == '\r')
			continue;
		if (c == '\n')
			return true;
		Line.push_back((char) c);
		}
	}

// Copies all of fFrom regardless of current
// file position, appends to fTo.
void AppendStdioFileToFile(FILE *fFrom, FILE *fTo)
	{
	off_t SavedFromPos = GetStdioFilePos(fFrom);
	off_t FileSize = GetStdioFileSize(fFrom);
	const off_t BUFF_SIZE = 1024*1024;
	char *Buffer = myalloc<char>(BUFF_SIZE);
	SetStdioFilePos(fFrom, 0);
	off_t BytesRemaining = FileSize;
	while (BytesRemaining > 0)
		{
		off_t BytesToRead = BytesRemaining;
		if (BytesToRead > BUFF_SIZE)
			BytesToRead = BUFF_SIZE;
		ReadStdioFile(fFrom, Buffer, (unsigned) BytesToRead);
		WriteStdioFile(fTo, Buffer, (unsigned) BytesToRead);
		BytesRemaining -= BytesToRead;
		}
	SetStdioFilePos(fFrom, SavedFromPos);
	}

void RenameStdioFile(const string &FileNameFrom, const string &FileNameTo)
	{
	int Ok = rename(FileNameFrom.c_str(), FileNameTo.c_str());
	if (Ok != 0)
		Die("RenameStdioFile(%s,%s) failed, errno=%d %s",
		  FileNameFrom.c_str(), FileNameTo.c_str(), errno, strerror(errno));
	}

void FlushStdioFile(FILE *f)
	{
	int Ok = fflush(f);
	if (Ok != 0)
		Die("fflush(%p)=%d,", f, Ok);
	}

void CloseStdioFile(FILE *f)
	{
	int Ok = fclose(f);
	if (Ok != 0)
		Die("fclose(%p)=%d", f, Ok);
	FreeBuffer(f);
	}

off_t GetStdioFilePos(FILE *f)
	{
	off_t FilePos = ftell(f);
	if (FilePos < 0)
		Die("ftello=%d", (int) FilePos);
	return FilePos;
	}

off_t GetStdioFileSize(FILE *f)
	{
	off_t CurrentPos = GetStdioFilePos(f);
	int Ok = fseeko(f, 0, SEEK_END);
	if (Ok < 0)
		Die("fseek in GetFileSize");
	off_t Length = ftell(f);
	if (Length < 0)
		Die("ftell in GetFileSize");
	SetStdioFilePos(f, CurrentPos);
	return Length;
	}

void DeleteStdioFile(const string &FileName)
	{
	int Ok = remove(FileName.c_str());
	if (Ok != 0)
		Die("remove(%s) failed, errno=%d %s", FileName.c_str(), errno, strerror(errno));
	}

void myvstrprintf(string &Str, const char *Format, va_list ArgList)
	{
	static char szStr[MAX_FORMATTED_STRING_LENGTH];
	vsnprintf(szStr, MAX_FORMATTED_STRING_LENGTH-1, Format, ArgList);
	szStr[MAX_FORMATTED_STRING_LENGTH - 1] = '\0';
	Str.assign(szStr);
	}

void myvstrprintf(string &Str, const char *Format, ...)
	{
	va_list ArgList;
	va_start(ArgList, Format);
	myvstrprintf(Str, Format, ArgList);
	va_end(ArgList);
	}

static FILE *g_fLog = 0;

void SetLogFileName(const string &FileName)
	{
	if (g_fLog != 0)
		CloseStdioFile(g_fLog);
	g_fLog = 0;
	if (FileName.empty())
		return;
	g_fLog = CreateStdioFile(FileName);
	}

void Log(const char *Format, ...)
	{
	if (g_fLog == 0)
		g_fLog = stdout;

	static bool InLog = false;
	if (InLog)
		return;

	InLog = true;
	va_list ArgList;
	va_start(ArgList, Format);
	vfprintf(g_fLog, Format, ArgList);
	va_end(ArgList);
	fflush(g_fLog);
	InLog = false;
	}

void Die(const char *Format, ...)
	{
	static bool InDie = false;
	if (InDie)
		exit(1);
	InDie = true;
	string Msg;

	if (g_fLog != 0)
		setbuf(g_fLog, 0);
	va_list ArgList;
	va_start(ArgList, Format);
	myvstrprintf(Msg, Format, ArgList);
	va_end(ArgList);

	const char *szStr = Msg.c_str();
	fprintf(stderr, "\n---Fatal error---\n%s\n", szStr);
	Log("\n---Fatal error---\n%s\n", szStr);

	Log("\n");
	time_t t = time(0);
	Log("%s", asctime(localtime(&t)));
	for (unsigned i = 0; i < g_Argv.size(); i++)
		{
		fprintf(stderr, (i == 0) ? "%s" : " %s", g_Argv[i].c_str());
		Log((i == 0) ? "%s" : " %s", g_Argv[i].c_str());
		}
	fprintf(stderr, "\n");
	Log("\n");

	time_t CurrentTime = time(0);
	unsigned ElapsedSeconds = unsigned(CurrentTime - g_StartTime);
	fprintf(stderr, "Elapsed time: %u seconds\n", ElapsedSeconds);
	Log("Elapsed time: %u seconds\n", ElapsedSeconds);

#ifdef _MSC_VER
	if (IsDebuggerPresent())
		__debugbreak();
	_CrtSetDbgFlag(0);
#endif

	exit(1);
	}

void Warning(const char *Format, ...)
	{
	string Msg;

	va_list ArgList;
	va_start(ArgList, Format);
	myvstrprintf(Msg, Format, ArgList);
	va_end(ArgList);

	const char *szStr = Msg.c_str();

	fprintf(stderr, "\nWARNING: %s\n", szStr);
	if (g_fLog != stdout)
		{
		Log("\nWARNING: %s\n", szStr);
		fflush(g_fLog);
		}
	}

#ifdef _MSC_VER
double GetMemUseBytes()
	{
	HANDLE hProc = GetCurrentProcess();
	PROCESS_MEMORY_COUNTERS PMC;
	BOOL bOk = GetProcessMemoryInfo(hProc, &PMC, sizeof(PMC));
	if (!bOk)
		return 1000000;
	double Bytes = (double) PMC.WorkingSetSize;
	if (Bytes > g_PeakMemUseBytes)
		g_PeakMemUseBytes = Bytes;
	return Bytes;
	}
#elif	linux || __linux__
double GetMemUseBytes()
	{
	static char statm[64];
	static int PageSize = 1;
	if (0 == statm[0])
		{
		PageSize = sysconf(_SC_PAGESIZE);
		pid_t pid = getpid();
		sprintf(statm, "/proc/%d/statm", (int) pid);
		}

	int fd = open(statm, O_RDONLY);
	if (-1 == fd)
		return 1000000;
	char Buffer[64];
	int n = read(fd, Buffer, sizeof(Buffer) - 1);
	close(fd);
	fd = -1;

	if (n <= 0)
		return 1000000;

	Buffer[n] = 0;
	double Pages = atof(Buffer);

	double Bytes = Pages*PageSize;
	if (Bytes > g_PeakMemUseBytes)
		g_PeakMemUseBytes = Bytes;
	return Bytes;
	}
#elif defined(__MACH__)
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <sys/gmon.h>
#include <mach/vm_param.h>
#include <netinet/in.h>
#include <netinet/icmp6.h>
#include <sys/vmmeter.h>
#include <sys/proc.h>
#include <mach/task_info.h>
#include <mach/task.h>
#include <mach/mach_init.h>
#include <mach/vm_statistics.h>

#define DEFAULT_MEM_USE	100000000.0

double GetMemUseBytes()
	{
	task_t mytask = mach_task_self();
	struct task_basic_info ti;
	memset((void *) &ti, 0, sizeof(ti));
	mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
	kern_return_t ok = task_info(mytask, TASK_BASIC_INFO, (task_info_t) &ti, &count);
	if (ok == KERN_INVALID_ARGUMENT)
		return DEFAULT_MEM_USE;

	if (ok != KERN_SUCCESS)
		return DEFAULT_MEM_USE;

	double Bytes = (double ) ti.resident_size;
	if (Bytes > g_PeakMemUseBytes)
		g_PeakMemUseBytes = Bytes;
	return Bytes;
	}
#else
double GetMemUseBytes()
	{
	return 0;
	}
#endif

const char *SecsToHHMMSS(int Secs)
	{
	int HH = Secs/3600;
	int MM = (Secs - HH*3600)/60;
	int SS = Secs%60;
	static char Str[16];
	if (HH == 0)
		sprintf(Str, "%02d:%02d", MM, SS);
	else
		sprintf(Str, "%02d:%02d:%02d", HH, MM, SS);
	return Str;
	}

const char *MemBytesToStr(double Bytes)
	{
	static char Str[32];

	if (Bytes < 1e6)
		sprintf(Str, "%.1fkb", Bytes/1e3);
	else if (Bytes < 10e6)
		sprintf(Str, "%.1fMb", Bytes/1e6);
	else if (Bytes < 1e9)
		sprintf(Str, "%.0fMb", Bytes/1e6);
	else if (Bytes < 10e9)
		sprintf(Str, "%.1fGb", Bytes/1e9);
	else if (Bytes < 100e9)
		sprintf(Str, "%fGb", Bytes/1e9);
	else
		sprintf(Str, "%gb", Bytes);
	return Str;
	}

bool opt_quiet = false;
bool opt_logopts = false;
bool opt_compilerinfo = false;
bool opt_help = false;
string opt_log = "";

static string g_CurrentProgressLine;
static vector<string> g_ProgressDesc;
static vector<unsigned> g_ProgressIndex;
static vector<unsigned> g_ProgressCount;

static unsigned g_CurrProgressLineLength;
static unsigned g_LastProgressLineLength;
static unsigned g_CountsInterval;
static time_t g_TimeLastOutputStep;

static string &GetProgressPrefixStr(string &s)
	{
	double Bytes = GetMemUseBytes();
	unsigned Secs = GetElapsedSecs();
	s = string(SecsToHHMMSS(Secs));
	if (Bytes > 0)
		{
		s.push_back(' ');
		char Str[32];
		sprintf(Str, "%5.5s", MemBytesToStr(Bytes));
		s += string(Str);
		}
	s.push_back(' ');
	return s;
	}

void ProgressLog(const char *Format, ...)
	{
	string Str;
	va_list ArgList;
	va_start(ArgList, Format);
	myvstrprintf(Str, Format, ArgList);
	va_end(ArgList);

	Log("%s", Str.c_str());
	Progress("%s", Str.c_str());
	}

void Progress(const char *Format, ...)
	{
	if (opt_quiet)
		return;

	string Str;
	va_list ArgList;
	va_start(ArgList, Format);
	myvstrprintf(Str, Format, ArgList);
	va_end(ArgList);

#if	0
	Log("Progress(");
	for (unsigned i = 0; i < Str.size(); ++i)
		{
		char c = Str[i];
		if (c == '\r')
			Log("\\r");
		else if (c == '\n')
			Log("\\n");
		else
			Log("%c", c);
		}
	Log(")\n");
#endif //0

	for (unsigned i = 0; i < Str.size(); ++i)
		{
		if (g_CurrProgressLineLength == 0)
			{
			string s;
			GetProgressPrefixStr(s);
			for (unsigned j = 0; j < s.size(); ++j)
				{
				fputc(s[j], stderr);
				++g_CurrProgressLineLength;
				}
			}

		char c = Str[i];
		if (c == '\n' || c == '\r')
			{
			for (unsigned j = g_CurrProgressLineLength; j < g_LastProgressLineLength; ++j)
				fputc(' ', stderr);
			if (c == '\n')
				g_LastProgressLineLength = 0;
			else
				g_LastProgressLineLength = g_CurrProgressLineLength;
			g_CurrProgressLineLength = 0;
			fputc(c, stderr);
			}
		else
			{
			fputc(c, stderr);
			++g_CurrProgressLineLength;
			}
		}
	}

void ProgressExit()
	{
	time_t Now = time(0);
	struct tm *t = localtime(&Now);
	const char *s = asctime(t);
	unsigned Secs = GetElapsedSecs();

	Log("\n");
	Log("Finished %s", s); // there is a newline in s
	Log("Elapsed time %s\n", SecsToHHMMSS((int) Secs));
	Log("Max memory %s\n", MemBytesToStr(g_PeakMemUseBytes));
	}

const char *PctStr(double x, double y)
	{
	if (y == 0)
		{
		if (x == 0)
			return "100%";
		else
			return "inf%";
		}
	static char Str[16];
	double p = x*100.0/y;
	sprintf(Str, "%5.1f%%", p);
	return Str;
	}

string &GetProgressLevelStr(unsigned Level, string &s)
	{
	unsigned Index = g_ProgressIndex[Level];
	unsigned Count = g_ProgressCount[Level];
	if (Count == UINT_MAX)
		{
		if (Index == UINT_MAX)
			s = "100%";
		else
			{
			char Tmp[16];
			sprintf(Tmp, "%u", Index); 
			s = Tmp;
			}
		}
	else
		s = string(PctStr(Index+1, Count));
	s += string(" ") + g_ProgressDesc[Level];
	return s;
	}

void ProgressStep(unsigned i, unsigned N, const char *Format, ...)
	{
	if (opt_quiet)
		return;

	if (i == 0)
		{
		string Str;
		va_list ArgList;
		va_start(ArgList, Format);
		myvstrprintf(Str, Format, ArgList);
		va_end(ArgList);
		g_ProgressDesc.push_back(Str);
		g_ProgressIndex.push_back(0);
		g_ProgressCount.push_back(N);
		g_CountsInterval = 1;
		g_TimeLastOutputStep = 0;
		}

	unsigned Depth = g_ProgressDesc.size();
	if (i >= N && i != UINT_MAX)
		Die("ProgressStep(%u,%u)", i, N);
	bool LastStep = (i == UINT_MAX || i + 1 == N);
	if (!LastStep)
		{
		if (i%g_CountsInterval != 0)
			return;

		time_t Now = time(0);
		if (Now == g_TimeLastOutputStep)
			{
			g_CountsInterval *= 2;
			return;
			}
		else if (Now - g_TimeLastOutputStep > 1)
			g_CountsInterval /= 2;

		if (g_CountsInterval < 1)
			g_CountsInterval = 1;

		g_TimeLastOutputStep = Now;
		}

	g_ProgressIndex[Depth-1] = i;

	if (i > 0)
		{
		string Str;
		va_list ArgList;
		va_start(ArgList, Format);
		myvstrprintf(Str, Format, ArgList);
		va_end(ArgList);
		g_ProgressDesc[Depth-1] = Str;
		}

	string s;
	for (unsigned Level = 0; Level < Depth; ++Level)
		{
		string LevelStr;
		GetProgressLevelStr(Level, LevelStr);
		s += string(" ") + LevelStr;
		}
	Progress("%s\r", s.c_str());

	if (LastStep)
		{
		g_ProgressDesc.pop_back();
		g_ProgressIndex.pop_back();
		g_ProgressCount.pop_back();
		g_CountsInterval = 1;
		fputc('\n', stderr);
		}
	}

enum OptType
	{
	OT_Flag,
	OT_Tog,
	OT_Int,
	OT_Uns,
	OT_Str,
	OT_Float,
	OT_Enum
	};

struct OptInfo
	{
	void *Value;
	string LongName;
	string ShortName;
	OptType Type;
	int iMin;
	int iMax;
	unsigned uMin;
	unsigned uMax;
	double dMin;
	double dMax;
	map<string, unsigned> EnumValues;

	bool bDefault;
	int iDefault;
	unsigned uDefault;
	double dDefault;
	string strDefault;

	string Help;

	bool operator<(const OptInfo &rhs) const
		{
		return LongName < rhs.LongName;
		}
	};

static set<OptInfo> g_Opts;

static void Help()
	{
	printf("\n");
	unsigned MaxL = 0;
	for (set<OptInfo>::const_iterator p = g_Opts.begin(); p != g_Opts.end(); ++p)
		{
		const OptInfo &Opt = *p;
		unsigned L = Opt.LongName.size();
		if (Opt.Type == OT_Tog)
			L += 4; // "[no]"
		if (L > MaxL)
			MaxL = L;
		}

	unsigned TabStop = MaxL + 10;
	for (set<OptInfo>::const_iterator p = g_Opts.begin(); p != g_Opts.end(); ++p)
		{
		const OptInfo &Opt = *p;

		printf("\n");
		if (Opt.ShortName == "-")
			printf("    ");
		else
			printf("  -%c", Opt.ShortName[0]);
		string LongName = Opt.LongName.c_str();
		if (Opt.Type == OT_Tog)
			LongName = string("[no]") + LongName;
		printf("  --%-*.*s  ", MaxL, MaxL, LongName.c_str());
		const string &s = Opt.Help;
		for (string::const_iterator q = s.begin(); q != s.end(); ++q)
			{
			char c = *q;
			if (c == '\n')
				printf("\n%*.*s", TabStop, TabStop, "");
			else
				printf("%c", c);
			}
		printf("\n");
		}
	printf("\n");
	exit(0);
	}

static void CmdLineErr(const char *Format, ...)
	{
	va_list ArgList;
	va_start(ArgList, Format);
	string Str;
	myvstrprintf(Str, Format, ArgList);
	va_end(ArgList);
	fprintf(stderr, "\n");
	fprintf(stderr, "Invalid command line\n");
	fprintf(stderr, "%s\n", Str.c_str());
	fprintf(stderr, "For list of command-line options use --help.\n");
	fprintf(stderr, "\n");
	exit(1);
	}

static set<OptInfo>::iterator GetOptInfo(const string &LongName,
  bool ErrIfNotFound)
	{
	for (set<OptInfo>::iterator p = g_Opts.begin();
	  p != g_Opts.end(); ++p)
		{
		const OptInfo &Opt = *p;
		if (Opt.LongName == LongName)
			return p;
		if (Opt.Type == OT_Tog && "no" + Opt.LongName == LongName)
			return p;
		}
	if (ErrIfNotFound)
		CmdLineErr("Option --%s is invalid", LongName.c_str());
	return g_Opts.end();
	}

static set<OptInfo>::iterator GetOptInfo(char ShortName, bool ErrIfNotFound)
	{
	for (set<OptInfo>::iterator p = g_Opts.begin();
	  p != g_Opts.end(); ++p)
		{
		const OptInfo *Opt = &(*p);
		if (Opt->ShortName[0] == ShortName)
			return p;
		}
	if (ErrIfNotFound)
		CmdLineErr("Option -%c is invalid", ShortName);
	return g_Opts.end();
	}

static void AddOpt(const OptInfo &Opt)
	{
	if (Opt.ShortName.size() != 1)
		Die("Option --%s has invalid short name (must be one character)",
		  Opt.LongName.c_str());
	if (GetOptInfo(Opt.LongName, false) != g_Opts.end())
		Die("Option --%s defined twice", Opt.LongName.c_str());
	if (GetOptInfo(Opt.ShortName, false) != g_Opts.end())
		Die("Option -%c defined twice", Opt.ShortName.c_str());
	g_Opts.insert(Opt);
	}

#ifdef _MSC_VER
#pragma warning(disable: 4505) // unreferenced local function
#endif

static void DefineFlagOpt(const string &LongName, const string &ShortName,
  const string &Help, void *Value)
	{
	*(bool *) Value = false;

	OptInfo Opt;
	Opt.Value = Value;
	Opt.LongName = LongName;
	Opt.ShortName = ShortName;
	Opt.bDefault = false;
	Opt.Help = Help;
	Opt.Type = OT_Flag;
	AddOpt(Opt);
	}

static void DefineTogOpt(const string &LongName, const string &ShortName,
  bool Default, const string &Help, void *Value)
	{
	*(bool *) Value = Default;

	OptInfo Opt;
	Opt.Value = Value;
	Opt.LongName = LongName;
	Opt.ShortName = ShortName;
	Opt.bDefault = Default;
	Opt.Help = Help;
	Opt.Type = OT_Tog;
	AddOpt(Opt);
	}

static void DefineIntOpt(const string &LongName, const string &ShortName,
  int Default, int Min, int Max, const string &Help, void *Value)
	{
	*(int *) Value = Default;

	OptInfo Opt;
	Opt.Value = Value;
	Opt.LongName = LongName;
	Opt.ShortName = ShortName;
	Opt.iDefault = Default;
	Opt.iMin = Min;
	Opt.iMax = Max;
	Opt.Help = Help;
	Opt.Type = OT_Int;
	AddOpt(Opt);
	}

static void DefineUnsOpt(const string &LongName, const string &ShortName,
  unsigned Default, unsigned Min, unsigned Max, const string &Help, void *Value)
	{
	*(unsigned *) Value = Default;

	OptInfo Opt;
	Opt.Value = Value;
	Opt.LongName = LongName;
	Opt.ShortName = ShortName;
	Opt.uDefault = Default;
	Opt.uMin = Min;
	Opt.uMax = Max;
	Opt.Help = Help;
	Opt.Type = OT_Uns;
	AddOpt(Opt);
	}

static void DefineFloatOpt(const string &LongName, const string &ShortName,
  double Default, double Min, double Max, const string &Help, void *Value)
	{
	*(double *) Value = Default;

	OptInfo Opt;
	Opt.Value = Value;
	Opt.LongName = LongName;
	Opt.ShortName = ShortName;
	Opt.dDefault = Default;
	Opt.dMin = Min;
	Opt.dMax = Max;
	Opt.Help = Help;
	Opt.Type = OT_Float;
	AddOpt(Opt);
	}

static void DefineStrOpt(const string &LongName, const string &ShortName,
  const char *Default, const string &Help, void *Value)
	{
	*(string *) Value = (Default == 0 ? "" : string(Default));

	OptInfo Opt;
	Opt.Value = Value;
	Opt.LongName = LongName;
	Opt.ShortName = ShortName;
	Opt.strDefault = (Default == 0 ? "" : string(Default));
	Opt.Help = Help;
	Opt.Type = OT_Str;
	AddOpt(Opt);
	}

static void ParseEnumValues(const string &Values, map<string, unsigned> &EnumValues)
	{
	EnumValues.clear();
	
	string Name;
	string Value;
	bool Eq = false;
	for (string::const_iterator p = Values.begin(); ; ++p)
		{
		char c = (p == Values.end() ? '|' : *p);
		if (isspace(c))
			;
		else if (c == '|')
			{
			if (EnumValues.find(Name) != EnumValues.end())
				Die("Invalid enum values, '%s' defined twice: '%s'",
				  Name.c_str(), Values.c_str());
			if (Name.empty() || Value.empty())
				Die("Invalid enum values, empty name or value: '%s'",
				  Values.c_str());

			EnumValues[Name] = atoi(Value.c_str());
			Name.clear();
			Value.clear();
			Eq = false;
			}
		else if (c == '=')
			Eq = true;
		else if (Eq)
			Value.push_back(c);
		else
			Name.push_back(c);
		if (p == Values.end())
			return;
		}
	}

static void DefineEnumOpt(const string &LongName, const string &ShortName,
  int Default, const string &Values, const string &Help, void *Value)
	{
	*(int *) Value = Default;

	OptInfo Opt;
	Opt.Value = Value;
	Opt.LongName = LongName;
	Opt.ShortName = ShortName;
	Opt.iDefault = Default;
	Opt.Help = Help;
	Opt.Type = OT_Enum;
	ParseEnumValues(Values, Opt.EnumValues);
	AddOpt(Opt);
	}
#undef FLAG_OPT
#undef TOG_OPT
#undef INT_OPT
#undef UNS_OPT
#undef FLT_OPT
#undef STR_OPT
#undef ENUM_OPT
#define FLAG_OPT(LongName, ShortName, Help)							bool opt_##LongName;
#define TOG_OPT(LongName, ShortName, Default, Help)				bool opt_##LongName;
#define INT_OPT(LongName, ShortName, Default, Min, Max, Help)		int opt_##LongName;
#define UNS_OPT(LongName, ShortName, Default, Min, Max, Help)		unsigned opt_##LongName;
#define FLT_OPT(LongName, ShortName, Default, Min, Max, Help)		double opt_##LongName;
#define STR_OPT(LongName, ShortName, Default, Help)					string opt_##LongName;
#define ENUM_OPT(LongName, ShortName, Values, Default, Help)		int opt_##LongName;
#include "myopts.h"

static int EnumStrToInt(const OptInfo &Opt, const string &Value)
	{
	const map<string, unsigned> &e = Opt.EnumValues;
	string s;
	for (map<string, unsigned>::const_iterator p = e.begin(); p != e.end(); ++p)
		{
		if (Value == p->first)
			return p->second;
		s += " " + p->first;
		}
	CmdLineErr("--%s %s not recognized, valid are: %s",
	  Opt.LongName.c_str(), Value.c_str(), s.c_str());
	ureturn(-1);
	}

static void SetOpt(OptInfo &Opt, const string &Value)
	{
	switch (Opt.Type)
		{
	case OT_Int:
		{
		*(int *) Opt.Value = atoi(Value.c_str());
		break;
		}
	case OT_Uns:
		{
		int iValue = atoi(Value.c_str());
		if (iValue < 0)
			CmdLineErr("Option --%s cannot be negative", Opt.LongName.c_str());
		*(unsigned *) Opt.Value = unsigned(iValue);
		break;
		}
	case OT_Float:
		{
		*(double *) Opt.Value = atof(Value.c_str());
		break;
		}
	case OT_Str:
		{
		*(string *) Opt.Value = Value;
		break;
		}
	case OT_Enum:
		{
		*(int *) Opt.Value = EnumStrToInt(Opt, Value);
		break;
		}
	default:
		asserta(false);
		}
	}

void LogOpts()
	{
	for (set<OptInfo>::const_iterator p = g_Opts.begin(); p != g_Opts.end(); ++p)
		{
		const OptInfo &Opt = *p;
		Log("%s = ", Opt.LongName.c_str());
		switch (Opt.Type)
			{
		case OT_Flag:
			Log("%s", (*(bool *) Opt.Value) ? "yes" : "no");
			break;
		case OT_Tog:
			Log("%s", (*(bool *) Opt.Value) ? "on" : "off");
			break;
		case OT_Int:
			Log("%d", *(int *) Opt.Value);
			break;
		case OT_Uns:
			Log("%u", *(unsigned *) Opt.Value);
			break;
		case OT_Float:
			{
			double Value = *(double *) Opt.Value;
			if (Value == LOG_ZERO || Value == FLT_MAX)
				Log("*");
			else
				Log("%g", Value);
			break;
			}
		case OT_Str:
			Log("%s", (*(string *) Opt.Value).c_str());
			break;
		case OT_Enum:
			Log("%d", *(int *) Opt.Value);
			break;
		default:
			asserta(false);
			}
		Log("\n");
		}
	}

static void CompilerInfo()
	{
#define x(t)	printf("sizeof(" #t ") = %d\n", (int) sizeof(t));
	x(int)
	x(long)
	x(float)
	x(double)
	x(void *)
#undef x
	exit(0);
	}

void Split(const string &Str, vector<string> &Fields, char Sep)
	{
	Fields.clear();
	const unsigned Length = (unsigned) Str.size();
	string s;
	for (unsigned i = 0; i < Length; ++i)
		{
		char c = Str[i];
		if ((Sep == 0 && isspace(c)) || c == Sep)
			{
			if (!s.empty() || Sep != 0)
				Fields.push_back(s);
			s.clear();
			}
		else
			s.push_back(c);
		}
	if (!s.empty())
		Fields.push_back(s);
	}

static void GetArgsFromFile(const string &FileName, vector<string> &Args)
	{
	Args.clear();

	FILE *f = OpenStdioFile(FileName);
	string Line;
	while (ReadLineStdioFile(f, Line))
		{
		size_t n = Line.find('#');
		if (n != string::npos)
			Line = Line.substr(0, n);
		vector<string> Fields;
		Split(Line, Fields);
		Args.insert(Args.end(), Fields.begin(), Fields.end());
		}
	CloseStdioFile(f);
	}

void MyCmdLine(int argc, char **argv)
	{
	static unsigned RecurseDepth = 0;
	++RecurseDepth;

	DefineFlagOpt("compilerinfo", "-", "Write info about compiler types and #defines to stdout.",
	  (void *) &opt_compilerinfo);
	DefineFlagOpt("quiet", "q", "Turn off progress messages.", (void *) &opt_quiet);
	DefineFlagOpt("logopts", "-", "Log options.", (void *) &opt_logopts);
	DefineFlagOpt("help", "-", "Display command-line options.", (void *) &opt_help);
	DefineStrOpt("log", "-", "", "Log file name.", (void *) &opt_log);

#undef FLAG_OPT
#undef TOG_OPT
#undef INT_OPT
#undef UNS_OPT
#undef FLT_OPT
#undef STR_OPT
#undef ENUM_OPT
#define FLAG_OPT(LongName, ShortName, Help)							DefineFlagOpt(#LongName, #ShortName, Help, (void *) &opt_##LongName);
#define TOG_OPT(LongName, ShortName, Default, Help)					DefineTogOpt(#LongName, #ShortName, Default, Help, (void *) &opt_##LongName);
#define INT_OPT(LongName, ShortName, Default, Min, Max, Help)		DefineIntOpt(#LongName, #ShortName,  Default, Min, Max, Help, (void *) &opt_##LongName);
#define UNS_OPT(LongName, ShortName, Default, Min, Max, Help)		DefineUnsOpt(#LongName, #ShortName, Default, Min, Max, Help, (void *) &opt_##LongName);
#define FLT_OPT(LongName, ShortName, Default, Min, Max, Help)		DefineFloatOpt(#LongName, #ShortName, Default, Min, Max, Help, (void *) &opt_##LongName);
#define STR_OPT(LongName, ShortName, Default, Help)					DefineStrOpt(#LongName, #ShortName, Default, Help, (void *) &opt_##LongName);
#define ENUM_OPT(LongName, ShortName, Values, Default, Help)		DefineEnumOpt(#LongName, #ShortName, Values, Default, Help, (void *) &opt_##LongName);
#include "myopts.h"

	if (RecurseDepth == 0)
		g_Argv.clear();

	for (int i = 0; i < argc; ++i)
		g_Argv.push_back(string(argv[i]));

	int i = 1;
	for (;;)
		{
		if (i >= argc)
			break;
		const string &Arg = g_Argv[i];
		if (Arg.empty())
			continue;
		else if (Arg == "file:" && i + 1 < argc)
			{
			const string &FileName = g_Argv[i+1];
			vector<string> Args;
			GetArgsFromFile(FileName, Args);
			for (vector<string>::const_iterator p = Args.begin();
			  p != Args.end(); ++p)
				{
				g_Argv.push_back(*p);
				++argc;
				}
			i += 2;
			continue;
			}
		else if (Arg.size() > 2 && Arg[0] == '-' && Arg[1] == '-')
			{
			string LongName = Arg.substr(2);
			OptInfo Opt = *GetOptInfo(LongName, true);
			if (Opt.Type == OT_Flag)
				{
				g_Opts.erase(Opt);
				*(bool *) Opt.Value = true;
				g_Opts.insert(Opt);
				++i;
				continue;
				}
			else if (Opt.Type == OT_Tog)
				{
				g_Opts.erase(Opt);
				if (string("no") + Opt.LongName == LongName)
					*(bool *) Opt.Value = false;
				else
					{
					asserta(Opt.LongName == LongName);
					*(bool *) Opt.Value = true;
					}
				g_Opts.insert(Opt);
				++i;
				continue;
				}


			++i;
			if (i >= argc)
				CmdLineErr("Missing value for --%s", LongName.c_str());

			string Value = g_Argv[i];
			SetOpt(Opt, Value);

			++i;
			continue;
			}
		else if (Arg.size() > 1 && Arg[0] == '-')
			{
			unsigned L = Arg.size();
			for (unsigned j = 1; j < L; ++j)
				{
				char ShortName = Arg[j];
				OptInfo Opt = *GetOptInfo(ShortName, true);
				if (Opt.Type == OT_Flag)
					{
					g_Opts.erase(Opt);
					*(bool *) Opt.Value = true;
					g_Opts.insert(Opt);
					continue;
					}
				else
					{
					string Value;
					if (j + 1 == L)
						{
						if (i + 1 == argc)
							CmdLineErr("Missing value for -%c", ShortName);
						++i;
						Value = g_Argv[i];
						++i;
						}
					else
						{
						Value = Arg.substr(j);
						++i;
						}
					SetOpt(Opt, Value);
					goto NextArg;
					}
				}
			}
		else
			CmdLineErr("Expected - or --, got '%s'", Arg.c_str());
	NextArg:;
		}

	--RecurseDepth;
	if (RecurseDepth > 0)
		return;

	if (opt_help)
		Help();

	if (opt_compilerinfo)
		CompilerInfo();

	SetLogFileName(opt_log);

	if (opt_log != "")
		{
		for (int i = 0; i < argc; ++i)
			Log("%s%s", i == 0 ? "" : " ", g_Argv[i].c_str());
		Log("\n");
		time_t Now = time(0);
		struct tm *t = localtime(&Now);
		const char *s = asctime(t);
		Log("Started %s", s); // there is a newline in s
		Log("\n");
		}

	if (opt_logopts)
		LogOpts();
	}
