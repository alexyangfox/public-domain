#include "myutils.h"
#include "mx.h"
#include "seqdb.h"
#include "params.h"
#include "sparsemx.h"
#include "hit.h"

#define TRACE	0

byte CompLetter(byte c);
void RevComp(const byte *Seq, byte *RevCompSeq, unsigned L);
void LogLocalAlnHit(SeqDB &DB, unsigned IdA, unsigned IdB, const HitData &Hit);
void MergeHits(const vector<HitData> &Hits, vector<HitData> &MergedHits);
void LogHits(const vector<HitData> &Hits);
void DotPlotHits(const vector<HitData> &Hits, unsigned LA, unsigned LB);
void FwdBwd(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &PPMx);
float Viterbi(Mx<float> &MatchMxf, string &Path);
void GetLetterCounts(const string &Path, unsigned &i, unsigned &j);
float GetFractId(const byte *Seq1, const byte *Seq2, const string &Path,
  unsigned Lo1 = 0, unsigned Lo2 = 0);

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

static bool ValidColPair(unsigned i1, unsigned j1, unsigned i2, unsigned j2,
  bool Strand1, bool Strand2,
  const vector<unsigned> &BestHitA, const vector<unsigned> &BestHitB)
	{
	if (Strand1 != Strand2)
		return false;

	if (!Strand1)
		return ValidColPair(i1, j2, i2, j1, true, true, BestHitA, BestHitB);

	for (unsigned i = i1 + 1; i < i2; ++i)
		if (BestHitA[i] != UINT_MAX)
			return false;

	for (unsigned j = j1 + 1; j < j2; ++j)
		if (BestHitB[j] != UINT_MAX)
			return false;

	if (i2 <= i1 || j2 <= j1)
		return false;
	if (i2 != i1 + 1 && j2 != j1 + 1)
		return false;

	return true;
	}

static bool ExtendsHit(const HitData &Hit, unsigned i, unsigned j, bool Strand,
  vector<unsigned> &BestHitA, vector<unsigned> &BestHitB)
	{
	if (Strand != Hit.Strand)
		return false;

	if (!(Hit.HiA < i))
		return false;

	if (Strand)
		{
		if (!(Hit.HiB <= j))
			return false;

		for (unsigned j2 = Hit.HiB + 1; j2 < j; ++j2)
			if (BestHitB[j2] != UINT_MAX)
				return false;
		}
	else
		{
		if (!(Hit.LoB >= j))
			return false;

		for (unsigned j2 = j + 1; j2 < Hit.LoB; ++j2)
			if (BestHitB[j2] != UINT_MAX)
				return false;
		}

	for (unsigned i2 = Hit.HiA + 1; i2 < i; ++i2)
		if (BestHitA[i2] != UINT_MAX)
			return false;

	if (i - Hit.HiA == 1)
		return true;

	if (Strand)
		{
		if (j - Hit.HiB == 1)
			return true;
		}
	else
		{
		if (Hit.LoB - j == 1)
			return true;
		}

	if (i - Hit.HiA > opt_maxbubble)
		return false;

	if (Strand)
		{
		if (j - Hit.HiB > opt_maxbubble)
			return false;
		}
	else
		{
		if (Hit.LoB - j > opt_maxbubble)
			return false;
		}

	return true;
	}

#if	TRACE
static bool ValidBubble(unsigned i1, unsigned j1, unsigned i2, unsigned j2)
	{
	if (i1 >= i2 || j1 >= j2)
		return false;
	return i2 - i1 <= opt_maxbubble || j2 - j1 <= opt_maxbubble;
	}

