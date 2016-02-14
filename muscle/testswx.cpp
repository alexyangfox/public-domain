#include "myutils.h"
#include "mx.h"
#include "sumlog.h"
#include "seqdb.h"
#include "params.h"

float FwdBwdSWXCRF(Mx<float> &PPMx);
void SetSimMxX(SeqDB &DB, unsigned IdA, unsigned IdB);

typedef void (*ON_PATH)(const string &Path);

static float g;
static float e;

static const byte *g_A;
static const byte *g_B;
static unsigned g_LA;
static unsigned g_LB;

static string g_DebugMx;
static int g_DebugState;
static unsigned g_Debugi;
static unsigned g_Debugj;

static bool g_LocalTrace = false;

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
static Mx<float> g_PPf2;

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

//static bool IsFirstState(char State)
//	{
//	return State == 'M' || State == 'D' || State == 'x' || State == 'X';
//	}
//
//static bool IsSecondState(char State)
//	{
//	return State == 'M' || State == 'I' || State == 'y' || State == 'Y';
//	}

static unsigned GetFirstStateCount(char c)
	{
	switch (c)
		{
	case 'M':
	case 'D':
		return 3;
	case 'X':
	case 'x':
		return 1;
		}
	return 0;
	}

static unsigned GetSecondStateCount(char c)
	{
	switch (c)
		{
	case 'M':
	case 'I':
		return 3;
	case 'Y':
	case 'y':
		return 1;
		}
	return 0;
	}

static void GetLetterCounts(const string &Path, unsigned &i, unsigned &j)
	{
	i = 0;
	j = 0;
	const unsigned L = SIZE(Path);
	for (unsigned k = 0; k < L; ++k)
		{
		int c = Path[k];
		i += GetFirstStateCount(c);
		j += GetSecondStateCount(c);
		}
	}

static bool IsFullPath(const string &Path)
	{
	unsigned i;
	unsigned j;
	GetLetterCounts(Path, i, j);
	return i == g_LA && j == g_LB;
	}

static bool ValidTrans(char State1, char State2)
	{
#define t(x, y)		if (State1 == #x[0] && State2 == #y[0]) return true;

	t(S, x)
	t(S, y)
	t(S, M)

	t(M, M)
	t(M, D)
	t(M, I)
	t(M, X)
	t(M, Y)
	t(M, E)

	t(D, D)
	t(D, M)

	t(I, I)
	t(I, M)

	t(x, x)
	t(x, y)
	t(x, M)
	t(x, E)

	t(y, y)
	t(y, M)
	t(y, E)

	t(X, X)
	t(X, Y)
	t(X, E)

	t(Y, Y)
	t(Y, E)

#undef t
	return false;
	}

static float TransScore(char State1, char State2)
	{
#define t(x, y, s)		if (State1 == #x[0] && State2 == #y[0]) return float(s);

	t(S, x, 0)
	t(S, y, 0)
	t(S, M, 0)

	t(M, M, 0)
	t(M, D, g)
	t(M, I, g)
	t(M, X, 0)
	t(M, Y, 0)
	t(M, E, 0)

	t(D, D, e)
	t(D, M, 0)

	t(I, I, e)
	t(I, M, 0)

	t(x, x, 0)
	t(x, y, 0)
	t(x, M, 0)
	t(x, E, 0)

	t(y, y, 0)
	t(y, M, 0)
	t(y, E, 0)

	t(X, X, 0)
	t(X, Y, 0)
	t(X, E, 0)

	t(Y, Y, 0)
	t(Y, E, 0)

#undef t
	asserta(false);
	ureturn(0);
	}

