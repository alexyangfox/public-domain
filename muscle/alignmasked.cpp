#include "myutils.h"
#include "seqdb.h"
#include "sparsemx.h"
#include "info.h"
#include <algorithm>

#if	0
void ComputeDistMx(SeqDB &DB, vector<SparseMx> &MatchPosteriors,
  const vector<unsigned> &Ids, Mx<float> &DistMxf);
void ComputePairMatchPosteriors(SeqDB &DB, vector<unsigned> &Ids,
  vector<SparseMx> &MatchPosteriors);
void Align(SeqDB &DB, vector<unsigned> &Ids,
  vector<SparseMx> &MatchPosteriors, Mx<float> &DistMxf, SeqDB &msa);

bool ismask(byte c)
	{
	switch (c)
		{
	case '&':
	case '+':
	case '@':
	case '>':
		return true;
		}
	return false;
	}

static unsigned GetMaskCount(const byte *Seq, unsigned L)
	{
	unsigned Count = 0;
	for (unsigned i = 0; i < L; ++i)
		if (ismask(Seq[i]))
			++Count;
	return Count;
	}

static void Mask(byte *Seq, unsigned InputSeqIndex)
	{
	const vector<RepeatInfo> &Repeats = GetRepeatInfos();
	const vector<DupeInfo> &Dupes = GetDupeInfos();
	const vector<InvertInfo> &Inverts = GetInvertInfos();

	{
	const unsigned Count = SIZE(Repeats);
	for (unsigned Index = 0; Index < Count; ++Index)
		{
		const RepeatInfo &I = Repeats[Index];
		if (I.InputSeqIndex != InputSeqIndex)
			continue;

		for (unsigned Pos = I.Start; Pos <= I.End; ++Pos)
			Seq[Pos] = '&';
		}
	}

	{
	const unsigned Count = SIZE(Dupes);
	for (unsigned Index = 0; Index < Count; ++Index)
		{
		const DupeInfo &I = Dupes[Index];
		if (I.InputSeqIndex != InputSeqIndex)
			continue;

		for (unsigned Pos = I.Start1; Pos <= I.End1; ++Pos)
			Seq[Pos] = '+';

		for (unsigned Pos = I.Start2; Pos <= I.End2; ++Pos)
			Seq[Pos] = '+';
		}
	}

	{
	const unsigned Count = SIZE(Inverts);
	for (unsigned Index = 0; Index < Count; ++Index)
		{
		const InvertInfo &I = Inverts[Index];
		if (I.InputSeqIndex1 == InputSeqIndex)
			{
			for (unsigned Pos = I.Start1; Pos <= I.End1; ++Pos)
				Seq[Pos] = '>';
			}
		if (I.InputSeqIndex2 == InputSeqIndex)
			{
			for (unsigned Pos = I.Start2; Pos <= I.End2; ++Pos)
				Seq[Pos] = '>';
			}
		}
	}
	}

void AlignMasked(SeqDB &DB)
	{
	SeqDB MaskedDB;
	unsigned Bytes = DB.GetSeqBytes();
	MaskedDB.Reserve(Bytes);

	unsigned TotalMaskedCount = 0;
	vector<string> ExcludedLabels;
	const unsigned SeqCount = DB.GetSeqCount();
	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{
		const byte *InSeq = DB.GetSeq(SeqIndex);
		const string &Label = DB.GetLabel(SeqIndex);
		unsigned SeqLength = DB.GetSeqLength(SeqIndex);

		byte *OutSeq = myalloc<byte>(SeqLength);
		memcpy(OutSeq, InSeq, SeqLength);

		Mask(OutSeq, SeqIndex);
		unsigned Count = GetMaskCount(OutSeq, SeqLength);
		TotalMaskedCount += Count;
		if (float(Count)*100.0f/float(SeqLength) > opt_maxmaskpct)
			{
			ExcludedLabels.push_back(Label);
			continue;
			}

		MaskedDB.AppendSeq(Label, OutSeq, SeqLength);
		myfree(OutSeq);
		}

	if (!ExcludedLabels.empty())
		{
		Log("\n");
		Log("Excluded due to re-arrangment masking:\n");
		for (unsigned i = 0; i < SIZE(ExcludedLabels); ++i)
			Log(">%s\n", ExcludedLabels[i].c_str());
		}

	if (TotalMaskedCount == 0)
		{
		Log("\n");
		Log("No rearrangements, skip masked SeqDB step.\n");
		return;
		}

	unsigned MaskedSeqCount = MaskedDB.GetSeqCount();
	vector<unsigned> Ids;
	for (unsigned i = 0; i < MaskedSeqCount; ++i)
		Ids.push_back(i);

	vector<SparseMx> PairMatchPosteriors;
	ComputePairMatchPosteriors(MaskedDB, Ids, PairMatchPosteriors);

	Mx<float> DistMxf;
	ComputeDistMx(MaskedDB, PairMatchPosteriors, Ids, DistMxf);

	SeqDB msa;

	bool opt_subfams_save = opt_subfams;
	opt_subfams = false;
	Align(MaskedDB, Ids, PairMatchPosteriors, DistMxf, msa);
	opt_subfams = opt_subfams_save;

	msa.m_Name = "Masked for re-arrangements";
	msa.LogMe();
	}
#endif //0
