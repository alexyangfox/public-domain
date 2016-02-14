#include "myutils.h"
#include "sparsemx.h"
#include "seqdb.h"
#include "isgap.h"
#include "seqdb.h"
#include "mx.h"

#define TRACE	0

SparseMx &GetPairMx(vector<SparseMx> &SPPs, unsigned i, unsigned j,
  bool &Transpose);
byte *MakeGappedSeq(const byte *A, const string &Path, bool First);
byte *MakeGappedSeq2(const byte *A, unsigned LoPos, const string &Path,
  bool First, bool Plus);
float Viterbi(Mx<float> &MatchMx, string &Path);
void SetSimMx(SeqDB &DB, unsigned IdA, unsigned IdB);
void FwdBwd(SeqDB &DB, unsigned IdA, unsigned IdB, SparseMx &SPP);
void RevComp(byte *Seq, unsigned L);
void GetLetterCounts(const string &Path, unsigned &i, unsigned &j);

extern SeqDB *g_Input;

static Mx<float> g_MatchMx;

static void SeqToColIndexes(const byte *Seq, unsigned ColCount,
  vector<unsigned> &ColIndexes)
	{
	ColIndexes.clear();
	ColIndexes.reserve(ColCount);
	ColIndexes.push_back(UINT_MAX); // 1-based ungapped positions
	for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
		{
		char c = Seq[ColIndex];
		if (!isgap(c))
			ColIndexes.push_back(ColIndex);
		}
	}

void MSAToColIndexesVec(const SeqDB &msa, vector<vector<unsigned> > &ColIndexesVec)
	{
	const unsigned SeqCount = msa.GetSeqCount();
	ColIndexesVec.clear();
	ColIndexesVec.resize(SeqCount);
	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{
		const byte *Seq = msa.GetSeq(SeqIndex);
		unsigned ColCount = msa.GetColCount();
		SeqToColIndexes(Seq, ColCount, ColIndexesVec[SeqIndex]);
		}
	}

#if	TRACE
static void LogMatchMx(Mx<float> &MatchMx, const SeqDB &msa1, const SeqDB &msa2)
	{
	const unsigned SeqCount1 = msa1.GetSeqCount();
	const unsigned SeqCount2 = msa2.GetSeqCount();

	const unsigned LA = msa1.GetColCount();
	const unsigned LB = msa2.GetColCount();

	Log("\n");
	Log("MatchMx\n");

	Log("Rows: ");
	for (unsigned i = 0; i < SeqCount1; ++i)
		Log(" [%u]%s", msa1.GetUser(i), msa1.GetLabel(i).c_str());
	Log("\n");

	Log("Cols: ");
	for (unsigned i = 0; i < SeqCount2; ++i)
		Log(" [%u]%s", msa2.GetUser(i), msa2.GetLabel(i).c_str());
	Log("\n");

	for (unsigned i = 0; i < SeqCount2; ++i)
		{
		for (unsigned j = 0; j < SeqCount1; ++j)
			Log(" ");
		Log("  ");

		for (unsigned j = 0; j < LB; ++j)
			Log("           %c", msa2.Get(i, j));
		Log("\n");
		}

	for (unsigned i = 0; i < LA; ++i)
		{
		for (unsigned j = 0; j < SeqCount1; ++j)
			Log("%c", msa1.Get(j, i));
		Log("  ");

		for (unsigned j = 0; j < LB; ++j)
			Log("%s", TypeToStr<float>(MatchMx.Get(i+1, j+1)));
		Log("\n");
		}
	}
#endif // TRACE

SparseMx &ComputeSPP(SeqDB &Input, const SeqDB &msa1, unsigned SeqIndex1, const SeqDB &msa2,
  unsigned SeqIndex2)
	{
	SparseMx &SPP = *new SparseMx;

	const byte *Seq1 = msa1.GetSeq(SeqIndex1);
	const byte *Seq2 = msa2.GetSeq(SeqIndex2);

	const string &Label1 = msa1.GetLabel(SeqIndex1);
	const string &Label2 = msa2.GetLabel(SeqIndex2);

	unsigned User1 = msa1.GetUser(SeqIndex1);
	unsigned User2 = msa2.GetUser(SeqIndex2);

	unsigned L1 = msa1.GetSeqLength(SeqIndex1);
	unsigned L2 = msa2.GetSeqLength(SeqIndex2);

	SeqDB DB;
	DB.AppendSeq(Label1, Seq1, L1, 1.0f, User1);
	DB.AppendSeq(Label2, Seq2, L2, 1.0f, User2);

	DB.StripGaps(0);
	DB.StripGaps(1);

	FwdBwd(DB, 0, 1, SPP);
	SPP.m_SeqDB = &Input;
	SPP.m_IdA = User1;
	SPP.m_IdB = User2;

	return SPP;
	}

