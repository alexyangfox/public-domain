#include "myutils.h"
#include "hit.h"
#include <algorithm>

#define TRACE	0

void GetUncoveredSegs(vector<BPData> &BPs, unsigned SeqLength,
  vector<SegData> &Segs)
	{
	Segs.clear();

	sort(BPs.begin(), BPs.end());

	unsigned Start = 0;
	int Depth = 0;
#if	TRACE
	Log("\n");
	Log("GetUncoveredSegs %u BPs\n", SIZE(BPs));
	Log("   BP         Pos       Start  lo  Depth\n");
	Log("-----  ----------  ----------  --  -----\n");
#endif
	for (unsigned i = 0; i < SIZE(BPs); ++i)
		{
		const BPData &BP = BPs[i];
		if (BP.lo)
			{
			if (Depth == 0 && BP.Pos > Start)
				{
				SegData Seg;
				Seg.Lo = Start;
				Seg.Hi = BP.Pos - 1;
				Seg.Strand = true;
				Segs.push_back(Seg);
				}
			Start = BP.Pos;
			++Depth;
			}
		else
			{
			Start = BP.Pos + 1;
			--Depth;
			}
#if	TRACE
		Log("%5u  %10u  %10u  %2s  %5d\n",
		  i, BP.Pos, Start, BP.lo ? "lo" : "HI", Depth);
#endif
		}
	asserta(Depth == 0);
	asserta(Start <= SeqLength);
	if (Start < SeqLength)
		{
		SegData Seg;
		Seg.Lo = Start;
		Seg.Hi = SeqLength - 1;
		Seg.Strand = true;
		Segs.push_back(Seg);
		}

#if	TRACE
	Log("\n");
	Log("%u uncovered segs\n", SIZE(Segs));
	Log("        Lo          Hi      Length  +\n");
	Log("----------  ----------  ----------  -\n");
	for (unsigned i = 0; i < SIZE(Segs); ++i)
		{
		const SegData &Seg = Segs[i];
		Log("%10u  %10u  %10u  %c\n", Seg.Lo, Seg.Hi, Seg.GetLength(), pom(Seg.Strand));
		}
#endif
	}

void AppendBPs(const vector<HitData> &Hits, vector<BPData> &BPs, bool DoA)
	{
	const unsigned HitCount = SIZE(Hits);
	for (unsigned i = 0; i < HitCount; ++i)
		{
		const HitData &Hit = Hits[i];

		BPData BP;
		BP.Pos = DoA ? Hit.LoA : Hit.LoB;
		BP.lo = true;
		BPs.push_back(BP);

		BP.Pos = DoA ? Hit.HiA : Hit.HiB;
		BP.lo = false;
		BPs.push_back(BP);
		}
	}
