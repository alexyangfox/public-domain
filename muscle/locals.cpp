#include "myutils.h"
#include "mx.h"
#include "sparsemx.h"
#include "seqdb.h"
#include "params.h"
#include "isgap.h"
#include "info.h"

#define TRACE	0

void GetLetterCounts(const string &Path, unsigned &i, unsigned &j);
void FindRepeats(SeqDB &DB, unsigned InputSeqIndex, unsigned Starti, unsigned Startj,
  unsigned &RepeatLength, float &RepeatCount, float &RepeatPctId,
  const string &Path);
void IterateLocalFB(const string &Description, vector<string> &Paths,
  vector<unsigned> &Startis, vector<unsigned> &Startjs, vector<float> &Scores);
void MaskSimMxSelf();
void LogSelfAln(SeqDB &DB, unsigned InputSeqIndex, unsigned Starti, unsigned Startj,
  const string &Path);

extern vector<SparseMx> G_MatchPosteriors;

static vector<RepeatInfo> g_Repeats;
static vector<DupeInfo> g_Dupes;

const vector<RepeatInfo> &GetRepeatInfos()
	{
	return g_Repeats;
	}

const vector<DupeInfo> &GetDupeInfos()
	{
	return g_Dupes;
	}

void LogSelfReport()
	{
	Log("\n");
	if (g_Dupes.empty())
		Log("No duplications found.\n");
	else
		{
		Log("Duplications:\n");
		Log(" Start1     End1   Start2     End2  Length1  Length2    PctId  Label\n");
		Log("-------  -------  -------  -------  -------  -------  -------  -----\n");
		for (unsigned i = 0; i < SIZE(g_Dupes); ++i)
			{
			const DupeInfo &DI = g_Dupes[i];
			unsigned Length1 = DI.End1 - DI.Start1 + 1;
			unsigned Length2 = DI.End2 - DI.Start2 + 1;
			const string &Label = DI.Label.c_str();
			Log("%7u  %7u  %7u  %7u  %7u  %7u  %7.1f  %s\n",
			  DI.Start1+1, DI.End1+1, DI.Start2+1, DI.End2+1, Length1, Length2,
			  DI.PctId, Label.c_str());
			}
		}

	Log("\n");
	if (g_Repeats.empty())
		Log("No repeats found.\n");
	else
		{
		Log("Repeats:\n");
		Log("  Start      End   Length   RptLen   NrRpts    PctId  Label\n");
		Log("-------  -------  -------  -------  -------  -------  -----\n");
		for (unsigned i = 0; i < SIZE(g_Repeats); ++i)
			{
			const RepeatInfo &RI = g_Repeats[i];

			unsigned Length = RI.End - RI.Start + 1;
			const string &Label = RI.Label.c_str();
			Log("%7u  %7u  %7u  %7u  %7.1f  %7.1f  %s\n",
			  RI.Start+1, RI.End+1, Length, RI.Length, RI.Count,
			  RI.AvgPctId, Label.c_str());
			}
		}
	}

unsigned Overlap(unsigned StartA, unsigned EndA, unsigned StartB, unsigned EndB)
	{
	unsigned MaxStart = max(StartA, StartB);
	unsigned MinEnd = min(EndA, EndB);
	if (MaxStart > MinEnd)
		return 0;
	return MinEnd - MaxStart + 1;
	}

static bool Is1SubAlnOf2(unsigned Starti1, unsigned Endi1, unsigned Startj1, unsigned Endj1,
  unsigned Starti2, unsigned Endi2, unsigned Startj2, unsigned Endj2)
	{
	return Starti1 >= Starti2 && Endi1 <= Endi2
	  && Startj1 >= Startj2 && Endj1 <= Endj2;
	}

void DeleteSubAlns(vector<string> &Paths, vector<unsigned> &Startis,
  vector<unsigned> &Startjs)
	{
	vector<unsigned> NewStartis;
	vector<unsigned> NewStartjs;
	vector<string> NewPaths;
	const unsigned N = SIZE(Paths);
	vector<unsigned> Endis;
	vector<unsigned> Endjs;
	for (unsigned i = 0; i < N; ++i)
		{
		unsigned Starti = Startis[i];
		unsigned Startj = Startjs[i];
		const string &Path = Paths[i];

		unsigned Ni;
		unsigned Nj;
		GetLetterCounts(Path, Ni, Nj);

		unsigned Endi = Starti + Ni - 1;
		unsigned Endj = Startj + Nj - 1;

		Endis.push_back(Endi);
		Endjs.push_back(Endj);
		}

	for (unsigned i1 = 0; i1 < N; ++i1)
		{
		unsigned Starti1 = Startis[i1];
		unsigned Startj1 = Startjs[i1];

		unsigned Endi1 = Endis[i1];
		unsigned Endj1 = Endjs[i1];
		for (unsigned i2 = 0; i2 < N; ++i2)
			{
			if (i2 == i1)
				continue;
			unsigned Starti2 = Startis[i2];
			unsigned Startj2 = Startjs[i2];

			unsigned Endi2 = Endis[i2];
			unsigned Endj2 = Endjs[i2];

			if (Is1SubAlnOf2(Starti1, Endi1, Startj1, Endj1,
			  Starti2, Endi2, Startj2, Endj2))
				{
#if	TRACE
				Log("Subaln discarded: (%u,%u)-(%u,%u) < (%u,%u)-(%u,%u)\n",
				  Starti1, Endi1, Startj1, Endj1,
				  Starti2, Endi2, Startj2, Endj2);
#endif
				goto NextAln;
				}
			}
		NewStartis.push_back(Starti1);
		NewStartjs.push_back(Startj1);
		NewPaths.push_back(Paths[i1]);
	NextAln:;
		}

	Startis = NewStartis;
	Startjs = NewStartjs;
	Paths = NewPaths;
	}

