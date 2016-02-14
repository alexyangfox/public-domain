#include "myutils.h"
#include "sparsemx.h"
#include "seqdb.h"
#include "isgap.h"

#define TRACE	0

SparseMx &GetPairMx(vector<SparseMx> &MatchPosteriors, unsigned i, unsigned j,
  bool &Transpose);
void MSAToColIndexesVec(const SeqDB &msa, vector<vector<unsigned> > &ColIndexesVec);

void ComputeRowProbs(SeqDB &Input, const SeqDB &msa,
  vector<float> &RowProbs)
	{
	RowProbs.clear();

	const unsigned SeqCount = msa.GetSeqCount();
	const unsigned ColCount = msa.GetColCount();
	RowProbs.resize(SeqCount, 0.0f);

	for (unsigned SeqIndex1 = 0; SeqIndex1 < SeqCount; ++SeqIndex1)
		{
		const unsigned InputSeqIndex1 = msa.GetUser(SeqIndex1);
		const byte *Seq1 = msa.GetSeq(SeqIndex1);
		float SumProb = 0.0f;
		unsigned LetterPairCount = 0;
		for (unsigned SeqIndex2 = 0; SeqIndex2 < SeqCount; ++SeqIndex2)
			{
			if (SeqIndex2 == SeqIndex1)
				continue;

			const unsigned InputSeqIndex2 = msa.GetUser(SeqIndex2);
			const byte *Seq2 = msa.GetSeq(SeqIndex2);

			bool Transpose;
			SparseMx &PairMx = Input.GetSPP(InputSeqIndex1, InputSeqIndex2, Transpose);

			unsigned Pos1 = 0;
			unsigned Pos2 = 0;
			for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
				{
				char c1 = Seq1[ColIndex];
				char c2 = Seq2[ColIndex];

				bool Gap1 = isgap(c1);
				bool Gap2 = isgap(c2);
				if (!Gap1 && !Gap2)
					{
				// @@ TODO: Get slow?
					float Prob = Transpose ? PairMx.Get(Pos2, Pos1) : PairMx.Get(Pos1, Pos2);
					SumProb += Prob;
					++LetterPairCount;
					}
				if (!Gap1)
					++Pos1;
				if (!Gap2)
					++Pos2;
				}
			}
		if (LetterPairCount == 0)
			RowProbs[SeqIndex1] = 0.0f;
		else
			RowProbs[SeqIndex1] = SumProb/LetterPairCount;
		}
	}
