#include "myutils.h"
#include "mx.h"
#include "seqdb.h"
#include "best.h"
#include <algorithm>

void GetLetterCounts(const string &Path, unsigned &i, unsigned &j);
void TrimLocalPath(string &Path);

static Mx<float> g_FwdM;
static Mx<char> g_TB;

float SWX(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &Mxf,
  float t, float e, unsigned &Starti, unsigned &Startj, string &Path)
	{
	unsigned LA = DB.GetSeqLength(IdA);
	unsigned LB = DB.GetSeqLength(IdB);

	float **MxData = Mxf.GetData();

	Path.clear();
	Starti = UINT_MAX;
	Startj = UINT_MAX;

	g_FwdM.Alloc("SWXFwdM", LA+1, LB+1, &DB, IdA, IdB);
	g_TB.Alloc("SWXTB", LA+1, LB+1, &DB, IdA, IdB);
	float **FwdM = g_FwdM.GetData();
	char **TB = g_TB.GetData();

	for (unsigned i = 0; i <= LA; ++i)
		{
		for (unsigned j = 0; j < 3; ++j)
			{
			TB[i][j] = 'S';
			FwdM[i][j] = 0;
			}
		}

	for (unsigned j = 0; j <= LB; ++j)
		{
		for (unsigned i = 0; i < 3; ++i)
			{
			TB[i][j] = 'S';
			FwdM[i][j] = 0;
			}
		}

// Main loop
	float BestScore = 0;
	unsigned Besti = UINT_MAX;
	unsigned Bestj = UINT_MAX;
	for (unsigned i = 3; i <= LA; ++i)
		{
		const float *MxRow = MxData[i];
		for (unsigned j = 3; j <= LB; ++j)
			{
			float Match = MxRow[j] - t;
			float MM = FwdM[i-3][j-3] + Match;
			float DM = FwdM[i-3][j] + e;
			float IM = FwdM[i][j-3] + e;
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
		if (i <= 3 || j <= 3)
			break;
		char c = TB[i][j];
		if (c == 'S')
			break;
		Path.push_back(c);
		if (c == 'M' || c == 'D')
			i -= 3;
		if (c == 'M' || c == 'I')
			j -= 3;
		}
	reverse(Path.begin(), Path.end());
	Starti = i;
	Startj = j;

	TrimLocalPath(Path);
	if (SIZE(Path) < opt_minlocallen)
		{
		Path.clear();
		Starti = UINT_MAX;
		Startj = UINT_MAX;
		}
	return BestScore;
	}

void MultiSWX(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &PPMx,
  float t, float e, vector<unsigned> &Startis, vector<unsigned> &Startjs,
  vector<string> &Paths)
	{
	Mx<float> Mxf;
	Mxf.Copy(PPMx);

	Startis.clear();
	Startjs.clear();
	Paths.clear();
 
	unsigned LA = DB.GetSeqLength(IdA);
	unsigned LB = DB.GetSeqLength(IdB);

	float **MxData = Mxf.GetData();
	for (;;)
		{
		if (SIZE(Paths) > 8)
			return;//@@
		unsigned Starti;
		unsigned Startj;
		string Path;
		SWX(DB, IdA, IdB, Mxf, t, e, Starti, Startj, Path);
		if (Path.empty())
			return;

		Paths.push_back(Path);
		Startis.push_back(Starti);
		Startjs.push_back(Startj);

		unsigned i = Starti;
		unsigned j = Startj;
		unsigned L = SIZE(Path);
		int b = (int) opt_mmband;
		if (b < 3)
			b = 3;
		for (unsigned k = 0; k < L; ++k)
			{
			char c = Path[k];
			for (int ii = (int) i - b; ii <= (int) i + b; ++ii)
				{
				for (int jj = (int) j - b; jj <= (int) j + b; ++jj)
					{
					if (ii < 0 || ii >= (int) LA || jj < 0 || jj >= (int) LB)
						continue;
					MxData[ii+1][jj+1] = 0;
					}
				}
			if (c == 'M' || c == 'D')
				i += 3;
			if (c == 'M' || c == 'I')
				j += 3;
			}

		if (opt_trace)
			Mxf.LogMe();
		}
	}
