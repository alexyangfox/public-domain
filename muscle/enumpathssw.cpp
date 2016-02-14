#include "myutils.h"
#include "mx.h"
#include "sumlog.h"

typedef void (*ON_PATH)(const string &Path);

static const byte *g_A;
static const byte *g_B;
static unsigned g_LA;
static unsigned g_LB;

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

static float **g_PP;

static float g_SumPaths;

static void OnCandidatePath(const string &Prefix,
  const string &Suffix, string Path, ON_PATH OnPath)
	{
	if (Path.empty())
		return;
	char First = *Path.begin();
	if (First == 'D' || First == 'I')
		return;
	char Last = *Path.rbegin();
	if (Last == 'D' || Last == 'I')
		return;
	OnPath(Prefix + Path + Suffix);
	}

static void EnumGlobalPathsRecurse(unsigned n1, unsigned n2,
  unsigned L1, unsigned L2, bool SubPaths, const string &Prefix,
  const string &Suffix, string Path, ON_PATH OnPath)
	{
	if (SubPaths)
		OnCandidatePath(Prefix, Suffix, Path, OnPath);
	if (n1 == L1 && n2 == L2)
		{
		if (!SubPaths)
			OnCandidatePath(Prefix, Suffix, Path, OnPath);
		return;
		}
	
	char LastLetter = 0;
	if (!Path.empty())
		LastLetter = *Path.rbegin();

	if (n1 < L1 && n2 < L2)
		EnumGlobalPathsRecurse(n1 + 1, n2 + 1, L1, L2, SubPaths,
		  Prefix, Suffix, Path + 'M', OnPath);

	if (n1 < L1 && LastLetter != 'I')
		EnumGlobalPathsRecurse(n1 + 1, n2, L1, L2, SubPaths,
		Prefix, Suffix, Path + 'D', OnPath);

	if (n2 < L2 && LastLetter != 'D')
		EnumGlobalPathsRecurse(n1, n2 + 1, L1, L2, SubPaths,
		  Prefix, Suffix, Path + 'I', OnPath);
	}

static void EnumGlobalPaths(unsigned L1, unsigned L2, bool SubPaths,
  const string &Prefix, const string &Suffix, ON_PATH OnPath)
	{
	string Path;
	EnumGlobalPathsRecurse(0, 0, L1, L2, SubPaths, Prefix, Suffix, Path, OnPath);
	}

void EnumPathsSW(unsigned LA, unsigned LB, bool SubPaths, ON_PATH OnPath)
	{
	for (unsigned iFrom = 0; iFrom < LA; ++iFrom)
		for (unsigned iTo = iFrom; iTo < LA; ++iTo)
			for (unsigned jFrom = 0; jFrom < LB; ++jFrom)
				for (unsigned jTo = jFrom; jTo < LB; ++jTo)
					{
					string PathPrefix;
					for (unsigned i = 0; i < iFrom; ++i)
						PathPrefix.push_back('d');
					for (unsigned j = 0; j < jFrom; ++j)
						PathPrefix.push_back('i');

					string PathSuffix;
					for (unsigned i = iTo+1; i < LA; ++i)
						PathSuffix.push_back('d');
					for (unsigned j = jTo+1; j < LB; ++j)
						PathSuffix.push_back('i');

					unsigned L1 = iTo - iFrom + 1;
					unsigned L2 = jTo - jFrom + 1;
					EnumGlobalPaths(L1, L2, SubPaths, PathPrefix, PathSuffix, OnPath);
					}
	}

static float ScorePath(const byte *A, const byte *B,
  const string &Path, bool Fwd, bool ValidateLengths)
	{
	const float g = -5.7f;
	const float e = -0.25f;
	extern float **g_VTML200;
	float **Mx = g_VTML200;

	if (g_LocalTrace)
		{
		Log("\n");
		Log("Score%sPath(%s)\n", Fwd ? "Fwd" : "Bwd", Path.c_str());
		}

	const unsigned L = SIZE(Path);
	unsigned i = 0;
	unsigned j = 0;
	float Score = 0.0f;
	int LastState = 'S';
	if (g_LocalTrace)
		{
		Log("  i    j  xx        What    Score         Total\n");
		Log("---  ---  --  ----------  -------  ------------\n");
		}
	for (unsigned k = 0; k < L; ++k)
		{
		int State = Path[k];
		if (k == 0 && !Fwd)
			{
			switch (toupper(State))
				{
			case 'M':
				++i;
				++j;
				break;
			case 'D':
				++i;
				break;
			case 'I':
				++j;
				break;
			default:
				asserta(false);
				}
			LastState = State;
			continue;
			}
		if (g_LocalTrace)
			Log("%3u  %3u  %c%c  ", i, j, LastState, State);
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
			if (LastState == 'M')
				Score += e;
			else
				Score += g;
			if (g_LocalTrace)
				{
				if (LastState == 'M')
					Log("  open      %7.4f", g);
				else
					Log("   ext      %7.4f", e);
				}
			++i;
			break;
		case 'I':
			if (LastState == 'M')
				Score += e;
			else
				Score += g;
			if (g_LocalTrace)
				{
				if (LastState == 'M')
					Log("  open      %7.4f", g);
				else
					Log("   ext      %7.4f", e);
				}
			++j;
			break;
		case 'd':
			Log("                   ");
			++i;
			break;
		case 'i':
			Log("                   ");
			++j;
			break;
		default:
			asserta(false);
			}
		if (g_LocalTrace)
			Log("  %12.5g\n", Score);
		LastState = State;
		}

	if (ValidateLengths)
		asserta(i == g_LA && j == g_LB);

	if (g_LocalTrace)
		Log("Score%sPath(%s)=%g\n", Fwd ? "Fwd" : "Bwd", Path.c_str(), Score);
	return Score;
	}

