#include "myutils.h"
#include "seqdb.h"
#include "params.h"
#include "hit.h"

#define TRACE	1

void SetSimMxMSAs(SeqDB &msa1, SeqDB &msa2);
void MultiLocal(Mx<float> &PPMx, bool Plus, float MinProb, vector<HitData> &Hits);
void MergeHits(const vector<HitData> &Hits, vector<HitData> &MergedHits);
void LogLocalAlnAlnHit(SeqDB &DBA, SeqDB &DBB, const HitData &Hit);
void FwdBwd(Mx<float> &PPMx);
float Viterbi(Mx<float> &Mxf, string &Path);
void LogSmallDotPlot(const Mx<float> &Mx, const string &Name, unsigned MaxL);
void ExtendHits(const SeqDB &msa1, const SeqDB &msa2, 
  const vector<HitData> &Hits, vector<HitData> &ExtendedHits);

static void RealignMSAHit(SeqDB &msa1, SeqDB &msa2,
  const HitData &Hit, HitData &RealignedHit)
	{
	SeqDB DB1;
	SeqDB DB2;
	
	DB1.FromColRange(msa1, Hit.LoA, Hit.HiA);
	DB2.FromColRange(msa2, Hit.LoB, Hit.HiB);

// PP and DB2RC must stay in scope until Viterbi() completes.
	Mx<float> PP;
	SeqDB DB2RC;

	if (Hit.Strand)
		SetSimMxMSAs(DB1, DB2);
	else
		{
		DB2RC.Copy(DB2);
		DB2RC.RevComp();
		SetSimMxMSAs(DB1, DB2RC);
		}

	FwdBwd(PP);

	RealignedHit = Hit;
	Viterbi(PP, RealignedHit.Path);
	RealignedHit.ValidatePath();
	}

// Very similar code to AlignSeqPair local, would be nice to consolidate.
void AlignMSAPairLocal(SeqDB &msa1, SeqDB &msa2, vector<HitData> &Hits)
	{
#if	TRACE
	Log("\n");
	Log("AlignMSAPairLocal\n");
#endif
	Hits.clear();

//	const unsigned SeqCount1 = msa1.GetSeqCount();
	const unsigned SeqCount2 = msa2.GetSeqCount();

	const unsigned ColCount1 = msa1.GetColCount();
	const unsigned ColCount2 = msa2.GetColCount();

	string Model;
	GetLocalModel(msa1, Model);
	FWD_BWD FB = SetModel(Model);

	SetSimMxMSAs(msa1, msa2);

	Mx<float> PPMx;
	FB(PPMx);

	if (opt_posteriors)
		WriteMx("MSAPairLocal", PPMx);
	if (opt_dotplots)
		LogSmallDotPlot(PPMx, "MSAPairLocal", 64);

	vector<HitData> FirstPassHits;
	MultiLocal(PPMx, true, opt_minlocalprob, FirstPassHits);

	if (msa1.IsNucleo())
		{
		Mx<float> PPMxRC;
		SeqDB DBRC;
		SeqDB DB2RC;
		DB2RC.Copy(msa2);
		for (unsigned i = 0; i < SeqCount2; ++i)
			DB2RC.RevComp(i);

		SetSimMxMSAs(msa1, DB2RC);
		FB(PPMxRC);
		if (opt_posteriors)
			WriteMx("MSAPairLocalRC", PPMxRC);
		if (opt_dotplots)
			LogSmallDotPlot(PPMxRC, "MSAPairLocalRC", 64);

	// Adjust coords to non-revcomp'd B
		float **PPRC = PPMxRC.GetData();
		for (unsigned i = 1; i <= ColCount1; ++i)
			for (unsigned j = 1; j <= ColCount2/2; ++j)
				{
				float p1 = PPRC[i][j];
				float p2 = PPRC[i][ColCount2-j+1];
				PPRC[i][ColCount2-j+1] = p1;
				PPRC[i][j] = p2;
				}

		vector<HitData> RCHits;
		MultiLocal(PPMxRC, false, opt_minlocalprob, RCHits);

		FirstPassHits.insert(FirstPassHits.end(), RCHits.begin(), RCHits.end());
		PPMxRC.Clear();
		}

//	MultiLocal(PPMx, PPMxRC, opt_minlocalprob, FirstPassHits);
	PPMx.Clear();

	vector<HitData> ExtendedHits;
	ExtendHits(msa1, msa2, FirstPassHits, ExtendedHits);
	FirstPassHits = ExtendedHits;

	if (opt_logfirstpasshits)
		for (unsigned i = 0; i < SIZE(FirstPassHits); ++i)
			{
			const HitData &Hit = FirstPassHits[i];
			LogLocalAlnAlnHit(msa1, msa2, Hit);
			}

	vector<HitData> MergedHits;

	bool OnePass = (opt_onepass || (&msa1 == &msa2 && opt_self1));
	if (OnePass)
		MergedHits = FirstPassHits;
	else
		MergeHits(FirstPassHits, MergedHits);

#if	TRACE
	Log("\n");
	Log("First pass hits\n");
	Log("---------------\n");
	LogHits(FirstPassHits);
	DotPlotHits(FirstPassHits, ColCount1, ColCount2);

	Log("\n");
	Log("Merged hits\n");
	Log("-----------\n");
	LogHits(MergedHits);
	DotPlotHits(MergedHits, ColCount1, ColCount2);
#endif

	for (unsigned HitIndex = 0; HitIndex < SIZE(MergedHits); ++HitIndex)
		{
		const HitData &Hit = MergedHits[HitIndex];
		HitData RealignedHit;
		RealignMSAHit(msa1, msa2, Hit, RealignedHit);
		if (Hit.GetAvgLength() < opt_minlocallen)
			continue;

#if	TRACE
		LogLocalAlnAlnHit(msa1, msa2, RealignedHit);
#endif
		Hits.push_back(RealignedHit);
		}
	}

void Profile(SeqDB &msa1)
	{
	SeqDB msa2;
	msa2.ReadSeqs(opt_profile);

	if (!msa1.m_Aligned || !msa2.m_Aligned)
		Die("Input file is not aligned");

	vector<HitData> Hits;
	AlignMSAPairLocal(msa1, msa2, Hits);

	for (unsigned i = 0; i < SIZE(Hits); ++i)
		{
		const HitData &Hit = Hits[i];
		LogLocalAlnAlnHit(msa1, msa2, Hit);
		}
	}