Mx<float> &ComputeMatchMx(SeqDB &Input, const SeqDB &msa1, const SeqDB &msa2)
	{
#if	TRACE
	Log("\n");
	Log("ComputeMatchMx()\n");
	Log("msa1=\n");
	msa1.LogMe();
	Log("msa2=\n");
	msa2.LogMe();
#endif

	const unsigned SeqCount1 = msa1.GetSeqCount();
	const unsigned SeqCount2 = msa2.GetSeqCount();
	const unsigned PairCount = SeqCount1*SeqCount2;

	vector<SparseMx *> PairMxs(PairCount);
	vector<bool> Transposes(PairCount);
	unsigned PairIndex = 0;

	for (unsigned SeqIndex1 = 0; SeqIndex1 < SeqCount1; ++SeqIndex1)
		{
		const unsigned InputSeqIndex1 = msa1.GetUser(SeqIndex1);
		for (unsigned SeqIndex2 = 0; SeqIndex2 < SeqCount2; ++SeqIndex2)
			{
			const unsigned InputSeqIndex2 = msa2.GetUser(SeqIndex2);
			bool Transpose = false;
			SparseMx *ptrPairMx = 0;
			if (opt_cons == 0)
				ptrPairMx = &ComputeSPP(Input, msa1, SeqIndex1, msa2, SeqIndex2);
			else
				ptrPairMx = &Input.GetSPP(InputSeqIndex1, InputSeqIndex2, Transpose);

			PairMxs[PairIndex] = ptrPairMx;
			Transposes[PairIndex] = Transpose;
			++PairIndex;
			}
		}

	vector<vector<unsigned> > ColIndexesVec1;
	vector<vector<unsigned> > ColIndexesVec2;
	MSAToColIndexesVec(msa1, ColIndexesVec1);
	MSAToColIndexesVec(msa2, ColIndexesVec2);

	const unsigned ColCount1 = msa1.GetColCount();
	const unsigned ColCount2 = msa2.GetColCount();
	g_MatchMx.Alloc("MatchMx", ColCount1+1, ColCount2+1);
	g_MatchMx.Init(0);
	float **MatchMx = g_MatchMx.GetData();

	for (unsigned SeqIndex1 = 0; SeqIndex1 < SeqCount1; ++SeqIndex1)
		{
		const unsigned InputSeqIndex1 = msa1.GetUser(SeqIndex1);
		const unsigned L1 = Input.GetSeqLength(InputSeqIndex1);
		const vector<unsigned> &ColIndexes1 = ColIndexesVec1[SeqIndex1];
		asserta(SIZE(ColIndexes1) == L1+1);
		const float w1 = Input.GetSeqWeight(InputSeqIndex1);
		for (unsigned SeqIndex2 = 0; SeqIndex2 < SeqCount2; ++SeqIndex2)
			{
			const unsigned InputSeqIndex2 = msa2.GetUser(SeqIndex2);
			const unsigned L2 = Input.GetSeqLength(InputSeqIndex2);
			const vector<unsigned> &ColIndexes2 = ColIndexesVec2[SeqIndex2];
			const float w2 = Input.GetSeqWeight(InputSeqIndex2);
			const float w = w1*w2;
			asserta(SIZE(ColIndexes2) == L2+1);

			unsigned PairIndex = SeqIndex1*SeqCount2 + SeqIndex2;
			asserta(PairIndex < SIZE(PairMxs));
			SparseMx *PairMx = PairMxs[PairIndex];
			bool Transpose = Transposes[PairIndex];
			if (Transpose)
				{
				asserta(PairMx->GetRowLabel() == Input.GetLabel(InputSeqIndex2));
				asserta(PairMx->GetColLabel() == Input.GetLabel(InputSeqIndex1));
				}
			else
				{
				asserta(PairMx->GetRowLabel() == Input.GetLabel(InputSeqIndex1));
				asserta(PairMx->GetColLabel() == Input.GetLabel(InputSeqIndex2));
				}

			if (Transpose)
				{
				for (unsigned Pos2 = 1; Pos2 <= L2; ++Pos2)
					{
					const unsigned ColIndex2 = ColIndexes2[Pos2];

					float *Values;
					unsigned *ColIndexes;
					const unsigned EntryCount = PairMx->GetRow(Pos2, &Values, &ColIndexes);
					for (unsigned EntryIndex = 0; EntryIndex < EntryCount; ++EntryIndex)
						{
						unsigned Pos1 = ColIndexes[EntryIndex];
						float Value = Values[EntryIndex];
						assert(Pos1 < SIZE(ColIndexes1));
						unsigned ColIndex1 = ColIndexes1[Pos1];
						assert(ColIndex1 < g_MatchMx.m_RowCount);
						assert(ColIndex2 < g_MatchMx.m_ColCount);
						MatchMx[ColIndex1+1][ColIndex2+1] += w*Value;
						}
					}
				}
			else // !Transpose
				{
				for (unsigned Pos1 = 1; Pos1 <= L1; ++Pos1)
					{
					const unsigned ColIndex1 = ColIndexes1[Pos1];
					float *Values;
					unsigned *ColIndexes;
					const unsigned EntryCount = PairMx->GetRow(Pos1, &Values, &ColIndexes);
					for (unsigned EntryIndex = 0; EntryIndex < EntryCount; ++EntryIndex)
						{
						float Value = Values[EntryIndex];
						unsigned Pos2 = ColIndexes[EntryIndex];
						unsigned ColIndex2 = ColIndexes2[Pos2];
						assert(ColIndex1 < g_MatchMx.m_RowCount);
						assert(ColIndex2 < g_MatchMx.m_ColCount);
						MatchMx[ColIndex1+1][ColIndex2+1] += w*Value;
						}
					}
				}
			}
		}

#if	TRACE
	LogMatchMx(g_MatchMx, msa1, msa2);
#endif

	if (opt_cons == 0)
		{
		for (unsigned i = 0; i < SIZE(PairMxs); ++i)
			delete PairMxs[i];
		}

	return g_MatchMx;
	}

