#include "myutils.h"
#include "seqdb.h"
#include "params.h"
#include "isgap.h"

void MaskSimMxSelf();
void DeleteSubAlns(vector<string> &Paths, vector<unsigned> &Startis,
  vector<unsigned> &Startjs);
void WriteMx(const string &Name, Mx<float> &Mxf);
void MultiSW(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &PPMx,
  float t, float e, vector<unsigned> &Startis, vector<unsigned> &Startjs,
  vector<string> &Paths);
void GetLetterCounts(const string &Path, unsigned &i, unsigned &j);
void MaskSimMx(unsigned FromA, unsigned ToA, unsigned FromB, unsigned ToB);
void OutputSelfAlns(SeqDB &DB, unsigned InputSeqIndex,
  const vector<string> &Paths,
  const vector<unsigned> &Startis,
  const vector<unsigned> &Startjs);
void IterateLocalFB(const string &Description, vector<string> &Paths,
  vector<unsigned> &Startis, vector<unsigned> &Startjs, vector<float> &Scores);
void LogLocalAlnAln(SeqDB &DBA, SeqDB &DBB, unsigned LoA, unsigned LoB,
  const string &Path, bool Inverted, bool Nucleo);
void LogSelfMatchCartoon(unsigned L, unsigned StartA, unsigned EndA,
  unsigned StartB, unsigned EndB);

static float SetSimMxColPair(SeqDB &DB1, unsigned ColIndex1,
   SeqDB &DB2, unsigned ColIndex2)
	{
	assert(ColIndex1 < DB1.GetColCount() && ColIndex2 < DB2.GetColCount());

	float **SubstMx = GetSubstMx();
	const unsigned SeqCount1 = DB1.GetSeqCount();
	const unsigned SeqCount2 = DB2.GetSeqCount();
	float Sum = 0;
	for (unsigned SeqIndex1 = 0; SeqIndex1 < SeqCount1; ++SeqIndex1)
		{
		byte a = DB1.Get(SeqIndex1, ColIndex1);
		if (isgap(a))
			continue;
		for (unsigned SeqIndex2 = 0; SeqIndex2 < SeqCount2; ++SeqIndex2)
			{
			byte b = DB2.Get(SeqIndex2, ColIndex2);
			if (isgap(b))
				continue;
			Sum += SubstMx[a][b];
			}
		}
	return Sum/(SeqCount1*SeqCount2);
	}

void SetSimMxMSAs(SeqDB &DB1, SeqDB &DB2)
	{
	if (!DB1.m_Aligned || !DB2.m_Aligned)
		Die("SetSimMxMSAs: not aligned");

	const unsigned L1 = DB1.GetColCount();
	const unsigned L2 = DB2.GetColCount();

	Mx<float> &SimMxf = GetSimMxf();
	SimMxf.Alloc("SimAln", L1+1, L2+1, 0, 0, 0);
	float **Sim = SimMxf.GetData();

#if	DEBUG
	for (unsigned i = 0; i <= L1; ++i)
		Sim[i][0] = 0;
	for (unsigned j = 0; j <= L2; ++j)
		Sim[0][j] = 0;
#endif

	for (unsigned i = 0; i < L1; ++i)
		{
		float *SimRow = Sim[i+1];
		for (unsigned j = 0; j < L2; ++j)
			SimRow[j+1] = SetSimMxColPair(DB1, i, DB2, j);
		}
	}

void OutputSelfAlnsAln(SeqDB &DB,
  const vector<string> &Paths,
  const vector<unsigned> &Startis,
  const vector<unsigned> &Startjs)
	{
	const unsigned AlnCount = SIZE(Paths);
	for (unsigned AlnIndex = 0; AlnIndex < AlnCount; ++AlnIndex)
		{
		const string &Path = Paths[AlnIndex];
		unsigned Starti = Startis[AlnIndex];
		unsigned Startj = Startjs[AlnIndex];

		unsigned L = DB.GetColCount();
		unsigned Ni;
		unsigned Nj;
		GetLetterCounts(Path, Ni, Nj);
		unsigned Endi = Starti + Ni - 1;
		unsigned Endj = Startj + Nj - 1;
		Log("\n");
		Log("Self:\n");
		LogLocalAlnAln(DB, DB, Starti, Startj, Path, false, DB.IsNucleo());
		LogSelfMatchCartoon(L, Starti, Endi, Startj, Endj);
		}
	}

void SelfAln(SeqDB &DB)
	{
	string Model;
	GetLocalModel(DB, Model);
	SetModel(Model);

	SetSimMxMSAs(DB, DB);
	MaskSimMxSelf();

	vector<string> Paths;
	vector<unsigned> Startis;
	vector<unsigned> Startjs;
	vector<float> Scores;
	IterateLocalFB("Self", Paths, Startis, Startjs, Scores);
	DeleteSubAlns(Paths, Startis, Startjs);

	OutputSelfAlnsAln(DB, Paths, Startis, Startjs);
	}
