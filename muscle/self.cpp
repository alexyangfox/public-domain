#include "myutils.h"
#include "mx.h"
#include "sparsemx.h"
#include "seqdb.h"
#include "params.h"
#include "isgap.h"
#include "info.h"
#include "hit.h"

#define TRACE	0

void GetLetterCounts(const string &Path, unsigned &i, unsigned &j);
void FindRepeats(SeqDB &DB, unsigned InputSeqIndex, unsigned Starti, unsigned Startj,
  unsigned &RepeatLength, float &RepeatCount, float &RepeatPctId,
  const string &Path);
void IterateLocalFB(const string &Description, vector<string> &Paths,
  vector<unsigned> &Startis, vector<unsigned> &Startjs, vector<float> &Scores);
void MaskSimMxSelf();
void AlignSeqPairLocal(SeqDB &DB, unsigned IdA, unsigned IdB, vector<HitData> &Hits);
void LogLocalAlnHit(SeqDB &DB, unsigned IdA, unsigned IdB, const HitData &Hit);
void RevComp(const byte *Seq, byte *RevCompSeq, unsigned L);

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

static float GetPctId(const SeqDB &DB, unsigned IdA, unsigned IdB, const HitData &Hit)
	{
	const byte *SeqA = DB.GetSeq(IdA);
	const byte *SeqB = DB.GetSeq(IdB);
	if (Hit.Strand)
		return GetPctId(SeqA + Hit.LoA, SeqB + Hit.LoB, Hit.Path);

	unsigned NB = Hit.GetLengthB();
	byte *BRC = myalloc<byte>(NB);
	RevComp(SeqB + Hit.LoB, BRC, NB);
	float PctId = GetPctId(SeqA + Hit.LoA, BRC, Hit.Path);
	myfree(BRC);
	return PctId;
	}

void OutputSelfHits(SeqDB &DB, unsigned Id, const vector<HitData> &Hits)
	{
	const unsigned HitCount = SIZE(Hits);
	for (unsigned HitIndex = 0; HitIndex < HitCount; ++HitIndex)
		{
		const HitData &Hit = Hits[HitIndex];

		const string &Path = Hit.Path;
		unsigned Starti = Hit.LoA;
		unsigned Startj = Hit.LoB;
		unsigned Endi = Hit.HiA;
		unsigned Endj = Hit.HiB;

	// Special case -- palindromes are found twice, discard one
		bool Discard = false;
		if (Starti > Startj && !Hit.Strand)
			{
			for (unsigned HitIndex2 = 0; HitIndex2 < HitCount; ++HitIndex2)
				{
				if (HitIndex2 == HitIndex)
					continue;
				const HitData &Hit2 = Hits[HitIndex2];
				if (Hit.LoA == Hit2.LoB && Hit.HiA == Hit2.HiB &&
				  Hit.LoB == Hit2.LoA && Hit.HiB == Hit2.HiA)
					{
					Discard = true;
					break;
					}
				}
			}
		if (Discard)
			continue;

		// LogSelfHit(DB, Id, Starti, Startj, Path);
		LogLocalAlnHit(DB, Id, Id, Hit);

		if (Overlap(Starti, Endi, Startj, Endj) > 8)
			{
			if (Hit.Strand)
				{
				unsigned RepeatLength;
				float RepeatCount;
				float RepeatPctId;
				FindRepeats(DB, Id, Starti, Startj, RepeatLength, RepeatCount,
				  RepeatPctId, Path);

				RepeatInfo RI;
				RI.InputSeqIndex = Id;
				RI.Label = DB.GetLabel(Id);
				RI.Start = Starti;
				RI.End = Endj;
				RI.Length = RepeatLength;
				RI.Count = RepeatCount;
				RI.AvgPctId = RepeatPctId;

				g_Repeats.push_back(RI);
				}
			}
		else
			{
			//float PctId = GetPctId(Seq+Starti, Seq+Startj, Path);
			float PctId = GetPctId(DB, Id, Id, Hit);
			DupeInfo DI;
			DI.InputSeqIndex = Id;
			DI.Label = DB.GetLabel(Id);
			DI.Start1 = Starti;
			DI.End1 = Endi;
			DI.Start2 = Startj;
			DI.End2 = Endj;
			DI.PctId = PctId;

			g_Dupes.push_back(DI);
			}
		}
	}

void ComputeSelfHitsDB(SeqDB &DB)
	{
	const unsigned SeqCount = DB.GetSeqCount();
	for (unsigned Id = 0; Id < SeqCount; ++Id)
		{
		string ShortLabel;
		DB.GetShortLabel(Id, ShortLabel);

		ProgressStep(Id, SeqCount, "Self-align '%.16s'", ShortLabel.c_str());

#if 0
			{
			string Model;
			GetLocalModel(DB, Model);
			SetModel(Model);

			SetSimMx(DB, Id, Id);

			vector<string> Paths;
			vector<unsigned> Startis;
			vector<unsigned> Startjs;
			vector<float> Scores;
			IterateLocalFB("Self", Paths, Startis, Startjs, Scores);
			DeleteSubAlns(Paths, Startis, Startjs);
			//OutputSelfAlns(DB, Id, Paths, Startis, Startjs);
			vector<HitData> Hits;
			for (unsigned i = 0; i < SIZE(Paths); ++i)
				{
				unsigned Ni;
				unsigned Nj;
				GetLetterCounts(Paths[i], Ni, Nj);

				HitData Hit;
				Hit.LoA = Startis[i];
				Hit.LoB = Startjs[i];
				Hit.HiA = Hit.LoA + Ni - 1;
				Hit.HiB = Hit.LoB + Nj - 1;
				Hit.Score = Scores[i];
				Hit.Path = Paths[i];

				Hits.push_back(Hit);
				}
			OutputSelfHits(DB, Id, Hits);
			Log("%s, %u self-alignments found\n", ShortLabel.c_str(), SIZE(Paths));
			}
		else
#endif
			{
			vector<HitData> Hits;
			AlignSeqPairLocal(DB, Id, Id, Hits);
			OutputSelfHits(DB, Id, Hits);
			}
		}
	}
