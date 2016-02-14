#include "myutils.h"
#include "mx.h"
#include "best.h"
#include "seqdb.h"
#include <algorithm>

static Mx<float> g_FwdM;
static Mx<char> g_TB;

void LogLocalAln(SeqDB &DB, unsigned IdA, unsigned IdB,
  unsigned Starti, unsigned Startj, const string &Path)
	{
	string a;
	string b;

	const byte *A = DB.GetSeq(IdA);
	const byte *B = DB.GetSeq(IdB);

	const string &LabelA = DB.GetLabel(IdA);
	const string &LabelB = DB.GetLabel(IdB);

	const unsigned L = SIZE(Path);
	unsigned i = Starti;
	unsigned j = Startj;
	for (unsigned k = 0; k < L; ++k)
		{
		char c = Path[k];
		if (c == 'M')
			{
			a.push_back(A[i++]);
			b.push_back(B[j++]);
			}
		else if (c == 'D')
			{
			a.push_back(A[i++]);
			b.push_back('-');
			}
		else if (c == 'I')
			{
			a.push_back('-');
			b.push_back(B[j++]);
			}
		else
			asserta(false);
		}
	
	Log("%16.16s %5u %s %u\n", LabelA.c_str(), Starti+1, a.c_str(), i);
	Log("%16.16s %5u %s %u\n", LabelB.c_str(), Startj+1, b.c_str(), j);
	}

void TrimLocalPath(string &Path)
	{
	const unsigned L = SIZE(Path);
	unsigned GapStart = UINT_MAX;
	unsigned DCount = 0;
	unsigned ICount = 0;
	for (unsigned i = 0; i < L; ++i)
		{
		char c = Path[i];
		switch (c)
			{
		case 'M':
			GapStart = UINT_MAX;
			DCount = 0;
			ICount = 0;
			break;

		case 'D':
			if (GapStart == UINT_MAX)
				{
				GapStart = i;
				DCount = 0;
				ICount = 0;
				}
			++DCount;
			break;

		case 'I':
			if (GapStart == UINT_MAX)
				{
				GapStart = i;
				DCount = 0;
				ICount = 0;
				}
			++ICount;
			break;

		default:
			asserta(false);
			}

		bool Trunc = (DCount + ICount > opt_maxlocalgap);
		Trunc = Trunc || (DCount > 0 && ICount > 0 && DCount + ICount > opt_maxlocaldi);
		if (Trunc)
			{
			Path = Path.substr(0, GapStart);
			return;
			}
		}
	}

float SW(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &Mxf,
  float t, float e, unsigned &Starti, unsigned &Startj, string &Path)
	{
	unsigned LA = DB.GetSeqLength(IdA);
	unsigned LB = DB.GetSeqLength(IdB);

	float **MxData = Mxf.GetData();

	Path.clear();
	Starti = UINT_MAX;
	Startj = UINT_MAX;

	g_FwdM.Alloc("SWFwdM", LA+1, LB+1, &DB, IdA, IdB);
	g_TB.Alloc("SWTB", LA+1, LB+1, &DB, IdA, IdB);
	float **FwdM = g_FwdM.GetData();
	char **TB = g_TB.GetData();

	for (unsigned i = 0; i <= LA; ++i)
		{
		TB[i][0] = 'S';
		FwdM[i][0] = 0;
		}

	for (unsigned j = 0; j <= LB; ++j)
		{
		TB[0][j] = 'S';
		FwdM[0][j] = 0;
		}

// Main loop
	float BestScore = 0;
	unsigned Besti = UINT_MAX;
	unsigned Bestj = UINT_MAX;
	for (unsigned i = 1; i <= LA; ++i)
		{
		const float *MxRow = MxData[i];
		for (unsigned j = 1; j <= LB; ++j)
			{
			float Match = MxRow[j] - t;
			float MM = FwdM[i-1][j-1] + Match;
			float DM = FwdM[i-1][j] + e;
			float IM = FwdM[i][j-1] + e;
			float SM = 0;
			float Score;
			Best4(MM, DM, IM, SM, 'M', 'D', 'I', 'S', Score, TB[i][j]);
			FwdM[i][j] = Score;
			if (Score > BestScore)
				{
				BestScore = Score;
				Besti = i;
				Bestj = j;
				}
			}
		}

	if (opt_trace)
		{
		g_FwdM.LogMe();
		g_TB.LogMe();
		}

	if (BestScore == 0)
		return 0;

	unsigned i = Besti;
	unsigned j = Bestj;
	for (;;)
		{
		asserta(i >= 0 && j >= 0);
		char c = TB[i][j];
		if (c == 'S')
			break;
		Path.push_back(c);
		if (c == 'M' || c == 'D')
			--i;
		if (c == 'M' || c == 'I')
			--j;
		}
	reverse(Path.begin(), Path.end());
	Starti = i;
	Startj = j;

	if (opt_trace)
		{
		Log("%u, %u %s\n", Starti, Startj, Path.c_str());
		LogLocalAln(DB, IdA, IdB, Starti, Startj, Path);
		}
	TrimLocalPath(Path);
	if (SIZE(Path) < opt_minlocallen)
		{
		Path.clear();
		Starti = UINT_MAX;
		Startj = UINT_MAX;
		}
	return BestScore;
	}