static void LogBestHits(unsigned LA, unsigned LB, const vector<unsigned> BestHitA,
  const vector<unsigned> &BestHitB, bool Plus)
	{
	unsigned ABestCount = 0;
	unsigned BBestCount = 0;
	unsigned RecipCount = 0;

	Log("\n");
	Log("BestHitA:\n");
	Log("  Pos   HitA\n");
	Log("-----  -----\n");
	for (unsigned i = 0; i < LA; ++i)
		{
		Log("%5u", i);
		unsigned j = BestHitA[i];
		if (j == UINT_MAX)
			Log("      *");
		else
			{
			++ABestCount;
			Log("  %5u", j);
			}
		Log("\n");
		}

	Log("\n");
	Log("BestHitB:\n");
	Log("  Pos   HitB\n");
	Log("-----  -----\n");
	for (unsigned i = 0; i < LB; ++i)
		{
		Log("%5u", i);
		unsigned j = BestHitB[i];
		if (j == UINT_MAX)
			Log("      *");
		else
			{
			++BBestCount;
			Log("  %5u", j);
			}
		Log("\n");
		}

	Log("\n");
	Log("Reciprocal:\n");
	Log("    i      j\n");
	Log("-----  -----\n");
	unsigned Lasti = UINT_MAX;
	unsigned Lastj = UINT_MAX;
	for (unsigned i = 0; i < LA; ++i)
		{
		unsigned j =  BestHitA[i];
		if (j == UINT_MAX)
			continue;
		if (BestHitB[j] != i)
			continue;
		++RecipCount;
		Log("%5u  %5u", i, j);
		if (i > 0)
			{
			if (ValidColPair(Lasti, Lastj, i, j, Plus, Plus, BestHitA, BestHitB))
				Log(" #");
			else if (ValidBubble(Lasti, Lastj, i, j))
				Log(" $$");
			else
				Log(" .");
			}
		Lasti = i;
		Lastj = j;
		Log("\n");
		}

	Log("\n");
	Log("A only:\n");
	Log("    i      j\n");
	Log("-----  -----\n");
	for (unsigned i = 0; i < LA; ++i)
		{
		unsigned j =  BestHitA[i];
		bool Strand = BestStrandA[i];
		if (j == UINT_MAX)
			continue;
		if (BestHitB[j] == i)
			continue;
		Log("%5u  %5u", i, j);
		if (i > 0)
			{
			if (ValidColPair(Lasti, Lastj, i, j, Plus, Plus, BestHitA, BestHitB))
				Log(" #");
			else if (ValidBubble(Lasti, Lastj, i, j))
				Log(" $$");
			else
				Log(" .");
			}
		Lasti = i;
		Lastj = j;
		Log("\n");
		}

	Log("\n");
	Log("B only:\n");
	Log("    i      j\n");
	Log("-----  -----\n");
	for (unsigned j = 0; j < LB; ++j)
		{
		unsigned i =  BestHitB[j];
		bool Strand = BestStrandB[j];
		if (i == UINT_MAX)
			continue;
		if (BestHitA[i] == j)
			continue;
		Log("%5u  %5u", i, j);
		if (i > 0)
			{
			if (ValidColPair(Lasti, Lastj, i, j, Plus, Plus,
			  BestHitA, BestHitB))
				Log(" #");
			else if (ValidBubble(Lasti, Lastj, i, j))
				Log(" $$");
			else
				Log(" .");
			}
		Lasti = i;
		Lastj = j;
		Log("\n");
		}

	Log("\n");
	Log("LA      %5u\n", LA);
	Log("LB      %5u\n", LB);
	Log("ABest   %5u\n", ABestCount);
	Log("BBest   %5u\n", BBestCount);
	Log("Recip   %5u\n", RecipCount);
	}
#endif

static bool AddHit(HitData &Hit, vector<HitData> &Hits,
  vector<unsigned> &BestHitA, vector<unsigned> &BestHitB)
	{
	if (Hit.LoA == UINT_MAX || Hit.LoB == UINT_MAX)
		return false;

	if (Hit.GetAvgLength() < opt_minlocallen)
		return false;

	asserta(Hit.LoA <= Hit.HiA);
	asserta(Hit.LoB <= Hit.HiB);

	Hits.push_back(Hit);

	for (unsigned i = Hit.LoA; i <= Hit.HiA; ++i)
		BestHitA[i] = UINT_MAX;

	for (unsigned j = Hit.LoB; j <= Hit.HiB; ++j)
		BestHitB[j] = UINT_MAX;

	return true;
	}

