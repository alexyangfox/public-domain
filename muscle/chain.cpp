#include "myutils.h"
#include "seqdb.h"
#include "mx.h"
#include "params.h"
#include "hit.h"
#include <algorithm>

void SetSimMx(SeqDB &DB, unsigned IdA, unsigned IdB);
void IterateLocalFB(const string &Description, vector<string> &Paths,
  vector<unsigned> &Startis, vector<unsigned> &Startjs, vector<float> &Scores);
void LogLocalMatchCartoon(unsigned LA, unsigned LB, unsigned StartA,
  unsigned EndA, unsigned StartB, unsigned EndB, bool Self);

void ComputeInvertsPair(SeqDB &DB, unsigned InputSeqIndex1, unsigned InputSeqIndex2,
  vector<string> &Paths, vector<unsigned> &Startis, vector<unsigned> &Startjs,
  vector<float> &Scores);
void GetLetterCounts(const string &Path, unsigned &i, unsigned &j);
void LogLocalAln(const byte *A, const byte *B, unsigned LoA, unsigned LoB,
  unsigned LoOffsetA, unsigned LoOffsetB, const string &Path, bool Inverted,
  bool Reversed, bool Nucleo);
void LogLocalAlnDB(SeqDB &DB, unsigned IdA, unsigned IdB, unsigned LoOffsetA,
  unsigned LoOffsetB, const string &Path, bool Inverted,
  bool Reversed, bool Nucleo);
void SuperMap(const vector<HitData> &Hits,
  vector<unsigned> &ChainA, vector<unsigned> &ChainB);
bool HasOverlap(const HitData &H1, const HitData &H2);
void Range(vector<unsigned> &v, unsigned N);
void Chop(const vector<HitData> &Hits, vector<HitData> &Hits2);
void AlignPairLocal(SeqDB &DB, unsigned IdA, unsigned IdB, vector<HitData> &Hits);

static vector<HitData> *g_Hits;
static unsigned g_SeqIndex;
static bool LT(unsigned i, unsigned j)
	{
	const HitData &Hi = (*g_Hits)[i];
	const HitData &Hj = (*g_Hits)[j];
	return Hi.GetLo(g_SeqIndex) < Hj.GetLo(g_SeqIndex);
	}

static void GetConnectedComponents(const vector<HitData> &Hits,
  vector<vector<unsigned> > &CCs)
	{
	CCs.clear();
	const unsigned N = SIZE(Hits);
	vector<bool> Done(N, false);

	for (unsigned i = 0; i < N; ++i)
		{
		if (Done[i])
			continue;
		const HitData &Hi = Hits[i];

		vector<unsigned> v;
		v.push_back(i);
		Done[i] = true;

		for (unsigned j = 0; j < N; ++j)
			{
			if (Done[j])
				continue;
			if (HasOverlap(Hi, Hits[j]))
				{
				v.push_back(j);
				Done[j] = true;
				}
			}
		CCs.push_back(v);
		}
	}

static bool Between(const HitData &H1, const HitData &H2, const HitData &H3)
	{
	if (H2.LoA > H1.LoA && H2.LoA < H3.LoA)
		return true;
	if (H2.LoB > H1.LoB && H2.LoB < H3.LoB)
		return true;
	return false;
	}

static void Merge(HitData &H, const HitData &H2)
	{
	Log("Merge ");
	H.LogMe();
	Log(" and ");
	H2.LogMe();
	H.LoA = min(H.LoA, H2.LoA);
	H.HiA = max(H.HiA, H2.HiA);
	H.LoB = min(H.LoB, H2.LoB);
	H.HiB = max(H.HiB, H2.HiB);
	H.Score += H2.Score;
	Log(" result: ");
	H.LogMe();
	Log("\n");
	}

static bool CoLinear(const HitData &H1, const HitData &H2)
	{
	return H1.Strand == H2.Strand && H1.HiA < H2.LoA && H1.HiB < H2.LoB;
	}

static void MergeHits(const vector<HitData> &Hits, vector<HitData> &MergedHits)
	{
	MergedHits = Hits;
	for (;;)
		{
		bool Merged = false;
		const unsigned N = SIZE(MergedHits);
		for (unsigned i = 0; i < N; ++i)
			{
			HitData &Hi = MergedHits[i];
			for (unsigned j = i + 1; j < N; ++j)
				{
				const HitData &Hj = MergedHits[j];
				if (!CoLinear(Hi, Hj) && !CoLinear(Hj, Hi))
					continue;
				bool CanMerge = true;
				for (unsigned k = 0; k < N; ++k)
					{
					if (k == i || k == j)
						continue;
					const HitData &Hk = MergedHits[k];
					if (Between(Hi, Hk, Hj))
						{
						CanMerge = false;
						break;
						}
					}
				if (CanMerge)
					{
					Merged = true;
					Merge(MergedHits[i], MergedHits[j]);
					vector<HitData>::iterator pj = MergedHits.begin() + j;
					MergedHits.erase(pj);
					goto Nexti;
					}
				}
			}
	Nexti:
		if (!Merged)
			return;
		}
	}

