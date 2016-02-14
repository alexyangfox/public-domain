#include "myutils.h"
#include "seqdb.h"
#include "params.h"
#include "mx.h"
#include "sparsemx.h"
#include "isgap.h"

#if 0
void FwdBwd(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &PPMx);
void AlignSeqToMSA(SeqDB &DBSeqs, unsigned SeqIndex, const SeqDB &msa,
  vector<SparseMx *> &MatchPosteriors, SeqDB &OutMSA);

unsigned StripGaps(const byte *s, unsigned L, byte *t)
	{
	unsigned NewL = 0;
	for (unsigned i = 0; i < L; ++i)
		{
		char c = s[i];
		if (!isgap(c))
			t[NewL++] = c;
		}
	return NewL;
	}

void ComputeSeqVsMSAMatchPosteriors(SeqDB &DB, unsigned Id, const SeqDB &msa,
  vector<SparseMx *> &MatchPosteriors)
	{
	MatchPosteriors.clear();
	const unsigned SeqCount = msa.GetSeqCount();
	MatchPosteriors.resize(SeqCount);

	byte *Seq1 = DB.GetSeq(Id);
	const string &Label1 = DB.GetLabel(Id);
	const unsigned L1 = DB.GetSeqLength(Id);

	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{
		const byte *Seq2 = msa.GetSeq(SeqIndex);
		const string &Label2 = msa.GetLabel(SeqIndex);
		const unsigned L2 = msa.GetSeqLength(SeqIndex);

		SeqDB DB2;
		DB2.AppendSeq(Label1, Seq1, L1);
		DB2.AppendSeq(Label2, Seq2, L2);
		DB.StripGaps();

		Mx<float> PPMx;
		FwdBwd(DB2, 0, 1, PPMx);
		SparseMx &SPPMx = MatchPosteriors[SeqIndex];
		SPPMx.FromMx(PPMx);
		SPPMx.m_SeqDB = 0;
		}
	}

void AddSeqs(const string &MSAFileName, const string &InputFileName)
	{
	SeqDB msa;
	SeqDB Input;
	msa.ReadSeqs(MSAFileName);
	Input.ReadSeqs(InputFileName);

	const unsigned InputSeqCount = Input.GetSeqCount();
	for (unsigned SeqIndex = 0; SeqIndex < InputSeqCount; ++SeqIndex)
		{
		vector<SparseMx *> MatchPosteriors;
		ComputeSeqVsMSAMatchPosteriors(Input, SeqIndex, msa, MatchPosteriors);

		SeqDB OutMSA;
		AlignSeqToMSA(Input, SeqIndex, msa, MatchPosteriors, OutMSA);
		msa.Copy(OutMSA);
		}

	msa.LogMe();
	if (opt_output != "")
		msa.ToFasta(opt_output);
	}
#endif //0