void MultiLocal(Mx<float> &PPMx, bool Plus, float MinProb, vector<HitData> &Hits)
	{
	Hits.clear();

	const unsigned LA = PPMx.m_RowCount - 1;
	const unsigned LB = PPMx.m_ColCount - 1;

	vector<unsigned> BestHitA(LA, UINT_MAX);
	vector<unsigned> BestHitB(LB, UINT_MAX);
	
	float **PP = PPMx.GetData();
#if	TRACE
	Log("\n");
	Log("    i  BestProb  Bestj\n");
	Log("-----  --------  -----\n");
#endif
	for (unsigned i = 0; i < LA; ++i)
		{
		unsigned BestPos = UINT_MAX;
		float BestProb = 0.0f;
		for (unsigned j = 0; j < LB; ++j)
			{
			float Prob = PP[i+1][j+1];
			if (Prob > BestProb)
				{
				BestProb = Prob;
				BestPos = j;
				}
			}
		if (BestProb < MinProb)
			BestPos = UINT_MAX;
		BestHitA[i] = BestPos;
#if	TRACE
		{
		Log("%5u  %8.4f", i, BestProb);
		if (BestPos == UINT_MAX)
			Log("  %5.5s", "*");
		else
			Log("  %5u", BestPos);

		Log(" ");
		for (unsigned k = 0; k < G_MSA1->GetSeqCount(); ++k)
			Log("%c", G_MSA1->Get(k, i));

		Log(" ");
		for (unsigned k = 0; k < G_MSA2->GetSeqCount(); ++k)
			{
			char c = BestPos == UINT_MAX ? '*' : G_MSA2->Get(k, BestPos);
			if (!Plus)
				c = CompLetter(c);
			Log("%c", c);
			}

		Log("\n");
		}
#endif
		}

#if	TRACE
	Log("\n");
	Log("    j  BestProb  Besti\n");
	Log("-----  --------  -----\n");
#endif
	for (unsigned j = 0; j < LB; ++j)
		{
		unsigned BestPos = UINT_MAX;
		float BestProb = 0.0f;
		for (unsigned i = 0; i < LA; ++i)
			{
			float Prob = PP[i+1][j+1];
			if (Prob > BestProb)
				{
				BestProb = Prob;
				BestPos = i;
				}
			}
		if (BestProb < MinProb)
			BestPos = UINT_MAX;
		BestHitB[j] = BestPos;
#if	TRACE
		Log("%5u  %8.4f", j, BestProb);
		if (BestPos == UINT_MAX)
			Log("  %5.5s", "*");
		else
			Log("  %5u", BestPos);
#endif
		}

#if	TRACE
	LogBestHits(LA, LB, BestHitA, BestHitB);
#endif

	HitData Hit;
	Hit.Strand = Plus;

#if TRACE
	Log("\n");
	Log("Reciprocal\n");
	Log("    i  Bestj  Besti  Ex\n");
	Log("-----  -----  -----  --\n");
#endif
// --------------------------------------------------------
// Reciprocal
// --------------------------------------------------------
	Hit.LoA = UINT_MAX;
	for (unsigned i = 0; i < LA; ++i)
		{
#if	TRACE
		{
		Log("%5u", i);
		unsigned j = BestHitA[i];
		if (j == UINT_MAX)
			Log("  %5s\n", "*");
		else
			{
			Log("  %5u", j);
			unsigned Besti = BestHitB[j];
			if (Besti == UINT_MAX)
				Log("  %5s\n", "*");
			else
				{
				Log("  %5u", Besti);
				if (Besti != i)
					Log("\n");
				}
			}
		}
#endif
		unsigned j = BestHitA[i];
		if (j == UINT_MAX)
			continue;
		if (BestHitB[j] != i)
			continue;

		bool Ex = Hit.LoA != UINT_MAX && ExtendsHit(Hit, i, j, Plus, BestHitA, BestHitB);
#if	TRACE
		Log("  %2c\n", tof(Ex));
#endif
		if (Ex)
			{
			for (unsigned i2 = Hit.HiA + 1; i2 < i; ++i2)
				Hit.Path.push_back('D');
			Hit.HiA = i;
			if (Plus)
				{
				for (unsigned j2 = Hit.HiB + 1; j2 < j; ++j2)
					Hit.Path.push_back('I');
				Hit.HiB = j;
				}
			else
				{
				for (unsigned j2 = j + 1; j2 < Hit.LoB; ++j2)
					Hit.Path.push_back('I');
				Hit.LoB = j;
				}
			Hit.Path.push_back('M');
			}
		else
			{
#if	TRACE
			Log("AddHit: ");
			Hit.LogMe(true);
#endif
			AddHit(Hit, Hits, BestHitA, BestHitB);

			Hit.LoA = i;
			Hit.HiA = i;
			Hit.LoB = j;
			Hit.HiB = j;
			Hit.Strand = Plus;
			Hit.Path = "M";
			}
		}
	AddHit(Hit, Hits, BestHitA, BestHitB);

// --------------------------------------------------------
// A only
// --------------------------------------------------------
	Hit.LoA = UINT_MAX;
	for (unsigned i = 0; i < LA; ++i)
		{
		unsigned j = BestHitA[i];
		if (j == UINT_MAX)
			continue;

		bool Ex = Hit.LoA != UINT_MAX && ExtendsHit(Hit, i, j, Plus, BestHitA, BestHitB);
		if (Ex)
			{
			for (unsigned i2 = Hit.HiA + 1; i2 < i; ++i2)
				Hit.Path.push_back('D');
			Hit.HiA = i;
			if (Plus)
				{
				for (unsigned j2 = Hit.HiB + 1; j2 < j; ++j2)
					Hit.Path.push_back('I');
				Hit.HiB = j;
				}
			else
				{
				for (unsigned j2 = j + 1; j2 < Hit.LoB; ++j2)
					Hit.Path.push_back('I');
				Hit.LoB = j;
				}
			Hit.Path.push_back('M');
			}
		else
			{
			AddHit(Hit, Hits, BestHitA, BestHitB);

			Hit.LoA = i;
			Hit.HiA = i;
			Hit.LoB = j;
			Hit.HiB = j;
			Hit.Strand = Plus;
			Hit.Path = "M";
			}
		}
	AddHit(Hit, Hits, BestHitA, BestHitB);

// --------------------------------------------------------
// B only
// --------------------------------------------------------
	Hit.LoB = UINT_MAX;
	for (unsigned j = 0; j < LB; ++j)
		{
		unsigned i = BestHitB[j];
		if (i == UINT_MAX)
			continue;

		bool Ex = Hit.LoB != UINT_MAX && ExtendsHit(Hit, i, j, Plus, BestHitA, BestHitB);
		if (Ex)
			{
			for (unsigned j2 = Hit.HiB + 1; j2 < j; ++j2)
				Hit.Path.push_back('I');
			Hit.HiB = j;
			if (Plus)
				{
				for (unsigned i2 = Hit.HiA + 1; i2 < i; ++i2)
					Hit.Path.push_back('D');
				Hit.HiA = i;
				}
			else
				{
				for (unsigned i2 = i + 1; i2 < Hit.LoA; ++i2)
					Hit.Path.push_back('D');
				Hit.LoA = i;
				}
			Hit.Path.push_back('M');
			}
		else
			{
			AddHit(Hit, Hits, BestHitA, BestHitB);

			Hit.LoA = i;
			Hit.HiA = i;
			Hit.LoB = j;
			Hit.HiB = j;
			Hit.Strand = Plus;
			Hit.Path = "M";
			}
		}
	AddHit(Hit, Hits, BestHitA, BestHitB);
#if	TRACE
	Log("\n");
	Log("Final hits:\n");
	LogHits(Hits);
	DotPlotHits(Hits, LA, LB);
#endif
	}

 void RevCompPair(SeqDB &DB, unsigned IdA, unsigned IdB, SeqDB &DBRevCompPair)
	{
	const byte *A = DB.GetSeq(IdA);
	const byte *B = DB.GetSeq(IdB);
	unsigned LA = DB.GetSeqLength(IdA);
	unsigned LB = DB.GetSeqLength(IdB);

	const string &LabelA = DB.GetLabel(IdA);
	const string &LabelB = DB.GetLabel(IdB);
	string LabelBRC = LabelB + string(".revcomp");

	byte *BRevComp = myalloc<byte>(LB);
	RevComp(B, BRevComp, LB);

	DBRevCompPair.AppendSeq(LabelA, A, LA);
	DBRevCompPair.AppendSeq(LabelBRC, BRevComp, LB);
	}

