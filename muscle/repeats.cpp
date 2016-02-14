#include "myutils.h"
#include "seqdb.h"
#include "isgap.h"
#include "sparsemx.h"
#include "mx.h"
#include <algorithm>

#define TRACE	0

static vector<unsigned> Pos1ToCol;
static vector<unsigned> Pos2ToCol;
static vector<unsigned> ColToPos1;
static vector<unsigned> ColToPos2;

static void GetStarts(SeqDB &DB, unsigned Id, unsigned Starti,
  vector<unsigned> &Starts)
	{
	Starts.clear();
	unsigned SeqLength = DB.GetSeqLength(Id);
	Starts.push_back(Starti);
	unsigned i = Starti;
	unsigned j;
	for (;;)
		{
		if (i >= SeqLength)
			break;
		unsigned Col = Pos1ToCol[i];
		if (Col == UINT_MAX)
			{
			++i;
			continue;
			}
		j = ColToPos2[Col];
		if (j != UINT_MAX)
			{
			Starts.push_back(j);
			i = j;
			}
		else
			++i;
		}

#if	TRACE
	{
	const byte *Seq = DB.GetSeq(Id);
	Log("Starts: ");
	for (unsigned k = 0; k < SIZE(Starts); ++k)
		Log(" %u", Starts[k]);
	Log("\n");

	for (unsigned k = 0; k < SIZE(Starts)-1; ++k)
		{
		unsigned StartPos = Starts[k];
		unsigned EndPos = Starts[k+1]-1;
		Log("%5u - %5u ", StartPos, EndPos);
		for (unsigned l = StartPos; l <= EndPos; ++l)
			Log("%c", Seq[l]);
		Log("\n");
		}
	}
#endif
	}

void FindRepeats(SeqDB &DB, unsigned Id, unsigned Starti, unsigned Startj,
  unsigned &RepeatLength, float &RepeatCount, float &RepeatPctId,
  const string &Path)
	{
	if (!opt_findrepmotifs)
		return;

#if	TRACE
	Log("\n");
	Log("Find repeats Path=%s\n", Path.c_str());
#endif

	unsigned i = Starti;
	unsigned j = Startj;
	asserta(j > i);
	const unsigned ColCount = SIZE(Path);
	const unsigned SeqLength = DB.GetSeqLength(Id);

	Pos1ToCol.clear();
	Pos2ToCol.clear();
	ColToPos1.clear();
	ColToPos2.clear();

	Pos1ToCol.resize(SeqLength, UINT_MAX);
	Pos2ToCol.resize(SeqLength, UINT_MAX);
	ColToPos1.resize(ColCount, UINT_MAX);
	ColToPos2.resize(ColCount, UINT_MAX);

	byte *Seq = DB.GetSeq(Id);

#if	TRACE
	Log("c  xx   Pos1   Pos2\n");
	Log("-  --  -----  -----\n");
#endif
	vector<unsigned> PairDists;
	for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
		{
		char c = Path[ColIndex];
#if	TRACE
		{
		Log("%c  ", c);
		if (c == 'M' || c == 'D')
			Log("%c", Seq[i]);
		else
			Log("-");
		if (c == 'M' || c == 'I')
			Log("%c", Seq[j]);
		else
			Log("-");
		if (c == 'M' || c == 'D')
			Log("  %5u", i);
		else
			Log("       ");
		if (c == 'M' || c == 'I')
			Log("  %5u", j);
		Log("\n");
		}
#endif
		switch (c)
			{
		case 'M':
			{
			Pos1ToCol[i] = ColIndex;
			Pos2ToCol[j] = ColIndex;
			ColToPos1[ColIndex] = i;
			ColToPos2[ColIndex] = j;
			assert(j >= i);
			unsigned PairDist = j - i;
			PairDists.push_back(PairDist);
			++i;
			++j;
			continue;
			}
		case 'D':
			{
			Pos1ToCol[i] = ColIndex;
			ColToPos1[ColIndex] = i;
			ColToPos2[ColIndex] = UINT_MAX;
			++i;
			continue;
			}
		case 'I':
			{
			Pos2ToCol[j] = ColIndex;
			ColToPos1[ColIndex] = UINT_MAX;
			ColToPos2[ColIndex] = j;
			++j;
			continue;
			}
		default:
			asserta(false);
			}
		}
	unsigned MaxPos = j - 1;
	
	vector<unsigned> Starts;
	GetStarts(DB, Id, Starti, Starts);

	unsigned PairCount = SIZE(PairDists);
	asserta(PairCount > 0);
	sort(PairDists.begin(), PairDists.end());
	RepeatLength = PairDists[PairCount/2];
	unsigned RegionLength = MaxPos - Starti + 1;
	RepeatCount = float(RegionLength)/float(RepeatLength);
#if	TRACE
	Log("RepeatLength=%u RepeatCount=%.1f\n", RepeatLength, RepeatCount);
#endif
	RepeatPctId = 0.0f;
	if (RepeatLength >= opt_minreplen)
		{
		SeqDB RepeatDB;
		vector<unsigned> Ids;
		for (unsigned k = 0; k < SIZE(Starts)-1; ++k)
			{
			unsigned StartPos = Starts[k];
			unsigned EndPos = Starts[k+1] - 1;
			unsigned L = EndPos - StartPos + 1;
			RepeatDB.AddSeq("Repeat", Seq+StartPos, L, 1.0f, k);
			Ids.push_back(k);
			}

		SeqDB &RepeatMSA = RepeatDB.ProgressiveAlign("");
		RepeatMSA.LogMe();
		RepeatPctId = RepeatMSA.GetAvgPctId();
		}
	}
