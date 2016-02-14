#include "myutils.h"
#include "hit.h"
#include "mx.h"
#include <algorithm>

unsigned Overlap(unsigned StartA, unsigned EndA, unsigned StartB, unsigned EndB);

void LogHits(const vector<HitData> &Hits)
	{
	Log("\n");
	Log("  User     LoA     HiA     LoB     HiB    LenA    LenB  +    Score\n");
	Log("------  ------  ------  ------  ------  ------  ------  -  -------\n");
	for (unsigned i = 0; i < SIZE(Hits); ++i)
		{
		const HitData &H = Hits[i];
		unsigned LenA = H.HiA - H.LoA + 1;
		unsigned LenB = H.HiB - H.LoB + 1;
		if (H.User == UINT_MAX)
			Log("%6.6s", "*");
		else
			Log("%6u", H.User);
		Log("  %6u  %6u  %6u  %6u  %6u  %6u  %c  %7.1f\n",
		  H.LoA, H.HiA, H.LoB, H.HiB, LenA, LenB, pom(H.Strand), H.Score);
		}
	}

void DotPlotHits(const vector<HitData> &Hits, unsigned LA, unsigned LB)
	{
	const unsigned HSize = 32;
	const unsigned VSize = 16;
	unsigned MaxL = max(LA, LB);
	float HBasesToPixels = float(HSize)/float(MaxL);
	float VBasesToPixels = float(VSize)/float(MaxL);
#define HPix(Bases)	unsigned(HBasesToPixels*(Bases))
#define VPix(Bases)	unsigned(VBasesToPixels*(Bases))
	unsigned Height = VPix(LA);
	unsigned Width = HPix(LB);

	Mx<char> Df;
	Df.Alloc("DotPlot", Height, Width);
	Df.Init(' ');
	char **D = Df.GetData();
	for (unsigned k = 0; k < SIZE(Hits); ++k)
		{
		const HitData &H = Hits[k];
		float g = float(H.HiB - H.LoB)/float(H.HiA - H.LoA);
		unsigned LoPixA = VPix(H.LoA);
		unsigned HiPixA = VPix(H.HiA);
		unsigned LoPixB = HPix(H.LoB);
		unsigned HiPixB = HPix(H.HiB);
		if (H.Strand)
			{
			for (unsigned i = LoPixA; i <= HiPixA; ++i)
				{
				unsigned j = LoPixB + unsigned(float(i-LoPixA)*g);
				if (i < Height && j < Width)
					D[i][j] = '\\';
				}
			}
		else
			{
			for (unsigned i = LoPixA; i <= HiPixA; ++i)
				{
				unsigned j = HiPixB - unsigned(float(i-LoPixA)*g);
				if (i < Height && j < Width)
					D[i][j] = '/';
				}
			}
		}
	Df.LogMe();

#undef Pix
	}

static void ChopHit(const HitData &Hit, unsigned ChopPos, unsigned SeqIndex,
  vector<HitData> &ChoppedHits)
	{
	ChoppedHits.clear();
	const string &Path = Hit.Path;
	unsigned PosA = Hit.LoA;
	unsigned PosB = (Hit.Strand ? Hit.LoB : Hit.HiB);
	unsigned ColCount = SIZE(Path);
	unsigned ColIndex = 0;
	for (;;)
		{
		if (ColIndex == ColCount)
			Die("ChopHit: pos not found");

		char c = Path[ColIndex++];
		if (SeqIndex == 0)
			{
			if (PosA == ChopPos)
				break;
			}
		else
			{
			if (PosB == ChopPos)
				break;
			}
		if (c == 'M' || c == 'D')
			++PosA;
		if (c == 'M' || c == 'I')
			{
			if (Hit.Strand)
				++PosB;
			else
				--PosB;
			}
		}

	HitData Hit1;
	HitData Hit2;

	Hit1.User = UINT_MAX;
	Hit2.User = UINT_MAX;

	Hit1.LoA = Hit.LoA;
	Hit1.HiA = PosA;

	Hit2.LoA = PosA + 1;
	Hit2.HiA = Hit.HiA;

	Hit1.Strand = Hit.Strand;
	Hit2.Strand = Hit.Strand;

	if (Hit.Strand)
		{
		Hit1.LoB = Hit.LoB;
		Hit1.HiB = PosB;

		Hit2.LoB = PosB + 1;
		Hit2.HiB = Hit.HiB;
		}
	else
		{
		Hit1.LoB = PosB;
		Hit1.HiB = Hit.HiB;

		Hit2.LoB = Hit.LoB;
		Hit2.HiB = PosB - 1;
		}

	unsigned L = Hit.GetAvgLength();
	unsigned L1 = Hit1.GetAvgLength();
	float f = float(L1)/float(L);
	Hit1.Score = f*Hit.Score;
	Hit2.Score = (1.0f - f)*Hit.Score;

	Hit1.Path.clear();
	Hit2.Path.clear();

	for (unsigned i = 0; i < ColIndex; ++i)
		Hit1.Path.push_back(Path[i]);
	for (unsigned i = ColIndex; i < ColCount; ++i)
		Hit2.Path.push_back(Path[i]);

	if (Hit1.LoA <= Hit1.HiA && Hit1.LoB <= Hit1.HiB)
		ChoppedHits.push_back(Hit1);
	if (Hit2.LoA <= Hit2.LoA && Hit2.LoB <= Hit2.LoB)
		ChoppedHits.push_back(Hit2);
	}