static void EnumPathsSWXFwdRecurse(const string &Path, unsigned LA, unsigned LB,
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

	char PrevState = Path.empty() ? 'S' : *Path.rbegin();
	if (i + 3 <= LA && j + 3 <= LB && ValidTrans(PrevState,'M'))
		EnumPathsSWXFwdRecurse(Path + "M", LA, LB, OnPath);

	if (i > 0 && i + 3 < LA && ValidTrans(PrevState,'D'))
		EnumPathsSWXFwdRecurse(Path + "D", LA, LB, OnPath);

	if (j > 0 && j + 3 < LB && ValidTrans(PrevState,'I'))
		EnumPathsSWXFwdRecurse(Path + "I", LA, LB, OnPath);

	if (i < LA && ValidTrans(PrevState,'x'))
		EnumPathsSWXFwdRecurse(Path + 'x', LA, LB, OnPath);

	if (i < LA && ValidTrans(PrevState,'X'))
		EnumPathsSWXFwdRecurse(Path + 'X', LA, LB, OnPath);

	if (j < LB && ValidTrans(PrevState,'y'))
		EnumPathsSWXFwdRecurse(Path + 'y', LA, LB, OnPath);

	if (j < LB && ValidTrans(PrevState,'Y'))
		EnumPathsSWXFwdRecurse(Path + 'Y', LA, LB, OnPath);
	}

static void EnumPathsSWXBwdRecurse(const string &Path, unsigned LA, unsigned LB,
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

	char NextState = Path.empty() ? 'E' : *Path.begin();
	if (i + 3 <= LA && j + 3 <= LB && ValidTrans('M', NextState))
		EnumPathsSWXBwdRecurse("M" + Path, LA, LB, OnPath);

	if (i + 3 < LA && ValidTrans('D', NextState))
		EnumPathsSWXBwdRecurse("D" + Path, LA, LB, OnPath);

	if (j + 3 < LB && ValidTrans('I', NextState))
		EnumPathsSWXBwdRecurse("I" + Path, LA, LB, OnPath);

	if (i < LA && ValidTrans('X', NextState))
		EnumPathsSWXBwdRecurse('X' + Path, LA, LB, OnPath);

	if (i < LA && ValidTrans('x', NextState))
		EnumPathsSWXBwdRecurse('x' + Path, LA, LB, OnPath);

	if (j < LB && ValidTrans('Y', NextState))
		EnumPathsSWXBwdRecurse('Y' + Path, LA, LB, OnPath);

	if (j < LB && ValidTrans('y', NextState))
		EnumPathsSWXBwdRecurse('y' + Path, LA, LB, OnPath);
	}

static void EnumPathsSWXFwd(unsigned LA, unsigned LB, ON_PATH OnPath)
	{
	string Path;
	EnumPathsSWXFwdRecurse(Path, LA, LB, OnPath);
	}

static void EnumPathsSWXBwd(unsigned LA, unsigned LB, ON_PATH OnPath)
	{
	string Path;
	EnumPathsSWXBwdRecurse(Path, LA, LB, OnPath);
	}

byte CodonToAA(const byte *DNA);
static void LogCodon(const byte *A, unsigned i, unsigned L)
	{
	bool Valid = true;
	for (int k = 2; k >= 0; --k)
		{
		int j = (int) i - k;
		if (j < 0 || j >= (int) L)
			{
			Valid = false;
			Log("#");
			}
		else
			Log("%c", A[j]);
		}
	if (Valid)
		Log("(%c)", CodonToAA(A+i-2));
	}

static void LogSub(const byte *A, const byte *B, unsigned i, unsigned j,
  unsigned LA, unsigned LB)
	{
	Log("  ");
	LogCodon(A, i, LA);
	Log("-");
	LogCodon(B, j, LB);
	}

