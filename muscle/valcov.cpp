#include "myutils.h"
#include "hit.h"
#include "seqdb.h"
#include <algorithm>

#define TRACE	1

void LogBlockParents(bool WithLabels);

static vector<pair<SeqDB *, unsigned> > g_Pairs;

static void GetBPs(SeqDB &msa, unsigned Id, vector<BPData> &BPs)
	{
	for (unsigned i = 0; i < msa.GetSeqCount(); ++i)
		{
		if (msa.GetUser(i) != Id)
			continue;
		BPData BP;
		unsigned Index = SIZE(g_Pairs);
		g_Pairs.push_back(pair<SeqDB *, unsigned>(&msa, i));
		BP.User = Index;
		BP.Pos = msa.GetLo(i);
		BP.lo = true;
		BPs.push_back(BP);
		BP.Pos = msa.GetHi(i);
		BP.lo = false;
		BPs.push_back(BP);
		}
	}

static unsigned ReportOverlaps(unsigned Id, vector<BPData> &BPs,
  unsigned &Index)
	{
	Index = UINT_MAX;
	if (BPs.empty())
		return 0;

	sort(BPs.begin(), BPs.end());

	unsigned OverlapCount = 0;
	int Depth = 0;
#if	TRACE
	Log("\n");
	Log("ReportOverlaps(%u) %u BPs\n", Id, SIZE(BPs));
	Log("   BP         Pos  lo  Depth\n");
	Log("-----  ----------  --  -----\n");
#endif
	for (unsigned i = 0; i < SIZE(BPs); ++i)
		{
		const BPData &BP = BPs[i];
		if (BP.lo)
			{
			++Depth;
			if (Depth > 1)
				{
				++OverlapCount;
				if (Index == UINT_MAX)
					Index = BP.User;
				Log("Start overlap id=%u lo=%u\n", Id, BP.Pos);
				}
			}
		else
			{
			--Depth;
			if (Depth == 1)
				Log("End overlap id=%u hi=%u\n", Id, BP.Pos);
			}
#if	TRACE
		Log("%5u  %10u  %2s  %5d\n",
		  i, BP.Pos, BP.lo ? "lo" : "HI", Depth);
#endif
		}
	asserta(Depth == 0);
	return OverlapCount;
	}

void ValidateCov(const vector<vector<SeqDB *> > &MSAVec)
	{
	Log("\n");
	Log(" Node   Seqs\n");
	Log("-----  -----\n");
	vector<vector<BPData> > BPVec;
	const unsigned NodeCount = SIZE(MSAVec);
	for (unsigned NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
		{
		const vector<SeqDB *> &MSAs = MSAVec[NodeIndex];
		const unsigned MSACount = SIZE(MSAs);
		for (unsigned MSAIndex = 0; MSAIndex < MSACount; ++MSAIndex)
			{
			SeqDB &msa = *MSAs[MSAIndex];
			unsigned N = msa.GetSeqCount();
			if (N < 2)
				continue;
			Log("%5u  %5u ", NodeIndex, N);
			for (unsigned Id = 0; Id < N; ++Id)
				{
				unsigned InputId = msa.GetUser(Id);
				Log(" %u", InputId);
				if (InputId >= SIZE(BPVec))
					BPVec.resize(InputId+1);
				GetBPs(msa, InputId, BPVec[InputId]);
				}
			Log("\n");
			}
		}

	const unsigned SeqCount = SIZE(BPVec);
	for (unsigned Id = 0; Id < SeqCount; ++Id)
		{
		unsigned Index = UINT_MAX;
		unsigned Count = ReportOverlaps(Id, BPVec[Id], Index);
		if (Count == 0)
			continue;
		Log("\n");
		Log("%u overlaps index=%u\n", Count, Index);
		pair<SeqDB *, unsigned> &Pair = g_Pairs[Index];
		const SeqDB *BadMSA = Pair.first;
		unsigned BadId = Pair.second;

		const unsigned NodeCount = SIZE(MSAVec);
		for (unsigned NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
			{
			const vector<SeqDB *> &MSAs = MSAVec[NodeIndex];
			const unsigned MSACount = SIZE(MSAs);
			for (unsigned MSAIndex = 0; MSAIndex < MSACount; ++MSAIndex)
				{
				const SeqDB &msa = *MSAs[MSAIndex];
				if (&msa == BadMSA)
					Log("BAD MSA, BAD ID %u=%s\n",
					BadId, msa.GetLabel(BadId).c_str());
				msa.LogMe();
				}
			}
		LogBlockParents(false);
		Die("Overlaps");
		}
	}