float GetPctId(const byte *A, const byte *B, const string &Path)
	{
	unsigned i = 0;
	unsigned j = 0;
	unsigned PairCount = 0;
	unsigned IdCount = 0;
	const unsigned ColCount = SIZE(Path);
	for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
		{
		switch (Path[ColIndex])
			{
		case 'M':
			{
			char a = A[i++];
			char b = B[j++];
			++PairCount;
			if (toupper(a) == toupper(b))
				++IdCount;
			break;
			}
		case 'D':
			++i;
			break;
		case 'I':
			++j;
			break;
		default:
			asserta(false);
			}
		}
	return PairCount == 0 ? 0.0f : float(IdCount)*100.0f/float(PairCount);
	}

void OutputSelfAlns(SeqDB &DB, unsigned InputSeqIndex,
  const vector<string> &Paths,
  const vector<unsigned> &Startis,
  const vector<unsigned> &Startjs)
	{
	const byte *Seq = DB.GetSeq(InputSeqIndex);

	const unsigned AlnCount = SIZE(Paths);
	for (unsigned AlnIndex = 0; AlnIndex < AlnCount; ++AlnIndex)
		{
		const string &Path = Paths[AlnIndex];
		unsigned Starti = Startis[AlnIndex];
		unsigned Startj = Startjs[AlnIndex];
		LogSelfAln(DB, InputSeqIndex, Starti, Startj, Path);

		unsigned Ni;
		unsigned Nj;
		GetLetterCounts(Path, Ni, Nj);
		unsigned Endi = Starti + Ni - 1;
		unsigned Endj = Startj + Nj - 1;
		if (Overlap(Starti, Endi, Startj, Endj) > 8)
			{
			unsigned RepeatLength;
			float RepeatCount;
			float RepeatPctId;
			FindRepeats(DB, InputSeqIndex, Starti, Startj, RepeatLength, RepeatCount,
			  RepeatPctId, Path);

			RepeatInfo RI;
			RI.InputSeqIndex = InputSeqIndex;
			RI.Label = DB.GetLabel(InputSeqIndex);
			RI.Start = Starti;
			RI.End = Endj;
			RI.Length = RepeatLength;
			RI.Count = RepeatCount;
			RI.AvgPctId = RepeatPctId;

			g_Repeats.push_back(RI);
			}
		else
			{
			float PctId = GetPctId(Seq+Starti, Seq+Startj, Path);
			DupeInfo DI;
			DI.InputSeqIndex = InputSeqIndex;
			DI.Label = DB.GetLabel(InputSeqIndex);
			DI.Start1 = Starti;
			DI.End1 = Endi;
			DI.Start2 = Startj;
			DI.End2 = Endj;
			DI.PctId = PctId;

			g_Dupes.push_back(DI);
			}
		}
	}

void ComputeSelfAlns(SeqDB &DB, vector<vector<string> > &PathVec,
  vector<vector<unsigned> > &StartiVec, vector<vector<unsigned> > &StartjVec,
  vector<vector<float> > &ScoresVec)
	{
	PathVec.clear();
	StartiVec.clear();
	StartjVec.clear();

	const unsigned SeqCount = DB.GetSeqCount();
	PathVec.resize(SeqCount);
	StartiVec.resize(SeqCount);
	StartjVec.resize(SeqCount);
	ScoresVec.resize(SeqCount);

	string Model;
	GetLocalModel(DB, Model);
	SetModel(Model);

	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{
		unsigned InputSeqIndex = SeqIndex;//@@
		const string &Label = DB.GetLabel(InputSeqIndex);
		ProgressStep(SeqIndex, SeqCount, "Self-align %.16s", Label.c_str());

		vector<unsigned> &Startis = StartiVec[InputSeqIndex];
		vector<unsigned> &Startjs = StartjVec[InputSeqIndex];
		vector<string> &Paths = PathVec[InputSeqIndex];
		vector<float> &Scores = ScoresVec[InputSeqIndex];

		SetSimMx(DB, InputSeqIndex, InputSeqIndex);
		MaskSimMxSelf();
		vector<string> Paths2;
		vector<unsigned> Startis2;
		vector<unsigned> Startjs2;
		vector<float> Scores2;
		IterateLocalFB("Self", Paths2, Startis2, Startjs2, Scores2);

		DeleteSubAlns(Paths2, Startis2, Startjs2);

		OutputSelfAlns(DB, InputSeqIndex, Paths2, Startis2, Startjs2);

		Paths.insert(Paths.end(), Paths2.begin(), Paths2.end());
		Startis.insert(Startis.end(), Startis2.begin(), Startis2.end());
		Startjs.insert(Startjs.end(), Startjs2.begin(), Startjs2.end());
		Scores.insert(Scores.end(), Scores2.begin(), Scores2.end());

		string ShortLabel;
		DB.GetShortLabel(SeqIndex, ShortLabel);
		Log("%s, %u self-alignments found\n", Label.c_str(), SIZE(Paths2));
		}
	}
