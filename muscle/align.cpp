#include "myutils.h"
#include "seqdb.h"
#include "mx.h"
#include "sparsemx.h"
#include "tree.h"
#include "params.h"

float ComputeColProbs(SeqDB &Input, const SeqDB &msa,
  vector<unsigned> &LetterPairCounts, vector<float> &PairProbs,
  vector<float> &ColProbs, float &SumPairProbs, float &SumColProbs);

void LogPosteriorDotPlots(vector<SparseMx *> &MatchPosteriors)
	{
	const unsigned N = SIZE(MatchPosteriors);
	for (unsigned i = 0; i < N; ++i)
		MatchPosteriors[i]->LogSmallDotPlot();
	}

SeqDB &SeqDB::Align(unsigned ConsIters, unsigned RefineIters,
  bool DoSequenceWeighting, const string &SubFamFileNamePrefix)
	{
	if (opt_localtree)
		{
		ComputeSPPs(true);
		ComputeAccAndIdMxs();
		ComputeGuideTree();
		ClearSPPs();
		ComputeSPPs(false);
		}
	else
		{
		if (opt_cons > 0)
			{
			ComputeSPPs(false);
			ComputeAccAndIdMxs();
			}
		ComputeGuideTree();
		}

	if (DoSequenceWeighting)
		ComputeSeqWeights(m_GuideTree);

	for (unsigned i = 0; i < ConsIters; ++i)
		Cons(i, ConsIters);

	if (opt_optimize != "")
		{
		unsigned SeqIndex = GetSeqIndex(opt_optimize);
		m_Weights[SeqIndex] = opt_optimize_f;
		}

	if (opt_accweight)
		{
		vector<float> AvgAccs;
		vector<float> FractIds;
		float Acc;
		float FractId;
		ComputeAccsAndIds(AvgAccs, FractIds, Acc, FractId);
		vector<float> NewWeights;

		const unsigned SeqCount = GetSeqCount();
		float SumWeights = 0;
		for (unsigned i = 0; i < SeqCount; ++i)
			{
			float Acc = AvgAccs[i];
			if (Acc < 0.2f)
				Acc = 0.2f;
			float NewWeight = m_Weights[i]/Acc;
			SumWeights += NewWeight;
			NewWeights.push_back(NewWeight);
			}
		for (unsigned i = 0; i < SeqCount; ++i)
			NewWeights[i] /= SumWeights;
		}

	SeqDB &msa = ProgressiveAlign(SubFamFileNamePrefix);
	if (msa.GetSeqCount() <= 2)
		return msa;

	for (unsigned i = 0; i < RefineIters; ++i)
		{
		ProgressStep(i, RefineIters, "Refine");
		Refine(msa, i, RefineIters);
		}

	if (opt_highprobupper)
		{
		vector<float> ColProbs;
		vector<float> PairProbs;
		vector<unsigned> LetterPairCounts;
		float SumColProbs;
		float SumPairProbs;
		ComputeColProbs(*this, msa, LetterPairCounts, PairProbs, ColProbs,
		  SumPairProbs, SumColProbs);

		vector<bool> Uppers;
		const unsigned ColCount = msa.GetColCount();
		for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
			{
			unsigned PairCount = LetterPairCounts[ColIndex];
			float ColProb = ColProbs[ColIndex];
			bool Upper = (PairCount > 0 && ColProb >= opt_mincolprob);
			Uppers.push_back(Upper);
			}
		msa.SetColCase(Uppers);
		}
	if (opt_treeorder)
		msa.SortByTree(m_GuideTree);
	else
		msa.SortByUser();
	return msa;
	}