static float ScorePath(const byte *A, const byte *B, bool Start, bool End,
  bool Bwd, const string &Path)
	{
	float **Mx = GetSimMx();

	if (g_LocalTrace)
		{
		Log("\n");
		Log("ScorePath(%s%s%s)\n", Start ? "S" : "", Path.c_str(), End ? "E" : "");
		}

	const unsigned L = SIZE(Path);
	unsigned i = 0;
	unsigned j = 0;
	float Score = 0.0f;
	int LastState = 'S';
	if (g_LocalTrace)
		{
		Log("  i    j  xx           Sub         Trans         Total\n");
		Log("---  ---  --  ------------  ------------  ------------\n");
		}
	for (unsigned k = 0; k < L; ++k)
		{
		int State = Path[k];

		if (g_LocalTrace)
			Log("%3u  %3u  %c%c", i, j, LastState, State);

		float Sub = 0;
		float Trans = 0;
		if (State == 'M')
			Sub = Mx[i+3][j+3];

		if (!(k == 0 && Bwd)) 
			{
			Trans = TransScore(LastState, State);
			Score += Sub + Trans;
			if (g_LocalTrace)
				{
				string s;
				if (State == 'M')
					Log("  %12.12s", FloatToStr(Sub, s));
				else
					Log("  %12.12s", "");
				Log("  %12.12s", FloatToStr(Trans, s));
				Log("  %12.12s", FloatToStr(Score, s));
				if (g_LocalTrace && State == 'M')
					LogSub(A, B, i+2, j+2, g_LA, g_LB);
				}
			}
		if (g_LocalTrace)
			Log("\n");

		i += GetFirstStateCount(State);
		j += GetSecondStateCount(State);

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
	if (!IsFullPath(Path))
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
			i += 3;
			j += 3;
			add(M)
			break;
			}
		case 'D':
			{
			i += 3;
			add(D)
			break;
			}
		case 'I':
			{
			j += 3;
			add(I)
			break;
			}
		case 'x':
		case 'X':
			++i;
			break;
		case 'y':
		case 'Y':
			++j;
			break;
			}
		}
#undef add
	}

