#include "myutils.h"
#include "mx.h"
#include "seqdb.h"

void GetLetterCounts(const string &Path, unsigned &i, unsigned &j);

float SW(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &Mxf,
  float t, float e, unsigned &Starti, unsigned &Startj, string &Path);
void LogLocalAln(SeqDB &DB, unsigned IdA, unsigned IdB,
  unsigned Starti, unsigned Startj, const string &Path);

void MultiSW(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &PPMx,
  float t, float e, vector<unsigned> &Startis, vector<unsigned> &Startjs,
  vector<string> &Paths, vector<float> &Scores)
	{
	Mx<float> Mxf;
	Mxf.Copy(PPMx);

	Startis.clear();
	Startjs.clear();
	Paths.clear();
 
	unsigned LA = DB.GetSeqLength(IdA);
	unsigned LB = DB.GetSeqLength(IdB);

	float **MxData = Mxf.GetData();
	if (opt_minlocalprob < 0)
		{
		float minprob = -opt_minlocalprob;
		t = 0;
		for (unsigned i = 1; i <= LA; ++i)
			for (unsigned j = 1; j <= LB; ++j)
				if (MxData[i][j] < minprob)
					MxData[i][j] = -1.0f;
		}

	for (;;)
		{
		if (SIZE(Paths) > 8)
			return;//@@
		unsigned Starti;
		unsigned Startj;
		string Path;
		float Score = SW(DB, IdA, IdB, Mxf, t, e, Starti, Startj, Path);
		if (Path.empty())
			return;

		Paths.push_back(Path);
		Startis.push_back(Starti);
		Startjs.push_back(Startj);
		Scores.push_back(Score);

		unsigned i = Starti;
		unsigned j = Startj;
		unsigned L = SIZE(Path);
		const int b = (int) opt_mmband;
		for (unsigned k = 0; k < L; ++k)
			{
			char c = Path[k];
			for (int ii = (int) i - b; ii <= (int) i + b; ++ii)
				{
				for (int jj = (int) j - b; jj <= (int) j + b; ++jj)
					{
					if (ii < 0 || ii >= (int) LA || jj < 0 || jj >= (int) LB)
						continue;
					MxData[ii+1][jj+1] = -1.0f;
					}
				}
			if (c == 'M' || c == 'D')
				++i;
			if (c == 'M' || c == 'I')
				++j;
			}

		if (opt_trace)
			Mxf.LogMe();
		}
	}