static void AppendSegsToDB(const SeqDB &DB, unsigned Id, const vector<SegData> &Segs,
  SeqDB &SegsDB)
	{
	const byte *Seq = DB.GetSeq(Id);
	const string &Label = DB.GetLabel(Id);

	const unsigned N = SIZE(Segs);
	for (unsigned i = 0; i < N; ++i)
		{
		const SegData &Seg = Segs[i];

		//char Tmp[16];
		//sprintf(Tmp, ":%u-%u%c", Seg.Lo, Seg.Hi, pom(Seg.Strand));
		//string Label2 = Label + Tmp;
		unsigned SegId = SegsDB.AppendSeq(Label, Seq + Seg.Lo, Seg.GetLength());
		SegsDB.m_Los[SegId] += Seg.Lo;
		if (!Seg.Strand)
			SegsDB.m_Strands[SegId] = !SegsDB.m_Strands[SegId];

		if (!Seg.Strand)
			SegsDB.RevComp(SegId);
		}
	}

static unsigned Overlap(const SegData &Seg1, const SegData &Seg2)
	{
	unsigned MaxLo = max(Seg1.Lo, Seg2.Lo);
	unsigned MinHi = min(Seg1.Hi, Seg2.Hi);
	if (MaxLo > MinHi)
		return 0;
	return MinHi - MaxLo + 1;
	}

static void MergeSegPair(const SegData &Seg1, const SegData &Seg2,
  SegData &MergedSeg)
	{
	asserta(Seg1.Strand == Seg2.Strand);
	MergedSeg.Lo = min(Seg1.Lo, Seg2.Lo);
	MergedSeg.Hi = max(Seg1.Hi, Seg2.Hi);
	MergedSeg.Strand = Seg1.Strand;
	}

static void MergeSegs(const vector<SegData> &Segs,
  vector<SegData> &MergedSegs)
	{
	vector<SegData> TmpSegs(Segs);
	for (;;)
		{
		vector<SegData> TmpSegs2(TmpSegs);
		bool Any = false;
		const unsigned N = SIZE(TmpSegs2);
		for (unsigned i = 0; i < N; ++i)
			{
			const SegData &Segi = TmpSegs2[i];
			for (unsigned j = i+1; j < N; ++j)
				{
				const SegData &Segj = TmpSegs2[j];
				if (Segi.Strand == Segj.Strand && Overlap(Segi, Segj) > 0)
					{
					Any = true;
					SegData MergedSeg;
					MergeSegPair(Segi, Segj, MergedSeg);

					TmpSegs.clear();
					for (unsigned k = 0; k < N; ++k)
						{
						if (k == i || k == j)
							continue;
						TmpSegs.push_back(TmpSegs2[k]);
						}
					TmpSegs.push_back(MergedSeg);
					TmpSegs2 = TmpSegs;
					goto Next;
					}
				}
			}
	Next:
		if (!Any)
			{
			MergedSegs = TmpSegs2;
			return;
			}
		}
	}