static void TrimHit(const HitData &Hit, HitData &TrimmedHit)
	{
	unsigned PrefixDCount = 0;
	unsigned PrefixICount = 0;
	unsigned SuffixDCount = 0;
	unsigned SuffixICount = 0;

	string TrimmedPath;
	bool FoundM = false;
	const string &Path = Hit.Path;
	unsigned LastMPos = UINT_MAX;
	for (unsigned i = 0; i < SIZE(Path); ++i)
		{
		char c = Path[i];
		if (c == 'M')
			{
			FoundM = true;
			LastMPos = i;
			}
		if (FoundM)
			TrimmedPath.push_back(c);
		else
			{
			if (c == 'D')
				++PrefixDCount;
			else if (c == 'I')
				++PrefixICount;
			else
				asserta(false);
			}
		}
	TrimmedPath.resize(LastMPos);
	for (unsigned i = LastMPos+1; i < SIZE(Path); ++i)
		{
		char c = Path[i];
		if (c == 'D')
			++SuffixDCount;
		else if (c == 'I')
			++SuffixICount;
		else
			asserta(false);
		}

	TrimmedHit.LoA = Hit.LoA + PrefixDCount;
	TrimmedHit.HiA = Hit.HiA - SuffixDCount;
	TrimmedHit.Strand = Hit.Strand;
	TrimmedHit.User = UINT_MAX;
	TrimmedHit.Score = Hit.Score;
	if (TrimmedHit.Strand)
		{
		TrimmedHit.LoB = Hit.LoB + PrefixICount;
		TrimmedHit.HiB = Hit.HiB - SuffixICount;
		}
	else
		{
		TrimmedHit.LoB = Hit.LoB + SuffixICount;
		TrimmedHit.HiB = Hit.HiB - PrefixICount;
		}
	}