void AlignMSAsGivenSubPath(const SeqDB &msa1, const SeqDB &msa2, 
  unsigned StartCol1, unsigned StartCol2, const string &Path, bool Strand,
  SeqDB &OutMSA)
	{
	const unsigned ColCount = SIZE(Path);
	const unsigned ColCount1 = msa1.GetColCount();
	const unsigned ColCount2 = msa2.GetColCount();
	asserta(ColCount1 > StartCol1);
	asserta(ColCount2 > StartCol2);

	const unsigned SeqCount1 = msa1.GetSeqCount();
	const unsigned SeqCount2 = msa2.GetSeqCount();

	for (unsigned i = 0; i < SeqCount1; ++i)
		{
		const byte *Seq = msa1.GetSeq(i);
		unsigned FullLength = msa1.GetFullLength(i);
		byte *NewSeq = MakeGappedSeq(Seq+StartCol1, Path, true);
		const string &Label = msa1.GetLabel(i);
		float Weight = msa1.GetSeqWeight(i);
		unsigned InputSeqIndex = msa1.GetUser(i);
		bool Strand1 = msa1.GetStrand(i);
		unsigned Lo;
		if (Strand1)
			Lo = msa1.ColToPos(i, StartCol1);
		else
			{
			unsigned Ni, Nj;
			GetLetterCounts(Path, Ni, Nj);
			unsigned EndCol1 = StartCol1 + Ni - 1;
			asserta(EndCol1 < ColCount1);
			Lo = msa1.ColToPos(i, EndCol1);
			}
		OutMSA.AddSeq(Label, NewSeq, ColCount, Weight, InputSeqIndex,
		  Lo, FullLength, Strand1);
		}

	for (unsigned i = 0; i < SeqCount2; ++i)
		{
		unsigned Ni, Nj;
		GetLetterCounts(Path, Ni, Nj);
		unsigned EndCol2 = StartCol2 + Nj - 1;
		asserta(EndCol2 < ColCount2);

		byte *Seq = msa2.GetSeq(i);
		unsigned FullLength = msa2.GetFullLength(i);
		byte *NewSeq = MakeGappedSeq2(Seq, StartCol2, Path, false, Strand);

		const string &Label = msa2.GetLabel(i);
		float Weight = msa2.GetSeqWeight(i);
		unsigned InputSeqIndex = msa2.GetUser(i);
		unsigned Lo = UINT_MAX;
		bool Strand2 = msa2.GetStrand(i);
		if (Strand2)
			Lo = msa2.ColToPos(i, StartCol2);
		else
			Lo = msa2.ColToPos(i, EndCol2);
		if (!Strand)
			Strand2 = !Strand2;
		OutMSA.AddSeq(Label, NewSeq, ColCount, Weight, InputSeqIndex,
		  Lo, FullLength, Strand2);
		}
	}

void AlignMSAsGivenPath(const SeqDB &msa1, const SeqDB &msa2, const string &Path,
  SeqDB &OutMSA)
	{
	AlignMSAsGivenSubPath(msa1, msa2, 0, 0, Path, true, OutMSA);
	}

float AlignTwoMSAs(SeqDB &Input, const SeqDB &msa1, const SeqDB &msa2,
  SeqDB &OutMSA)
	{
	OutMSA.Clear();

	Mx<float> &MatchMx = ComputeMatchMx(Input, msa1, msa2);

	string Path;
	float Score = Viterbi(MatchMx, Path);

#if	TRACE
	Log("\n");
	Log("AlignTwoMSAs output:\n");
	OutMSA.LogMe();
#endif
	AlignMSAsGivenPath(msa1, msa2, Path, OutMSA);
	return Score;
	}
