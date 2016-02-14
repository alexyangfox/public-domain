#include "myutils.h"
#include "mx.h"
#include "sumlog.h"

typedef void (*ON_PATH)(const string &Path);

static const byte *g_A;
static const byte *g_B;
static unsigned g_LA;
static unsigned g_LB;

static string g_DebugMx;
static int g_DebugState;
static unsigned g_Debugi;
static unsigned g_Debugj;

static bool g_LocalTrace = true;

static Mx<float> g_FwdMf;
static Mx<float> g_FwdDf;
static Mx<float> g_FwdIf;

static Mx<float> g_BwdMf;
static Mx<float> g_BwdDf;
static Mx<float> g_BwdIf;

static Mx<float> g_FwdBwdMf;
static Mx<float> g_FwdBwdDf;
static Mx<float> g_FwdBwdIf;

static Mx<float> g_FwdBwdMf2;
static Mx<float> g_FwdBwdDf2;
static Mx<float> g_FwdBwdIf2;

static Mx<float> g_PPf;

static float **g_FwdM;
static float **g_FwdD;
static float **g_FwdI;

static float **g_BwdM;
static float **g_BwdD;
static float **g_BwdI;

static float **g_FwdBwdM;
static float **g_FwdBwdD;
static float **g_FwdBwdI;

static float **g_FwdBwdM2;
static float **g_FwdBwdD2;
static float **g_FwdBwdI2;

static float **g_PP;

static float g_SumPaths;

static bool DoTrace(const char *Mx, const char *State, unsigned i, unsigned j)
	{
	if (g_LocalTrace)
		return true;
	return g_DebugMx == Mx && g_DebugState == *State && g_Debugi == i && g_Debugj == j;
	}

static void GetLetterCounts(const string &Path, unsigned &i, unsigned &j)
	{
	i = 0;
	j = 0;
	const unsigned L = SIZE(Path);
	for (unsigned k = 0; k < L; ++k)
		{
		int c = Path[k];
		if (c == 'M' || c == 'D')
			++i;
		if (c == 'M' || c == 'I')
			++j;
		}
	}

static void GetPBS(const string &Path, string &Prefix, string &Body,
  string &Suffix)
	{
	unsigned FirstM = UINT_MAX;
	unsigned LastM = UINT_MAX;

	const unsigned L = SIZE(Path);
	for (unsigned k = 0; k < L; ++k)
		{
		char c = Path[k];
		if (c == 'M')
			{
			if (FirstM == UINT_MAX)
				FirstM = k;
			LastM = k;
			}
		}

	if (FirstM == UINT_MAX)
		{
		Prefix = Path;
		Body.clear();
		Suffix.clear();
		return;
		}

	Prefix = Path.substr(0, FirstM);
	Body = Path.substr(FirstM, LastM - FirstM + 1);
	Suffix = Path.substr(LastM + 1);
	}

static bool ValidPath(const string &Path, bool Full)
	{
	if (Full)
		{
		unsigned i;
		unsigned j;
		GetLetterCounts(Path, i, j);
		if (i != g_LA || j != g_LB)
			return false;
		}
	
	string Prefix;
	string Body;
	string Suffix;
	GetPBS(Path, Prefix, Body, Suffix);

	if (Body.find("DI") != string::npos || Body.find("ID") != string::npos)
		return false;
	if (Prefix.find("ID") != string::npos || Suffix.find("ID") != string::npos)
		return false;
	return true;
	}

static void EnumPathsSWRecurse(const string &Path, unsigned LA, unsigned LB,
  ON_PATH OnPath)
	{
	unsigned i;
	unsigned j;
	GetLetterCounts(Path, i, j);

	bool FullPath = (i == LA && j == LB);

	if (!Path.empty())
			OnPath(Path);

	if (FullPath)
		return;

	if (i < LA && j < LB)
		EnumPathsSWRecurse(Path + 'M', LA, LB, OnPath);

	if (i < LA)
		EnumPathsSWRecurse(Path + 'D', LA, LB, OnPath);

	if (j < LB)
		EnumPathsSWRecurse(Path + 'I', LA, LB, OnPath);
	}

void EnumPathsSW(unsigned LA, unsigned LB, ON_PATH OnPath)
	{
	string Path;
	EnumPathsSWRecurse(Path, LA, LB, OnPath);
	}

static unsigned FirstM(const string &Path)
	{
	for (unsigned k = 0; k < SIZE(Path); ++k)
		if (Path[k] == 'M')
			return k;
	return UINT_MAX;
	}

