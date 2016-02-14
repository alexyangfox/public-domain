#include "myutils.h"
#include "sparsemx.h"
#include "seqdb.h"
#include "isgap.h"
#include "mx.h"

void MSAToColIndexesVec(const SeqDB &msa, vector<vector<unsigned> > &ColIndexesVec);
float Viterbi(Mx<float> &MatchMx, string &Path);;
byte *MakeGappedSeq(const byte *A, const string &Path, bool First);

static Mx<float> g_MatchMx;

Mx<float> &ComputeMatchMx1(vector<SparseMx> &MatchPosteriors, const SeqDB &msa)
	{
	const unsigned SeqCount = msa.GetSeqCount();

	vector<vector<unsigned> > ColIndexesVec;
	MSAToColIndexesVec(msa, ColIndexesVec);

	const unsigned ColCount = msa.GetColCount();
	const unsigned SeqLength = MatchPosteriors[0].m_RowCount - 1;
	g_MatchMx.Alloc("MatchMx", SeqLength, ColCount);
	g_MatchMx.Init(0);
	float **MatchMx = g_MatchMx.GetData();

	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{
		SparseMx &SPPMx = MatchPosteriors[SeqIndex];
		asserta(SPPMx.m_RowCount == SeqLength + 1);

		const vector<unsigned> &ColIndexes2 = ColIndexesVec[SeqIndex];
		asserta(SIZE(ColIndexes2) == SPPMx.m_ColCount);

	// ColIndex1 is the 0-based ungapped position in the solo sequence.
		for (unsigned ColIndex1 = 0; ColIndex1 < SeqLength; ++ColIndex1)
			{
			float *Values;
			unsigned *ColIndexes;
			const unsigned EntryCount = SPPMx.GetRow(ColIndex1+1, &Values, &ColIndexes);
			for (unsigned EntryIndex = 0; EntryIndex < EntryCount; ++EntryIndex)
				{
				float Value = Values[EntryIndex];

			// Pos2 is the 1-based ungapped position in the SeqDB sequence.
				unsigned Pos2 = ColIndexes[EntryIndex];

			// ColIndex2 is the 0-based column index of Pos2 within the SeqDB
				unsigned ColIndex2 = ColIndexes2[Pos2];
				assert(ColIndex1 < g_MatchMx.m_RowCount);
				assert(ColIndex2 < g_MatchMx.m_ColCount);
				MatchMx[ColIndex1][ColIndex2] += Value;
				}
			}
		}

	return g_MatchMx;
	}

void AlignSeqToMSA(SeqDB &DBSeqs, unsigned SeqIndex, const SeqDB &msa,
  vector<SparseMx> &MatchPosteriors, SeqDB &OutMSA)
	{
	OutMSA.Clear();

	Mx<float> &MatchMx = ComputeMatchMx1(MatchPosteriors, msa);

	string Path;
	Viterbi(MatchMx, Path);
	Log("Path=%s\n", Path.c_str());
	const unsigned ColCount = SIZE(Path);

	const unsigned SeqCount = msa.GetSeqCount();
	for (unsigned i = 0; i < SeqCount; ++i)
		{
		const byte *Seq = msa.GetSeq(i);
		const string &Label = msa.GetLabel(i);
		byte *NewSeq = MakeGappedSeq(Seq, Path, false);
		OutMSA.AddSeq(Label, NewSeq, ColCount);
		}

	const string &Label = DBSeqs.GetLabel(SeqIndex);
	const byte *Seq = DBSeqs.GetSeq(SeqIndex);
	byte *NewSeq = MakeGappedSeq(Seq, Path, true);
	OutMSA.AddSeq(Label, NewSeq, ColCount);
	}
