#include "myutils.h"
#include "seqdb.h"
#include "mx.h"
#include "params.h"
#include "sparsemx.h"
#include "info.h"
#include <algorithm>

void GetLocalModel(SeqDB &DB, string &Model);
void GetLetterCounts(const string &Path, unsigned &i, unsigned &j);
void MultiSW(SeqDB &DB, unsigned SeqIndexA, unsigned SeqIndexB, Mx<float> &PPMx,
  float t, float e, vector<unsigned> &Startis, vector<unsigned> &Startjs,
  vector<string> &Paths, vector<float> &Scores);
void WriteMx(const string &Name, Mx<float> &Mxf);

void MaskSimMx(unsigned FromA, unsigned ToA, unsigned FromB, unsigned ToB)
	{
	float **SimMx = GetSimMx();

	for (unsigned i = FromA; i <= ToA; ++i)
		for (unsigned j = FromB; j <= ToB; ++j)
			SimMx[i+1][j+1] = LOG_ZERO;
	}

bool IsGlobalHitPair(unsigned Starti1, unsigned Endi1, unsigned Startj1, unsigned Endj1,
  unsigned Starti2, unsigned Endi2, unsigned Startj2, unsigned Endj2)
	{
	unsigned k = opt_arrangeov;
	if (Endi1 <= Starti2 + k && Endj1 <= Startj2 + k)
		return true;

	if (Endi2 <= Starti1 + k && Endj2 <= Startj1 + k)
		return true;

	return false;
	}

void IterateLocalFB(const string &Description, vector<string> &Paths,
  vector<unsigned> &Startis, vector<unsigned> &Startjs, vector<float> &Scores)
	{
	Paths.clear();
	Startis.clear();
	Startjs.clear();

	Mx<float> &Simf = GetSimMxf();
	SeqDB &DB = *Simf.m_SeqDB;
	unsigned SeqIndexA = Simf.m_IdA;
	unsigned SeqIndexB = Simf.m_IdB;

	string Model;
	GetLocalModel(DB, Model);
	FWD_BWD FB = SetModel(Model);

	const float t = opt_minlocalprob;
	const float e = -opt_gaplocal;
	for (unsigned Try = 0; Try < 8; ++Try)
		{
		Mx<float> PPMx;
		FB(PPMx);
		if (Try == 0 && opt_posteriors)
			WriteMx(Description, PPMx);

		vector<unsigned> Startisx;
		vector<unsigned> Startjsx;
		vector<string> Pathsx;
		vector<float> Scoresx;
		MultiSW(DB, SeqIndexA, SeqIndexB, PPMx, t, e, Startisx, Startjsx, Pathsx, Scoresx);

		const unsigned PathCount = SIZE(Pathsx);
		if (PathCount == 0)
			break;

		for (unsigned PathIndex = 0; PathIndex < PathCount; ++PathIndex)
			{
			const string &Path = Pathsx[PathIndex];
			const unsigned Starti = Startisx[PathIndex];
			const unsigned Startj = Startjsx[PathIndex];
			float Score = Scoresx[PathIndex];

			unsigned Ni;
			unsigned Nj;
			GetLetterCounts(Path, Ni, Nj);

			unsigned Endi = Starti + Ni - 1;
			unsigned Endj = Startj + Nj - 1;
			
			MaskSimMx(Starti, Endi, Startj, Endj);

			Paths.push_back(Path);
			Startis.push_back(Starti);
			Startjs.push_back(Startj);
			Scores.push_back(Score);
			}
		}
	}