static unsigned LastM(const string &Path)
	{
	for (int k = (int) SIZE(Path); k >= 0; --k)
		if (Path[k] == 'M')
			return (unsigned) k;
	return 0;
	}

static float ScorePath(const byte *A, const byte *B, bool Start, bool End,
  bool Bwd, const string &Path)
	{
	const float g = -5.7f;
	const float e = -0.25f;
	extern float **g_VTML200;
	float **Mx = g_VTML200;

	if (g_LocalTrace)
		{
		Log("\n");
		Log("ScorePath(%s%s%s)\n", Start ? "S" : "", Path.c_str(), End ? "E" : "");
		}

	unsigned FM = FirstM(Path);
	unsigned LM = LastM(Path);

	const unsigned L = SIZE(Path);
	unsigned i = 0;
	unsigned j = 0;
	float Score = 0.0f;
	int LastState = 'S';
	if (g_LocalTrace)
		{
		Log("  i    j  xx      What    Score         Total\n");
		Log("---  ---  --  --------  -------  ------------\n");
		}
	for (unsigned k = 0; k < L; ++k)
		{
		int State = Path[k];

		if (g_LocalTrace)
			Log("%3u  %3u  %c%c", i, j, LastState, State);

		if (k == 0 && Bwd)
			{
			if (State == 'M' || State == 'D')
				++i;
			if (State == 'M' || State == 'I')
				++j;
			if (g_LocalTrace)
				Log("\n");
			LastState = State;
			continue;
			}

		switch (State)
			{
		case 'M':
			{
			byte a = A[i];
			byte b = B[j];
			Score += Mx[a][b];
			if (g_LocalTrace)
				Log("   sub  %c%c  %7.4f", a, b, Mx[a][b]);
			++i;
			++j;
			break;
			}
		case 'D':
		case 'I':
			{
			if ((Start && k < FM) || (End && k > LM))
				{
				if (g_LocalTrace)
					Log("    ix      %7.7s", ".");
				}
			else if (LastState == 'M')
				{
				if (g_LocalTrace)
					Log("  open      %7.7s", ".");
				Score += g;
				}
			else
				{
				if (g_LocalTrace)
					Log("   ext      %7.7s", ".");
				Score += e;
				}
			State == 'D' ? ++i : ++j;
			break;
			}
		default:
			asserta(false);
			}
		if (g_LocalTrace)
			Log("  %12.12s\n", TypeToStr<float>(Score));
		LastState = State;
		}

	if (Start && End)
		asserta(i == g_LA && j == g_LB);

	if (g_LocalTrace)
		Log("ScorePath(%s%s%s)=%g\n", Start ? "S" : "", Path.c_str(), End ? "E" : "", Score);
	return Score;
	}

