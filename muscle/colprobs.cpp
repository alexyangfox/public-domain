#include "myutils.h"
#include "sparsemx.h"
#include "seqdb.h"
#include "isgap.h"

#define TRACE	0

SparseMx &GetPairMx(vector<SparseMx> &MatchPosteriors, unsigned i, unsigned j,
  bool &Transpose);
void MSAToColIndexesVec(const SeqDB &msa, vector<vector<unsigned> > &ColIndexesVec);

float ComputeColProbs(SeqDB &Input, const SeqDB &msa,
  vector<unsigned> &LetterPairCounts, vector<float> &PairProbs,
  vector<float> &ColProbs, float &SumPairProbs, float &SumColProbs)
	{
#if	TRACE
	Log("\n");
	Log("ComputeColProbs()\n");
#endif

	LetterPairCounts.clear();
	PairProbs.clear();
	ColProbs.clear();

	const unsigned SeqCount = msa.GetSeqCount();
	const unsigned ColCount = msa.GetColCount();
	PairProbs.resize(ColCount, 0);
	ColProbs.resize(ColCount, 1);
	LetterPairCounts.resize(ColCount, 0);

	vector<unsigned> SeqPos(SeqCount, 0);
	float SumLetterPairProbs = 0;
	float LetterPairCount = 0;
	for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
		{
		for (unsigned SeqIndex1 = 0; SeqIndex1 < SeqCount; ++SeqIndex1)
			{
			byte c1 = msa.Get(SeqIndex1, ColIndex);
			if (isgap(c1))
				continue;
			unsigned Pos1 = SeqPos[SeqIndex1];
			unsigned InputSeqIndex1 = msa.GetUser(SeqIndex1);

			for (unsigned SeqIndex2 = 0; SeqIndex2 < SeqIndex1; ++SeqIndex2)
				{
				byte c2 = msa.Get(SeqIndex2, ColIndex);
				if (isgap(c2))
					continue;
				unsigned Pos2 = SeqPos[SeqIndex2];
				unsigned InputSeqIndex2 = msa.GetUser(SeqIndex2);

				++(LetterPairCounts[ColIndex]);
				++LetterPairCount;

				bool Transpose;
				SparseMx &PairMx = Input.GetSPP(InputSeqIndex1, InputSeqIndex2, Transpose);

				// @@ TODO: Get slow?
				float Prob = (Transpose ? PairMx.Get(Pos2, Pos1) : PairMx.Get(Pos1, Pos2));

				PairProbs[ColIndex] += Prob;
				ColProbs[ColIndex] *= Prob;
				SumLetterPairProbs += Prob;
				}
			}

		for (unsigned SeqIndex1 = 0; SeqIndex1 < SeqCount; ++SeqIndex1)
			{
			byte c1 = msa.Get(SeqIndex1, ColIndex);
			if (!isgap(c1))
				++(SeqPos[SeqIndex1]);
			}
		}

	SumPairProbs = 0;
	SumColProbs = 0;
	for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
		{
		SumPairProbs += PairProbs[ColIndex];
		SumColProbs += ColProbs[ColIndex];

		unsigned N = LetterPairCounts[ColIndex];
		if (N == 0)
			PairProbs[ColIndex] = 0;
		else
			PairProbs[ColIndex] /= N;
		}

	if (LetterPairCount == 0)
		return 0;
	return SumLetterPairProbs/LetterPairCount;
	}

void LogColProbs(SeqDB &Input, const SeqDB &msa)
	{
	vector<unsigned> LetterPairCounts;
	vector<float> PairProbs;
	vector<float> ColProbs;
	float SumPairProbs;
	float SumColProbs;
	float PredictedAccuracy =
	  ComputeColProbs(Input, msa, LetterPairCounts, PairProbs, ColProbs,
	    SumPairProbs, SumColProbs);
	Log("\n");
	Log("Expected accuracy=%.1f%%\n", PredictedAccuracy*100.0f);
	const unsigned ColCount = msa.GetColCount();
	float NonZeroColSum = 0.0f;
	unsigned NonZeroColCount = 0;
	for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
		{
		float Pp = PairProbs[ColIndex];
		float Pc = ColProbs[ColIndex];
		unsigned N = LetterPairCounts[ColIndex];
		if (Pc > 0 && N > 0)
			{
			++NonZeroColCount;
			NonZeroColSum += Pc;
			}

		string Col;
		msa.GetCol(ColIndex, Col);
		Log("%s  %7.4f", Col.c_str(), Pp);
		if (N == 0)
			Log("  %7.7s", "");
		else
			Log("  %7.4f", Pc);

		unsigned H = unsigned(Pp*10);
		Log("  ");
		for (unsigned i = 0; i < H; ++i)
			Log("%c", Pp > 0.5 ? '*' : '.');
		Log("\n");
		}
	float ColAvg = (NonZeroColCount == 0 ? 0.0f : NonZeroColSum/NonZeroColCount);
	Log("ColAvg = %g\n", ColAvg);
	}
