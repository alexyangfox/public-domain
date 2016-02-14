#include "myutils.h"
#include "seqdb.h"
#include "params.h"
#include "hit.h"
#include "isgap.h"
#include <algorithm>

byte CompLetter(byte c);

#define TRACE	0

void GetGoodSegments(const SeqDB &msa, float SwitchPenalty,
  vector<SeqDB *> &msas)
	{
	msas.clear();

#if	TRACE
	Log("\n");
	Log("FindBSegments\n");
#endif

	const unsigned ColCount = msa.GetColCount();
	if (ColCount == 0)
		return;

	vector<float> ColScores;
	for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
		ColScores.push_back(msa.GetColScore(ColIndex));

	vector<float> DPG;
	vector<float> DPB;
	string TBG;
	string TBB;

	DPG.push_back(ColScores[0]);
	DPB.push_back(-SwitchPenalty);
	TBG.push_back('S');
	TBB.push_back('S');

#if	TRACE
	Log("  Col  ColScore        GG        BG        BB        GB  TBG  TBB\n");
	Log("-----  --------  --------  --------  --------  --------  ---  ---\n");
#endif
	for (unsigned i = 0; i < ColCount; ++i)
		{
		float ColScore = ColScores[i];
		float GG = DPG[i] + ColScore;
		float BG = DPB[i] + ColScore - SwitchPenalty;
		float GB = DPG[i] - SwitchPenalty;
		float BB = DPB[i];

		if (GG >= BG)
			{
			DPG.push_back(GG);
			TBG.push_back('G');
			}
		else
			{
			DPG.push_back(BG);
			TBG.push_back('B');
			}

		if (BB >= GB)
			{
			DPB.push_back(BB);
			TBB.push_back('B');
			}
		else
			{
			DPB.push_back(GB);
			TBB.push_back('G');
			}
#if	TRACE
		Log("%5u  %8.1f  %8.1f  %8.1f  %8.1f  %8.1f  %3c  %3c  ",
		  i, ColScore, GG, BG, BB, GB, TBG[i], TBB[i]);
		msa.LogCol(i);
		Log("\n");
#endif
		}

	char State = '?';
	if (DPG[ColCount] >= DPB[ColCount])
		State = 'G';
	else
		State = 'B';

	string Path;
	for (int ColIndex = ColCount; ColIndex > 0; --ColIndex)
		{
		Path.push_back(State);
		State = (State == 'G' ? TBG[ColIndex] : TBB[ColIndex]);
		}
	reverse(Path.begin(), Path.end());

#if	TRACE
	for (unsigned i = 0; i < msa.GetSeqCount(); ++i)
		{
		unsigned L = msa.GetSeqLength(i);
		const byte *Seq = msa.GetSeq(i);
		Log("%*.*s\n", L, L, Seq);
		}
	Log("%s\n", Path.c_str());
#endif

	unsigned StartBad = UINT_MAX;
	bool AllGaps = true;
	for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
		{
		if (Path[ColIndex] == 'B')
			{
			if (StartBad == UINT_MAX)
				{
				StartBad = ColIndex;
				AllGaps = true;
				}
			AllGaps = AllGaps && msa.HasGap(ColIndex);
			}
		else
			{
			if (StartBad != UINT_MAX)
				{
				if (AllGaps)
					{
					for (unsigned i = StartBad; i < ColIndex; ++i)
						Path[i] = 'G';
					}
				StartBad = UINT_MAX;
				}
			}
		}
	if (StartBad != UINT_MAX)
		{
		if (AllGaps)
			{
			for (unsigned i = StartBad; i < ColCount; ++i)
				Path[i] = 'G';
			}
		}
#if TRACE
	Log("%s\n", Path.c_str());
#endif

	unsigned StartGood = UINT_MAX;
	for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
		{
		if (Path[ColIndex] == 'G')
			{
			if (StartGood == UINT_MAX)
				StartGood = ColIndex;
			}
		else
			{
			if (StartGood != UINT_MAX && ColIndex - StartGood >= opt_minlocallen)
				{
				SeqDB *tmp = new SeqDB;
				msas.push_back(tmp);
				msas.back()->FromColRange(msa, StartGood, ColIndex-1);
#if	TRACE
				{
				char Name[64];
				sprintf(Name, "Cols%u-%u", StartGood, ColIndex-1);
				msas.back()->m_Name = Name;
				msas.back()->LogMe();
				}
#endif
				}
			StartGood = UINT_MAX;
			}
		}
	if (StartGood != UINT_MAX && ColCount - StartGood >= opt_minlocallen)
		{
		SeqDB *tmp = new SeqDB;
		msas.push_back(tmp);
		msas.back()->FromColRange(msa, StartGood, ColCount-1);
#if	TRACE
		{
		char Name[64];
		sprintf(Name, "Cols%u-%u", StartGood, ColCount-1);
		msas.back()->m_Name = Name;
		msas.back()->LogMe();
		}
#endif
		}
	}