static void OnPathFwd(const string &Path)
	{
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
		Log("g_Fwd%s[%u][%u] +=  %12.12s",											\
		  #x, i, j, TypeToStr<float>(Score));										\
	    Log(" = %12.12s\n", TypeToStr<float>(g_Fwd##x[i][j]));						\
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
	case 'x':
	case 'y':
	case 'X':
	case 'Y':
		break;
	default:
		asserta(false);
		}
#undef add
	}

static void OnPathBwd(const string &Path)
	{
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
		i += 3;
	if (u == 'M' || u == 'I')
		j += 3;

	float Score = ScorePath(g_A+pi, g_B+pj, false, true, true, Path);

#define add(x)																		\
	g_Bwd##x[i][j] = SumLog2Exact(g_Bwd##x[i][j], Score);							\
	if (DoTrace("Bwd", #x, i, j))													\
		{																			\
		bool Save = g_LocalTrace;													\
		g_LocalTrace = true;														\
		ScorePath(g_A+pi, g_B+pj, false, true, true, Path);							\
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
	case 'x':
	case 'y':
	case 'X':
	case 'Y':
		break;
	default:
		asserta(false);
		}
#undef add
	}

void BruteFwdBwdSW(const byte *A, const byte *B, unsigned LA, unsigned LB)
	{
	Log("BruteFwdBwdSW()\n");
	Log("A=%*.*s\n", LA, LA, A);
	Log("B=%*.*s\n", LB, LB, B);
	Log("\n");

	g_A = A;
	g_B = B;
	g_LA = LA;
	g_LB = LB;
	g_SumPaths = float(0);
	g_FwdMf.Alloc("BruteFwdM", LA+1, LB+1);
	g_FwdDf.Alloc("BruteFwdD", LA+1, LB+1);
	g_FwdIf.Alloc("BruteFwdI", LA+1, LB+1);

	g_BwdMf.Alloc("BruteBwdM", LA+1, LB+1);
	g_BwdDf.Alloc("BruteBwdD", LA+1, LB+1);
	g_BwdIf.Alloc("BruteBwdI", LA+1, LB+1);

	g_FwdBwdMf.Alloc("BruteFwdBwdM", LA+1, LB+1);
	g_FwdBwdDf.Alloc("BruteFwdBwdD", LA+1, LB+1);
	g_FwdBwdIf.Alloc("BruteFwdBwdI", LA+1, LB+1);

	g_FwdBwdMf2.Alloc("BruteFwdBwdM2", LA+1, LB+1);
	g_FwdBwdDf2.Alloc("BruteFwdBwdD2", LA+1, LB+1);
	g_FwdBwdIf2.Alloc("BruteFwdBwdI2", LA+1, LB+1);

	g_PPf.Alloc("BrutePP", LA+1, LB+1);

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
	EnumPathsSWXFwd(g_LA, g_LB, OnPathFwdBwd);
	EnumPathsSWXFwd(g_LA, g_LB, OnPathFwd);
	EnumPathsSWXBwd(g_LA, g_LB, OnPathBwd);

	float SumPaths2 = LOG_ZERO;
	for (unsigned i = 1; i <= LA; ++i)
		for (unsigned j = 1; j <= LB; ++j)
			{
			SumPaths2 = SumLog2Exact(SumPaths2, g_FwdM[i][j]);
			g_PP[i][j] = exp(g_FwdBwdM[i][j] - g_SumPaths);
			}
	SumPaths2 = SumLog2(SumPaths2, 0);

	for (unsigned i = 0; i <= LA; ++i)
		for (unsigned j = 0; j <= LB; ++j)
			{
			g_FwdBwdM2[i][j] = g_FwdM[i][j] + g_BwdM[i][j];
			g_FwdBwdD2[i][j] = g_FwdD[i][j] + g_BwdD[i][j];
			g_FwdBwdI2[i][j] = g_FwdI[i][j] + g_BwdI[i][j];
			}

	Log("\n");
	Log("SumPaths  = %g\n", g_SumPaths);
	Log("SumPaths2 = %g\n", SumPaths2);

	bool OkM = g_FwdBwdMf.Eq(g_FwdBwdMf2);
	bool OkD = g_FwdBwdDf.Eq(g_FwdBwdDf2);
	bool OkI = g_FwdBwdIf.Eq(g_FwdBwdIf2);

	if (!OkM || !OkD || !OkI)
		{
		MxBase::LogAll();
		Die("Cmp");
		}

	assertaeq(g_SumPaths, SumPaths2);
	}

void TestSWX(SeqDB &DB)
	{
	SetModel("localaffnucx");

	g = TransMD;
	e = TransMM;

	g_DebugMx = "Fwd";
	g_DebugState = 'M';
	g_Debugi = 4;
	g_Debugj = 3;

	g_LocalTrace = opt_trace;
	if (g_DebugMx != "")
		g_LocalTrace = false;

	const byte *A = DB.GetSeq(0);
	const byte *B = DB.GetSeq(1);
	const unsigned LA = DB.GetSeqLength(0);
	const unsigned LB = DB.GetSeqLength(1);

	SetSimMxX(DB, 0, 1);
	BruteFwdBwdSW(A, B, LA, LB);
	FwdBwdSWXCRF(g_PPf2);

	//Mx<float> &DPFwdM = *(Mx<float> *) MxBase::Get("SWXCRF_FwdM");
	//Mx<float> &DPBwdM = *(Mx<float> *) MxBase::Get("SWXCRF_BwdM");
	//Mx<float> &DPFwdD = *(Mx<float> *) MxBase::Get("SWXCRF_FwdD");
	//Mx<float> &DPFwdI = *(Mx<float> *) MxBase::Get("SWXCRF_FwdI");
	Mx<float> &PP = *(Mx<float> *) MxBase::Get("SWXCRF_PP");
	MxBase::LogAll();

//	bool OkM = DPFwdM.Eq(g_FwdMf);
//	bool OkMB = DPBwdM.Eq(g_BwdMf);
	//bool OkD = DPFwdD.Eq(g_FwdDf);
	//bool OkI = DPFwdI.Eq(g_FwdIf);
	bool OkPP = PP.Eq(g_PPf);
//	if (!OkM || !OkD || !OkI || !OkPP)
//	if (!OkM || !OkPP || !OkMB)
	if (!OkPP)
		Die("Cmp");
	}