static void RealignHit(SeqDB &DB, unsigned IdA, unsigned IdB,
  const HitData &Hit, HitData &RealignedHit)
	{
	SeqDB DB2;
	
	const string &LabelA = DB.GetLabel(IdA);
	const string &LabelB = DB.GetLabel(IdB);
	
	byte *SeqA = DB.GetSeq(IdA) + Hit.LoA;
	byte *SeqB = DB.GetSeq(IdB) + Hit.LoB;

	unsigned LoA = DB.GetLo(IdA) + Hit.LoA;
	unsigned LoB = DB.GetLo(IdB) + Hit.LoB;

// ORIGINAL SEQ STUFF /NOT/ DONE PROPERLY HERE!
	DB2.AddSeq(LabelA, SeqA, Hit.GetLengthA(), 1.0, 0, LoA, true);
	DB2.AddSeq(LabelB, SeqB, Hit.GetLengthB(), 1.0, 0, LoB, true);

// PP and DB2RC must stay in scope until Viterbi() completes.
	Mx<float> PP;
	SeqDB DB2RC;

	if (Hit.Strand)
		FwdBwd(DB2, 0, 1, PP);
	else
		{
		RevCompPair(DB2, 0, 1, DB2RC);
		FwdBwd(DB2RC, 0, 1, PP);
		}

	RealignedHit = Hit;
	Viterbi(PP, RealignedHit.Path);
	RealignedHit.ValidatePath();
	}