static void OnPathFwdBwd(const string &Path)
	{
	if (!ValidPath(Path, true))
		return;

	if (g_LocalTrace)
		{
		Log("\n");
		Log("OnPathFwdBwd(%s)\n", Path.c_str());
		}
	float Score = ScorePath(g_A, g_B, true, true, false, Path);
	g_SumPaths = SumLog2Exact(g_SumPaths, Score);
	if (g_LocalTrace)
		{
		string s1;
		string s2;
		Log("SumPaths += %12.12s = %12.12s  %s\n",
		  FloatToStr(Score, s1),
		  FloatToStr(g_SumPaths, s2),
		  Path.c_str());
		}

	unsigned i = 0;
	unsigned j = 0;
	const unsigned L = SIZE(Path);
	for (unsigned k = 0; k < L; ++k)
		{
		int State = Path[k];

#define add(x)																			\
		g_FwdBwd##x[i][j] = SumLog2Exact(g_FwdBwd##x[i][j], Score);						\
		if (DoTrace("FwdBwd", #x, i, j))													\
			{																			\
			bool Save = g_LocalTrace;													\
			g_LocalTrace = true;														\
			ScorePath(g_A, g_B, true, true, false, Path);								\
			g_LocalTrace = Save;														\
			string s1;																	\
			string s2;																	\
			Log("g_FwdBwd%s[%u][%u] +=  %12.12s = %12.12s\n",							\
			  #x, i, j, FloatToStr(Score, s1), FloatToStr(g_FwdBwd##x[i][j], s2));		\
			}

		switch (State)
			{
		case 'M':
			{
			++i;
			++j;
			add(M)
			break;
			}
		case 'D':
			{
			++i;
			add(D)
			break;
			}
		case 'I':
			{
			++j;
			add(I)
			break;
			}
		case 'd':
			++i;
			break;
		case 'i':
			++j;
			break;
			}
		}
#undef add
	}

static void OnPathFwd(const string &Path)
	{
	if (!ValidPath(Path, false))
		return;

	if (Path.empty())
		return;

	if (g_LocalTrace)
		{
		Log("\n");
		Log("OnPathFwd(%s)\n", Path.c_str());
		}

	unsigned i;
	unsigned j;
	GetLetterCounts(Path, i, j);
	char LastState = *Path.rbegin();
	float Score = ScorePath(g_A, g_B, true, false, false, Path);

#define add(x)																		\
	g_Fwd##x[i][j] = SumLog2Exact(g_Fwd##x[i][j], Score);							\
	if (DoTrace("Fwd", #x, i, j))													\
		{																			\
		bool Save = g_LocalTrace;													\
		g_LocalTrace = true;														\
		ScorePath(g_A, g_B, true, false, false, Path);								\
		g_LocalTrace = Save;														\
		Log("g_Fwd%s[%u][%u] +=  %12.12s = %12.12s\n",								\
		  #x, i, j, TypeToStr<float>(Score), TypeToStr<float>(g_Fwd##x[i][j]));		\
		}

	switch (LastState)
		{
	case 'M':
		add(M)
		break;
	case 'D':
		add(D)
		break;
	case 'I':
		add(I)
		break;
	case 'd':
	case 'i':
		break;
	default:
		asserta(false);
		}
#undef add
	}

static void OnPathBwd(const string &Path)
	{
	if (!ValidPath(Path, false))
		return;

	if (g_LocalTrace)
		{
		Log("\n");
		Log("OnPathBwd(%s)\n", Path.c_str());
		}

	unsigned si;
	unsigned sj;
	GetLetterCounts(Path, si, sj);
	char FirstState = *Path.begin();

	unsigned pi = g_LA - si;
	unsigned pj = g_LB - sj;

	unsigned i = pi;
	unsigned j = pj;

	int u = toupper(FirstState);
	if (u == 'M' || u == 'D')
		++i;
	if (u == 'M' || u == 'I')
		++j;

	float Score = ScorePath(g_A+pi, g_B+pj, false, true, true, Path);

#define add(x)																		\
	g_Bwd##x[i][j] = SumLog2Exact(g_Bwd##x[i][j], Score);							\
	if (DoTrace("Bwd", #x, i, j))													\
		{																			\
		bool Save = g_LocalTrace;													\
		g_LocalTrace = true;														\
		ScorePath(g_A+pi, g_B+pj, false, true, true, Path);								\
		g_LocalTrace = Save;														\
		string s1, s2;																\
		Log("g_Bwd%s[%u][%u] +=  %12.12s = %12.12s\n",								\
		  #x, i, j, FloatToStr(Score, s1), FloatToStr(g_Bwd##x[i][j], s2));			\
		}

	switch (FirstState)
		{
	case 'M':
		add(M)
		break;
	case 'D':
		add(D)
		break;
	case 'I':
		add(I)
		break;
	case 'd':
	case 'i':
		break;
	default:
		asserta(false);
		}
#undef add
	}

void BruteFwdBwdSW(const byte *A, const byte *B, unsigned LA, unsigned LB)
	{
	Log("BruteFwdBwdSW()\n");
	Log("A=%s\n", A);
	Log("B=%s\n", B);
	Log("\n");

	g_A = A;
	g_B = B;
	g_LA = LA;
	g_LB = LB;
	g_SumPaths = float(0);
	g_FwdMf.Alloc("FwdM", LA+1, LB+1);
	g_FwdDf.Alloc("FwdD", LA+1, LB+1);
	g_FwdIf.Alloc("FwdI", LA+1, LB+1);

	g_BwdMf.Alloc("BwdM", LA+1, LB+1);
	g_BwdDf.Alloc("BwdD", LA+1, LB+1);
	g_BwdIf.Alloc("BwdI", LA+1, LB+1);

	g_FwdBwdMf.Alloc("FwdBwdM", LA+1, LB+1);
	g_FwdBwdDf.Alloc("FwdBwdD", LA+1, LB+1);
	g_FwdBwdIf.Alloc("FwdBwdI", LA+1, LB+1);

	g_FwdBwdMf2.Alloc("FwdBwdM2", LA+1, LB+1);
	g_FwdBwdDf2.Alloc("FwdBwdD2", LA+1, LB+1);
	g_FwdBwdIf2.Alloc("FwdBwdI2", LA+1, LB+1);

	g_PPf.Alloc("PP", LA+1, LB+1);

	g_FwdMf.Init(LOG_ZERO);
	g_FwdDf.Init(LOG_ZERO);
	g_FwdIf.Init(LOG_ZERO);

	g_BwdMf.Init(LOG_ZERO);
	g_BwdDf.Init(LOG_ZERO);
	g_BwdIf.Init(LOG_ZERO);

	g_FwdBwdMf.Init(LOG_ZERO);
	g_FwdBwdDf.Init(LOG_ZERO);
	g_FwdBwdIf.Init(LOG_ZERO);

	g_FwdBwdMf2.Init(LOG_ZERO);
	g_FwdBwdDf2.Init(LOG_ZERO);
	g_FwdBwdIf2.Init(LOG_ZERO);

	g_PPf.Init(0);

	g_FwdM = g_FwdMf.GetData();
	g_FwdD = g_FwdDf.GetData();
	g_FwdI = g_FwdIf.GetData();

	g_BwdM = g_BwdMf.GetData();
	g_BwdD = g_BwdDf.GetData();
	g_BwdI = g_BwdIf.GetData();

	g_FwdBwdM = g_FwdBwdMf.GetData();
	g_FwdBwdD = g_FwdBwdDf.GetData();
	g_FwdBwdI = g_FwdBwdIf.GetData();

	g_FwdBwdM2 = g_FwdBwdMf2.GetData();
	g_FwdBwdD2 = g_FwdBwdDf2.GetData();
	g_FwdBwdI2 = g_FwdBwdIf2.GetData();

	g_PP = g_PPf.GetData();

	g_SumPaths = LOG_ZERO;
	EnumPathsSW(g_LA, g_LB, OnPathFwdBwd);
	EnumPathsSW(g_LA, g_LB, OnPathFwd);
	EnumPathsSW(g_LA, g_LB, OnPathBwd);

	float SumPaths2 = LOG_ZERO;
	for (unsigned i = 1; i <= LA; ++i)
		for (unsigned j = 1; j <= LB; ++j)
			{
			SumPaths2 = SumLog2Exact(SumPaths2, g_FwdM[i][j]);
			// Log(" %u,%u += %g = %g\n", i, j, g_FwdM[i][j], SumPaths2);//@@
			g_PP[i][j] = exp(g_FwdBwdM[i][j] - g_SumPaths);
			}

	for (unsigned i = 0; i <= LA; ++i)
		for (unsigned j = 0; j <= LB; ++j)
			{
			g_FwdBwdM2[i][j] = g_FwdM[i][j] + g_BwdM[i][j];
			g_FwdBwdD2[i][j] = g_FwdD[i][j] + g_BwdD[i][j];
			g_FwdBwdI2[i][j] = g_FwdI[i][j] + g_BwdI[i][j];
			}

	g_BwdDf.LogMe();
	g_FwdDf.LogMe();
	g_FwdBwdDf.LogMe();
	g_FwdBwdDf2.LogMe();

	g_FwdIf.LogMe();
	g_BwdIf.LogMe();
	g_FwdBwdIf.LogMe();
	g_FwdBwdIf2.LogMe();

	g_FwdMf.LogMe();
	g_BwdMf.LogMe();
	g_FwdBwdMf.LogMe();
	g_FwdBwdMf2.LogMe();

	g_PPf.LogMe();

	Log("\n");
	Log("SumPaths  = %g\n", g_SumPaths);
	Log("SumPaths2 = %g\n", SumPaths2);

	bool OkM = g_FwdBwdMf.Eq(g_FwdBwdMf2);
	bool OkD = g_FwdBwdDf.Eq(g_FwdBwdDf2);
	bool OkI = g_FwdBwdIf.Eq(g_FwdBwdIf2);

	if (!OkM || !OkD || !OkI)
		Die("Cmp");
	}

void Test()
	{
	g_DebugMx = "Bwd";
	g_DebugState = 'D';
	g_Debugi = 1;
	g_Debugj = 0;

	if (g_DebugMx != "")
		g_LocalTrace = false;

	const byte *A = (const byte *) "SEQVENCE";
	const byte *B = (const byte *) "MQTIF";
	const unsigned LA = (unsigned) strlen((const char *) A);
	const unsigned LB = (unsigned) strlen((const char *) B);
	BruteFwdBwdSW(A, B, LA, LB);
	}