void Chain(SeqDB &DB, unsigned IdA, unsigned IdB)
	{
	//string Model;
	//GetLocalModel(DB, Model);
	//SetModel(Model);

	//const unsigned LA = DB.GetSeqLength(IdA);
	//const unsigned LB = DB.GetSeqLength(IdB);
	//SetSimMx(DB, IdA, IdB);

	vector<HitData> Hits;
	AlignPairLocal(DB, IdA, IdB, Hits);
	return;

	//IterateLocalFB("Chain", Paths, Startis, Startjs, Scores);
	//for (unsigned i = 0; i < SIZE(Paths); ++i)
	//	LogLocalAlnDB(DB, IdA, IdB, Startis[i], Startjs[i], Paths[i],
	//	  false, false, DB.IsNucleo());

	//vector<string> IPaths;
	//vector<unsigned> IStartis;
	//vector<unsigned> IStartjs;
	//vector<float> IScores;
	//if (DB.IsNucleo())
	//	ComputeInvertsPair(DB, IdA, IdB, IPaths, IStartis, IStartjs, IScores);

	//Log("\n");
	//Log("Start1    End1  Start2    End2       Score  Inv\n");
	//Log("------  ------  ------  ------  ----------  ---\n");
	//for (unsigned i = 0; i < SIZE(Paths); ++i)
	//	{
	//	unsigned Ni;
	//	unsigned Nj;
	//	GetLetterCounts(Paths[i], Ni, Nj);
	//	unsigned Endi = Startis[i] + Ni - 1;
	//	unsigned Endj = Startjs[i] + Nj - 1;
	//	Log("%6u  %6u  %6u  %6u  %10.1f    N\n",
	//	  Startis[i], Endi, Startjs[i], Endj, Scores[i]);
	//	}
	//for (unsigned i = 0; i < SIZE(IPaths); ++i)
	//	{
	//	unsigned Ni;
	//	unsigned Nj;
	//	GetLetterCounts(IPaths[i], Ni, Nj);
	//	unsigned Endi = IStartis[i] + Ni - 1;
	//	unsigned Endj = IStartjs[i] + Nj - 1;
	//	Log("%6u  %6u  %6u  %6u  %10.1f    Y\n",
	//	  IStartis[i], Endi, IStartjs[i], Endj, IScores[i]);
	//	}

	//vector<HitData> Hits;
	//for (unsigned i = 0; i < SIZE(Paths); ++i)
	//	{
	//	unsigned Ni;
	//	unsigned Nj;
	//	GetLetterCounts(Paths[i], Ni, Nj);
	//	unsigned Starti = Startis[i];
	//	unsigned Startj = Startjs[i];
	//	unsigned Endi = Starti + Ni - 1;
	//	unsigned Endj = Startj + Nj - 1;

	//	HitData H;
	//	H.User = SIZE(Hits);
	//	H.LoA = Starti;
	//	H.LoB = Startj;
	//	H.HiA = Endi;
	//	H.HiB = Endj;
	//	H.Strand = true;
	//	H.Score = Scores[i];
	//	H.Path = Paths[i];
	//	Hits.push_back(H);
	//	}

	//for (unsigned i = 0; i < SIZE(IPaths); ++i)
	//	{
	//	unsigned Ni;
	//	unsigned Nj;
	//	GetLetterCounts(IPaths[i], Ni, Nj);

	//	unsigned Starti = IStartis[i];
	//	unsigned Startj = IStartjs[i];
	//	unsigned Endi = Starti + Ni - 1;
	//	unsigned Endj = Startj + Nj - 1;

	//	HitData H;
	//	H.User = SIZE(Hits);
	//	H.LoA = Starti;
	//	H.LoB = Startj;
	//	H.HiA = Endi;
	//	H.HiB = Endj;
	//	H.Strand = false;
	//	H.Path = IPaths[i];
	//	H.Score = IScores[i];
	//	Hits.push_back(H);
	//	}
	//Log("\n");
	//Log("Hits:\n");
	//LogHits(Hits);
	//DotPlotHits(Hits, LA, LB);

	//vector<HitData> Hits2;
	//Chop(Hits, Hits2);
	//Hits = Hits2;

	//for (unsigned i = 0; i < SIZE(Hits); ++i)
	//	Hits[i].User = i;
	//const unsigned HitCount = SIZE(Hits);

	//Log("\n");
	//Log("Chopped hits:\n");
	//LogHits(Hits);

	//vector<unsigned> ChainA;
	//vector<unsigned> ChainB;
	//SuperMap(Hits, ChainA, ChainB);

	//vector<HitData> HitsA;
	//vector<HitData> HitsB;
	//vector<HitData> HitsAB;

	//DotPlotHits(Hits, LA, LB);

	//vector<bool> Done(HitCount);

	//const unsigned CountA = SIZE(ChainA);
	//const unsigned CountB = SIZE(ChainB);

	//g_Hits = &Hits;
	//g_SeqIndex = 0;
	//sort(ChainA.begin(), ChainA.end(), LT);

	//g_SeqIndex = 1;
	//sort(ChainB.begin(), ChainB.end(), LT);

	//for (unsigned i = 0; i < CountA; ++i)
	//	{
	//	unsigned HitIndex = ChainA[i];
	//	HitsA.push_back(Hits[HitIndex]);
	//	HitsAB.push_back(Hits[HitIndex]);
	//	Done[HitIndex] = true;
	//	}

	//for (unsigned i = 0; i < CountB; ++i)
	//	{
	//	unsigned HitIndex = ChainB[i];
	//	HitsB.push_back(Hits[HitIndex]);
	//	if (!Done[HitIndex])
	//		HitsAB.push_back(Hits[HitIndex]);
	//	}

	//Log("\n");
	//Log("HitsA:\n");
	//LogHits(HitsA);

	//Log("\n");
	//Log("HitsB:\n");
	//LogHits(HitsB);

	//Log("\n");
	//Log("HitsAB:\n");
	//LogHits(HitsAB);

	//DotPlotHits(HitsAB, LA, LB);

	//vector<HitData> MergedHits;
	//MergeHits(HitsAB, MergedHits);
	//Log("\n");
	//Log("Merged hits:\n");
	//LogHits(MergedHits);

	//HitsAB = MergedHits;

	//vector<vector<unsigned> > CCs;
	//GetConnectedComponents(HitsAB, CCs);
	//const unsigned CCCount = SIZE(CCs);

	//Log("\n");
	//Log("Connected components:\n");
	//Log("    CC    User     LoA     HiA     LoB     HiB    LenA    LenB  +    Score\n");
	//Log("------  ------  ------  ------  ------  ------  ------  ------  -  -------\n");
	//for (unsigned CCIndex = 0; CCIndex < CCCount; ++CCIndex)
	//	{
	//	const vector<unsigned> &CC = CCs[CCIndex];
	//	for (unsigned i = 0; i < SIZE(CC); ++i)
	//		{
	//		unsigned HitIndex = CC[i];
	//		const HitData &H = HitsAB[HitIndex];
	//		unsigned LenA = H.HiA - H.LoA + 1;
	//		unsigned LenB = H.HiB - H.LoB + 1;
	//		Log("%6u  %6u  %6u  %6u  %6u  %6u  %6u  %6u  %c  %7.1f\n",
	//		  CCIndex, H.User, H.LoA, H.HiA, H.LoB, H.HiB, LenA, LenB, pom(H.Strand), H.Score);
	//		}
	//	}

	//vector<vector<SegData> > SegsVecA;
	//vector<vector<SegData> > SegsVecB;

	//for (unsigned CCIndex = 0; CCIndex < CCCount; ++CCIndex)
	//	{
	//	vector<SegData> SegsA;
	//	vector<SegData> SegsB;
	//	const vector<unsigned> &CC = CCs[CCIndex];
	//	for (unsigned i = 0; i < SIZE(CC); ++i)
	//		{
	//		unsigned HitIndex = CC[i];
	//		const HitData &H = HitsAB[HitIndex];

	//		SegData SegA;
	//		SegData SegB;
	//		H.GetSegA(SegA);
	//		H.GetSegB(SegB);

	//		SegsA.push_back(SegA);
	//		SegsB.push_back(SegB);
	//		}

	//	vector<SegData> SegsA2;
	//	vector<SegData> SegsB2;
	//	MergeSegs(SegsA, SegsA2);
	//	MergeSegs(SegsB, SegsB2);

	//	SegsA = SegsA2;
	//	SegsB = SegsB2;

	//	SeqDB SegsDB;
	//	AppendSegsToDB(DB, IdA, SegsA, SegsDB);
	//	AppendSegsToDB(DB, IdB, SegsB, SegsDB);

	//	for (unsigned i = 0; i < SegsDB.GetSeqCount(); ++i)
	//		SegsDB.SetUser(i, i);

	//	SeqDB &msa = SegsDB.Align(0, 0, false, "");
	//	Log("\n");
	//	Log("CC %u\n", CCIndex);
	//	msa.LogMe(&SegsDB);

	//	SegsVecA.push_back(SegsA);
	//	SegsVecB.push_back(SegsB);
	//	}
	}