static void Merge(HitData &H, const HitData &H2)
	{
#if	TRACE
	Log("Merge ");
	H.LogMe();
	Log(" and ");
	H2.LogMe();
#endif
	H.LoA = min(H.LoA, H2.LoA);
	H.HiA = max(H.HiA, H2.HiA);
	H.LoB = min(H.LoB, H2.LoB);
	H.HiB = max(H.HiB, H2.HiB);
	H.Score += H2.Score;
	H.Path.clear();
	asserta(H.LoA <= H.HiA);
	asserta(H.LoB <= H.HiB);
#if	TRACE
	Log(" result: ");
	H.LogMe();
	Log("\n");
#endif
	}

static bool Between(const HitData &H1, const HitData &H2, const HitData &H3)
	{
	if (H2.LoA > H1.LoA && H2.LoA < H3.LoA)
		return true;
	if (H2.LoB > H1.LoB && H2.LoB < H3.LoB)
		return true;
	return false;
	}

static bool CoLinear(const HitData &H1, const HitData &H2)
	{
	if (H1.Strand != H2.Strand)
		return false;

	if (H1.Strand)
		return H1.HiA < H2.LoA && H1.HiB < H2.LoB;
	else
		return H1.HiA < H2.LoA && H1.HiB > H2.LoB;
	}

#pragma message("TODO: add segs at terminals")
void MergeHits(const vector<HitData> &Hits, vector<HitData> &MergedHits)
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

void AlignSeqPairLocal(SeqDB &DB, unsigned IdA, unsigned IdB, vector<HitData> &Hits)
	{
	Hits.clear();

	const unsigned LA = DB.GetSeqLength(IdA);
	const unsigned LB = DB.GetSeqLength(IdB);

	string Model;
	GetLocalModel(DB, Model);
	FWD_BWD FB = SetModel(Model);

	SetSimMx(DB, IdA, IdB);

	Mx<float> PPMx;
	FB(PPMx);
#if	TRACE
	PPMx.LogMe();
#endif

	vector<HitData> FirstPassHits;
	MultiLocal(PPMx, true, opt_minlocalprob, FirstPassHits);

	if (DB.IsNucleo())
		{
		SeqDB DBRC;
		RevCompPair(DB, IdA, IdB, DBRC);
		SetSimMx(DBRC, 0, 1);

		Mx<float> PPMxRC;
		FB(PPMxRC);

	// Adjust coords to non-revcomp'd B
		float **PPRC = PPMxRC.GetData();
		for (unsigned i = 1; i <= LA; ++i)
			for (unsigned j = 1; j <= LB/2; ++j)
				{
				float p1 = PPRC[i][j];
				float p2 = PPRC[i][LB-j+1];
				PPRC[i][LB-j+1] = p1;
				PPRC[i][j] = p2;
				}

		vector<HitData> RCHits;
		MultiLocal(PPMxRC, false, opt_minlocalprob, RCHits);

		FirstPassHits.insert(FirstPassHits.end(), RCHits.begin(), RCHits.end());
		}

	//vector<HitData> FirstPassHits;
	//MultiLocal(PPMx, PPMxRC, opt_minlocalprob, FirstPassHits);

	vector<HitData> MergedHits;

	bool OnePass = (opt_onepass || (IdA == IdB && opt_self1));
	if (OnePass)
		MergedHits = FirstPassHits;
	else
		MergeHits(FirstPassHits, MergedHits);

#if	TRACE
	Log("\n");
	Log("First pass hits\n");
	Log("---------------\n");
	LogHits(Hits);
	DotPlotHits(Hits, LA, LB);

	Log("\n");
	Log("Merged hits\n");
	Log("-----------\n");
	LogHits(MergedHits);
	DotPlotHits(MergedHits, LA, LB);
#endif

	const byte *A = DB.GetSeq(IdA);
	const byte *B = DB.GetSeq(IdB);
	for (unsigned HitIndex = 0; HitIndex < SIZE(MergedHits); ++HitIndex)
		{
		const HitData &Hit = MergedHits[HitIndex];
		HitData RealignedHit;
		RealignHit(DB, IdA, IdB, Hit, RealignedHit);
		if (Hit.GetAvgLength() < opt_minlocallen)
			continue;
		float PctId = GetFractId(A, B, Hit.Path, Hit.LoA, Hit.LoB)*100.0f;
		if (PctId < opt_minlocalid)
			continue;

#if	TRACE
		LogLocalAlnHit(DB, IdA, IdB, RealignedHit);
#endif
		Hits.push_back(RealignedHit);
		}
	}