static float ScoreFwdPath(const byte *A, const byte *B, const string &Path)
	{
	return ScorePath(A, B, Path, true, false);
	}

static float ScoreBwdPath(const byte *A, const byte *B, const string &Path)
	{
	return ScorePath(A, B, Path, false, false);
	}

static void OnPath(const string &Path)
	{
	if (g_LocalTrace)
		{
		Log("\n");
		Log("OnPath(%s)\n", Path.c_str());
		}
	float Score = ScorePath(g_A, g_B, Path, true, true);
	g_SumPaths = SumLog2(g_SumPaths, Score);

	unsigned i = 0;
	unsigned j = 0;
	const unsigned L = SIZE(Path);
	for (unsigned k = 0; k < L; ++k)
		{
		int State = Path[k];
		string FwdPath = Path.substr(0, k+1);
		string BwdPath = Path.substr(k, L-k);
		if (g_LocalTrace)
			Log("k=%3u FwdPath=%s BwdPath=%s\n", k, FwdPath.c_str(), BwdPath.c_str());
		asserta(FwdPath + BwdPath.substr(1) == Path);
		float FwdScore = ScoreFwdPath(g_A, g_B, FwdPath);
		float BwdScore = ScoreBwdPath(g_A+i, g_B+j, BwdPath);

#define add(x)			\
		g_FwdBwd##x[i][j] = SumLog2(g_FwdBwd##x[i][i], Score);	\
		g_Fwd##x[i][j] = SumLog2(g_Fwd##x[i][j], FwdScore);		\
		g_Bwd##x[i][j] = SumLog2(g_Bwd##x[i][j], BwdScore);		\
		if (g_LocalTrace)										\
			{													\
			Log("g_FwdBwd%s[%u][%u] += %12.5g = %12.5g\n", #x, i, j, Score, g_FwdBwd##x[i][j]);		\
			Log("g_Fwd%s[%u][%u]    += %12.5g = %12.5g\n", #x, i, j, FwdScore, g_Fwd##x[i][j]);		\
			Log("g_Bwd%s[%u][%u]    += %12.5g = %12.5g\n", #x, i, j, BwdScore, g_Bwd##x[i][j]);		\
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
			break;
			}
		}
#undef add
	}

void BruteFwdBwdSW(const byte *A, const byte *B, unsigned LA, unsigned LB)
	{
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

	g_PP = g_PPf.GetData();

	g_SumPaths = LOG_ZERO;
	EnumPathsSW(g_LA, g_LB, false, OnPath);

	for (unsigned i = 1; i <= LA; ++i)
		for (unsigned j = 1; j <= LB; ++j)
			g_PP[i][j] = exp(g_FwdBwdM[i][j] - g_SumPaths);

	g_FwdMf.LogMe();
	g_FwdDf.LogMe();
	g_FwdIf.LogMe();

	g_BwdMf.LogMe();
	g_BwdDf.LogMe();
	g_BwdIf.LogMe();

	g_FwdBwdMf.LogMe();
	g_FwdBwdDf.LogMe();
	g_FwdBwdIf.LogMe();

	g_PPf.LogMe();

	Log("\n");
	Log("SumPaths=%g\n", g_SumPaths);
	}

void Test()
	{
	const byte *A = (const byte *) "WAC";
	const byte *B = (const byte *) "WC";
	const unsigned LA = (unsigned) strlen((const char *) A);
	const unsigned LB = (unsigned) strlen((const char *) B);
	BruteFwdBwdSW(A, B, LA, LB);
	}