static float GetColPairScore(const SeqDB &msa1, unsigned i,
  const SeqDB &msa2, unsigned j, bool Plus)
	{
	const unsigned SeqCount1 = msa1.GetSeqCount();
	const unsigned SeqCount2 = msa2.GetSeqCount();
	float **SubstMx = GetSubstMx();
	float Score = 0;
	for (unsigned SeqIndex1 = 0; SeqIndex1 < SeqCount1; ++SeqIndex1)
		{
		byte c1 = msa1.Get(SeqIndex1, i);
		for (unsigned SeqIndex2 = 0; SeqIndex2 < SeqCount2; ++SeqIndex2)
			{
			byte c2 = msa2.Get(SeqIndex2, j);
			if (!Plus)
				c2 = CompLetter(c2);
			Score += SubstMx[c1][c2];
			}
		}
	return Score;
	}

// Extend hit with columns scoring >= 0
static void ExtendHit(const SeqDB &msa1, const SeqDB &msa2, 
  const HitData &Hit, HitData &XHit)
	{
	const unsigned ColCount1 = msa1.GetColCount();
	const unsigned ColCount2 = msa2.GetColCount();
	XHit = Hit;
	if (Hit.Strand)
		{
		for (;;)
			{
			if (XHit.LoA == 0 || XHit.LoB == 0)
				break;
			if (GetColPairScore(msa1, XHit.LoA-1, msa2, XHit.LoB-1, true) < 0)
				break;
			--XHit.LoA;
			--XHit.LoB;
			XHit.Path = 'M' + XHit.Path;
			}

		for (;;)
			{
			if (XHit.HiA+1 == ColCount1 || XHit.HiB+1 == ColCount2)
				break;
			if (GetColPairScore(msa1, XHit.HiA+1, msa2, XHit.HiB+1, true) < 0)
				break;
			++XHit.HiA;
			++XHit.HiB;
			XHit.Path += 'M';
			}
		}
	else
		{
		for (;;)
			{
			if (XHit.LoA == 0 || XHit.HiB+1 == ColCount2)
				break;
			if (GetColPairScore(msa1, XHit.LoA-1, msa2, XHit.HiB+1, false) < 0)
				break;
			--XHit.LoA;
			++XHit.HiB;
			XHit.Path = 'M' + XHit.Path;
			}

		for (;;)
			{
			if (XHit.HiA+1 == ColCount1 || XHit.HiB == 0)
				break;
			if (GetColPairScore(msa1, XHit.HiA+1, msa2, XHit.LoB-1, true) < 0)
				break;
			++XHit.HiA;
			--XHit.LoB;
			XHit.Path += 'M';
			}
		}
#if	TRACE
	Log("Hit: ");
	Hit.LogMe(true);
	Log("\nXHit:");
	XHit.LogMe(true);
	Log("\n");
#endif
	Hit.ValidatePath();
	}

void ExtendHits(const SeqDB &msa1, const SeqDB &msa2, 
  const vector<HitData> &Hits, vector<HitData> &ExtendedHits)
	{
	ExtendedHits.clear();
	const unsigned HitCount = SIZE(Hits);
	for (unsigned i = 0; i < HitCount; ++i)
		{
		const HitData &Hit = Hits[i];
		HitData ExtendedHit;
		ExtendHit(msa1, msa2, Hit, ExtendedHit);
		asserta(ExtendedHit.LoA <= ExtendedHit.HiA);
		asserta(ExtendedHit.LoB <= ExtendedHit.HiB);
		ExtendedHits.push_back(ExtendedHit);
		}
	}