// Chop at first overlap point found
static void ChopHits(const HitData &Hit1, const HitData &Hit2,
  vector<HitData> &Hits)
	{
	const unsigned SeqIndex_A = 0;
	const unsigned SeqIndex_B = 1;

	Hits.clear();
	HitData H1;
	HitData H2;

	//if (Hit1.LoA >= Hit2.LoA && Hit1.LoA <= Hit1.HiA)
	//	{
	//	ChopHit(Hit2, Hit1.LoA, SeqIndex_A, ChoppedHit1, ChoppedHit2);
	//	Hits.push_back(ChoppedHit1);
	//	Hits.push_back(ChoppedHit2);
	//	return;
	//	}

#define x(_1, _2, _Lo, _A, _B) \
	if (Hit##_1.##_Lo##_A > Hit##_2.##_Lo##_A && Hit##_1.##_Lo##_A < Hit##_2.Hi##_A)  \
		{  \
		vector<HitData> ChoppedHits; \
		ChopHit(Hit##_2, Hit##_1.##_Lo##_A, SeqIndex_##_A, ChoppedHits);  \
		Hits.insert(Hits.end(), ChoppedHits.begin(), ChoppedHits.end()); \
		return;  \
		}
x(1, 2, Lo, A, B)
x(1, 2, Lo, B, A)
x(2, 1, Lo, A, B)
x(2, 1, Lo, B, A)
x(1, 2, Hi, A, B)
x(1, 2, Hi, B, A)
x(2, 1, Hi, A, B)
x(2, 1, Hi, B, A)
	}

void Chop(const vector<HitData> &Hits, vector<HitData> &Hits2)
	{
	vector<HitData> TmpHits = Hits;
	for (;;)
		{
		bool Any = false;
		unsigned HitCount = SIZE(TmpHits);
		for (unsigned i = 0; i < HitCount; ++i)
			{
			const HitData &Hiti = TmpHits[i];
			for (unsigned j = i + 1; j < HitCount; ++j)
				{
				const HitData &Hitj = TmpHits[j];
				vector<HitData> ChoppedHits;
				ChopHits(Hiti, Hitj, ChoppedHits);
				if (!ChoppedHits.empty())
					{
					Any = true;
					vector<HitData> TmpHits2;
					for (unsigned k = 0; k < HitCount; ++k)
						{
						if (k == i || k == j)
							continue;
						TmpHits2.push_back(TmpHits[k]);
						}
					TmpHits2.insert(TmpHits2.end(), ChoppedHits.begin(), ChoppedHits.end());
					TmpHits = TmpHits2;
					goto Next;
					}
				}
			}
	Next:
		if (!Any)
			{
			Hits2.clear();
			for (unsigned i = 0; i < SIZE(TmpHits); ++i)
				{
				const HitData &Hit = TmpHits[i];
				if (Hit.LoA >= Hit.HiA || Hit.LoB >= Hit.HiB)
					continue;
				HitData TrimmedHit;
				TrimHit(Hit, TrimmedHit);
				if (TrimmedHit.GetAvgLength() < opt_minlocallen)
					continue;
				Hits2.push_back(TrimmedHit);
				}
			return;
			}
		}
	}

// DESCENDING by score
static const vector<HitData> *g_Hits;
static bool LT(unsigned i, unsigned j)
	{
	return (*g_Hits)[i].Score > (*g_Hits)[j].Score;
	}

void Range(vector<unsigned> &v, unsigned N)
	{
	v.clear();
	v.reserve(N);
	for (unsigned i = 0; i < N; ++i)
		v.push_back(i);
	}

static bool IsSubHit(const HitData &Hit1, const HitData &Hit2)
	{
	return Hit1.LoA >= Hit2.LoA && Hit1.HiA <= Hit2.HiA &&
	  Hit1.LoB >= Hit2.LoB && Hit1.HiB <= Hit2.HiB;
	}

void DeleteSubHits(const vector<HitData> &Hits, vector<HitData> &NewHits)
	{
	NewHits.clear();
	const unsigned HitCount = SIZE(Hits);
	vector<bool> Delete(HitCount, false);
	for (unsigned i = 0; i < HitCount; ++i)
		{
		const HitData &Hiti = Hits[i];
		if (Delete[i])
			continue;
		for (unsigned j = 0; j < HitCount; ++j)
			{
			if (Delete[j] || j == i)
				continue;
			const HitData &Hitj = Hits[j];
			if (IsSubHit(Hiti, Hitj))
				{
				Delete[i] = true;
				continue;
				}
			if (IsSubHit(Hitj, Hiti))
				Delete[j] = true;
			}
		}
	for (unsigned i = 0; i < HitCount; ++i)
		if (!Delete[i])
			NewHits.push_back(Hits[i]);
	}

//static void TrimMargin(HitData &Hit1, HitData &Hit2)
//	{
//	if (Lo > Hi)
//		return;
//	if (Hi - Lo < 8)
//		{
//		Warning("Trimmable margin");
//		//unsigned Mid = (Hi + Lo)/2;
//		//Hi = Mid;
//		//Lo = Mid + 1;
//		}
//	}
//
//static void TrimMarginsPair(HitData &Hit1, HitData &Hit2)
//	{
//	TrimMargin(Hit1.LoA, Hit2.HiA);
//	TrimMargin(Hit2.LoA, Hit1.HiA);
//	TrimMargin(Hit1.LoB, Hit2.HiB);
//	TrimMargin(Hit2.LoB, Hit1.HiB);
//	}

//static bool SizeOK(const HitData &Hit)
//	{
//	if (Hit.HiA <= Hit.LoA)
//		return false;
//	if (Hit.HiB <= Hit.LoB)
//		return false;
//	if (Hit.HiA - Hit.LoA + 1 < opt_minlocallen)
//		return false;
//	if (Hit.HiB - Hit.LoB + 1 < opt_minlocallen)
//		return false;
//	return true;
//	}

//static void TrimMargins(const vector<HitData> &Hits, vector<HitData> &NewHits)
//	{
//	NewHits.clear();
//	vector<HitData> TmpHits(Hits);
//	const unsigned HitCount = SIZE(Hits);
//	for (unsigned i = 0; i < HitCount; ++i)
//		{
//		HitData &Hiti = TmpHits[i];
//		for (unsigned j = i + 1; j < HitCount; ++j)
//			{
//			HitData &Hitj = TmpHits[j];
//			TrimMarginsPair(Hiti, Hitj);
//			}
//		}
//	for (unsigned i = 0; i < HitCount; ++i)
//		{
//		const HitData &H = TmpHits[i];
//		if (SizeOK(H))
//			NewHits.push_back(H);
//		}
//	}

unsigned Overlap(const HitData &H1, const HitData &H2, unsigned SeqIndex)
	{
	unsigned Lo1 = H1.GetLo(SeqIndex);
	unsigned Hi1 = H1.GetHi(SeqIndex);
	unsigned Lo2 = H2.GetLo(SeqIndex);
	unsigned Hi2 = H2.GetHi(SeqIndex);
	return Overlap(Lo1, Hi1, Lo2, Hi2);
	}

bool HasOverlap(const HitData &H1, const HitData &H2)
	{
	return Overlap(H1, H2, 0) > 0 || Overlap(H1, H2, 1) > 0;
	}

static void Compute1MonotonicChain(const vector<HitData> &Hits,
  unsigned SeqIndex, vector<unsigned> &Chain)
	{
	Chain.clear();

	vector<unsigned> HitIndexes;
	const unsigned HitCount = SIZE(Hits);
	vector<unsigned> SortOrder;
	Range(SortOrder, HitCount);

	g_Hits = &Hits;
	sort(SortOrder.begin(), SortOrder.end(), LT);

	for (unsigned i = 0; i < HitCount; ++i)
		{
		unsigned HitIndex = SortOrder[i];
		const HitData &H = Hits[HitIndex];
		bool Delete = false;
		for (unsigned j = 0; j < SIZE(HitIndexes); ++j)
			if (Overlap(H, Hits[HitIndexes[j]], SeqIndex) != 0)
				{
				Delete = true;
				break;
				}
		if (!Delete)
			HitIndexes.push_back(HitIndex);
		}

	for (unsigned i = 0; i < SIZE(HitIndexes); ++i)
		{
		unsigned HitIndex = HitIndexes[i];
		const HitData &H = Hits[HitIndex];
		Chain.push_back(H.User);
		}
	}

void SuperMap(const vector<HitData> &Hits,
  vector<unsigned> &ChainA, vector<unsigned> &ChainB)
	{
	ChainA.clear();
	ChainB.clear();
	const unsigned HitCount = SIZE(Hits);
	vector<HitData> Hits2(Hits);
	for (unsigned i = 0; i < HitCount; ++i)
		Hits2[i].User = i;

	vector<HitData> Hits3;
	DeleteSubHits(Hits2, Hits3);
	Hits2 = Hits3;

	//TrimMargins(Hits2, Hits3);//@@
	//Hits2 = Hits3;

	//Log("\n");
	//Log("After margins trimmed:\n");
	//LogHits(Hits2);

	Compute1MonotonicChain(Hits2, 0, ChainA);
	Compute1MonotonicChain(Hits2, 1, ChainB);
	}

void TestSuperMap()
	{
	unsigned HitIndex = 0;
	vector<HitData> Hits;
#define X(loA, loB, hiA, hiB, strand, score)	\
		{  \
		HitData H;  \
		H.User = HitIndex++;  \
		H.LoA = loA;  \
		H.LoB = loB;  \
		H.HiA = hiA;  \
		H.HiB = hiB;  \
		H.Strand = strand;  \
		H.Score = score;  \
		Hits.push_back(H);  \
		}
	X(0, 0, 100, 100, true, 1.0)
	X(98, 98, 200, 200, true, 1.0)
	X(20, 20, 40, 40, true, 1.0)
	X(500, 500, 502, 502, true, 1.0)
	X(50, 50, 150, 150, false, 2.0)
#undef X

	LogHits(Hits);
	vector<unsigned> Chain1;
	vector<unsigned> Chain2;
	SuperMap(Hits, Chain1, Chain2);

	vector<HitData> Hits1;
	vector<HitData> Hits2;

	for (unsigned i = 0; i < SIZE(Chain1); ++i)
		{
		unsigned HitIndex = Chain1[i];
		Hits1.push_back(Hits[HitIndex]);
		}

	for (unsigned i = 0; i < SIZE(Chain2); ++i)
		{
		unsigned HitIndex = Chain2[i];
		Hits2.push_back(Hits[HitIndex]);
		}

	Log("\n");
	Log("Hits1:\n");
	LogHits(Hits1);

	Log("\n");
	Log("Hits2:\n");
	LogHits(Hits2);
	}