static bool OnNode(const Tree &t, unsigned NodeIndex, void *UserData)
	{
	if (t.IsLeaf(NodeIndex))
		return true;

	SeqDB &DB = *(SeqDB *) UserData;

	unsigned Left = t.GetLeft(NodeIndex);
	unsigned Right = t.GetRight(NodeIndex);
	if (t.IsLeaf(Left) && t.IsLeaf(Right))
		{
		ProgressStep(NodeIndex, UINT_MAX, "Chain");
		unsigned IdA = t.GetUser(Left);
		unsigned IdB = t.GetUser(Right);
		Chain(DB, IdA, IdB);
		return true;
		}

	return true;
	}

void AlignMAF(SeqDB &DB)
	{
	unsigned SeqCount = DB.GetSeqCount();

	if (SeqCount == 2)
		{
		Chain(DB, 0, 1);
		return;
		}

	if (opt_tree == "")
		Die("No tree specified");

	DB.m_GuideTree.FromFile(opt_tree);
	DB.BindTree(DB.m_GuideTree);
	DB.m_GuideTree.LogMe();
	ProgressStep(0, UINT_MAX, "Chain");
	DB.m_GuideTree.Traverse(OnNode, &DB);
	ProgressStep(UINT_MAX, UINT_MAX, "Chain");
	}
