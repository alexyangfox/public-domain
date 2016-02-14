#include "myutils.h"
#include "seqdb.h"
#include "mx.h"
#include "svnrevision.h"
#include "params.h"
#include "sparsemx.h"
#include "hit.h"

const char Version[] = "MUSCLE v4.0";

using namespace std;

void Align(SeqDB &DB, vector<unsigned> &Ids,
  vector<SparseMx> &MatchPosteriors, Mx<float> &DistMxf, SeqDB &msa);
void LogColProbs(SeqDB &Input, const SeqDB &msa);
void LogPosteriorDotPlots(vector<SparseMx *> &MatchPosteriors);
void ComputeSelfHitsDB(SeqDB &DB);
void Prune(SeqDB &Input);
void ComputeInverts(SeqDB &DB);
void ComputeLowComplexity(SeqDB &DB);
void LogAccs(SeqDB &DB);
void SelfAln(SeqDB &DB);
void LogMSACartoon(SeqDB &msa);
void XlatAlign(SeqDB &DB);
void FrameShift(SeqDB &DB);
void AlignMAF(SeqDB &DB);
void Profile(SeqDB &DB);
void SWAff(SeqDB &DB);

void LogSelfReport();
void LogInvertReport();
void AlignMasked(SeqDB &DB);

SeqDB *g_Input;

int main(int argc, char **argv)
	{
	MyCmdLine(argc, argv);

	if (opt_version)
		{
		printf("%s.%u\n", Version, SVN_REVISION);
		ProgressExit();
		return 0;
		}
	if (opt_svnrevision)
		{
		printf("%u", SVN_REVISION);
		// ProgressExit(); // Don't call ProgressExit!!
		return 0;
		}
	else if (opt_drawtree != "")
		{
		Tree t;
		t.FromFile(opt_drawtree);
		t.LogMePretty(true);
		ProgressExit();
		return 0;
		}

	if (opt_logmodels)
		{
		void LogModels();
		LogModels();
		ProgressExit();
		return 0;
		}
	else if (opt_self_aln != "")
		{
		SeqDB Input;
		Input.ReadSeqs(opt_self_aln);
		if (!Input.m_Aligned)
			Die("Sequences not aligned in file: '%.16s'", opt_self_aln.c_str());
		SelfAln(Input);
		ProgressExit();
		return 0;
		}
	else if (opt_input == "")
		Die("No input file specified");

	//if (opt_addseqs != "")
	//	{
	//	AddSeqs(opt_addseqs, opt_input);
	//	ProgressExit();
	//	return 0;
	//	}

	if (opt_logparams)
		LogModelParams();

	SeqDB Input;
	Input.ReadSeqs(opt_input);
	g_Input = &Input;

	if (opt_posregex != "")
		Input.SetPosFromLabels(opt_posregex);

	if (opt_model == "")
		{
		if (Input.GetSeqType() == ST_Amino)
			opt_model = "global+localaff";
		else
			opt_model = "globalnuc+localaffnuc";
		}

	if (opt_profile != "")
		{
		Profile(Input);
		ProgressExit();
		return 0;
		}

	if (opt_sw)
		{
		SWAff(Input);
		ProgressExit();
		return 0;
		}

	if (opt_maf != "")
		{
		AlignMAF(Input);
		return 0;
		}

	const unsigned SeqCount = Input.GetSeqCount();
	if (SeqCount == 0)
		{
		Warning("No sequences found.");
		ProgressExit();
		return 0;
		}

	if (opt_frameshift_only)
		{
		FrameShift(Input);
		ProgressExit();
		return 0;
		}

	if (opt_msa_only || opt_prune_only)
		{
		opt_self = false;
		opt_inversions = false;
		opt_lowc = false;
		}

	if (opt_inversions_only)
		{
		ComputeInverts(Input);
		LogInvertReport();
		ProgressExit();
		return 0;
		}

	if (opt_self)
		{
		ComputeSelfHitsDB(Input);

		if (opt_self_only)
			{
			LogSelfReport();
			ProgressExit();
			return 0;
			}
		}

	if (opt_inversions && Input.IsNucleo())
		ComputeInverts(Input);

	if (opt_self)
		LogSelfReport();

	if (opt_inversions && Input.IsNucleo())
		LogInvertReport();

	if (!opt_msa)
		{
		ProgressExit();
		return 0;
		}

	if (opt_prune)
		{
		Prune(Input);
		if (opt_prune_only)
			{
			ProgressExit();
			return 0;
			}
		}

	Input.ClearSPPs();
	SeqDB &msa = Input.Align(opt_cons, opt_refine, opt_seqweights, opt_subfamfiles);
	msa.m_Name = opt_input;
	if (opt_cons > 0)
		LogAccs(Input);

	if (opt_logtree)
		{
		Log("\n");
		Log("Guide tree:\n");
		Input.m_GuideTree.LogNewick();
		}

	if (opt_posteriors)
		{
		for (unsigned i = 0; i < SIZE(Input.m_SPPs); ++i)
			WriteSMx("PairwiseFinal", *Input.m_SPPs[i]);
		}

	if (opt_output != "")
		msa.ToFasta(opt_output);

	if (msa.GetSeqCount() == 1)
		{
		ProgressExit();
		return 0;
		}

	if (opt_colprobs)
		LogColProbs(Input, msa);

	if (opt_dotplots)
		LogPosteriorDotPlots(Input.m_SPPs);

	LogMSACartoon(msa);

	void LogSparseMxStats();
	LogSparseMxStats();//@@

	msa.m_Name = "All sequences, unmasked";
	msa.LogMe();

	ProgressExit();
	return 0;
	}
