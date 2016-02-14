#include "myutils.h"
#include "seqdb.h"
#include "mx.h"
#include "sparsemx.h"
#include "params.h"

float Viterbi(Mx<float> &PPMx, string &Path);
void SetSubstMx(const string &Model);
void FwdBwdXlat(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &PPMx);

static Mx<float> g_SimMxf;

static bool IsFirstState(char State)
	{
	return State == 'M' || State == 'D' || State == 'x' || State == 'X';
	}

static bool IsSecondState(char State)
	{
	return State == 'M' || State == 'I' || State == 'y' || State == 'Y';
	}

void GetLetterCounts(const string &Path, unsigned &i, unsigned &j)
	{
	i = 0;
	j = 0;
	const unsigned L = SIZE(Path);
	for (unsigned k = 0; k < L; ++k)
		{
		int c = Path[k];
		if (IsFirstState(c))
			++i;
		if (IsSecondState(c))
			++j;
		}
	}

void GetLocalModel(SeqDB &DB, string &Model)
	{
	SEQ_TYPE SeqType = DB.GuessSeqType();
	switch (SeqType)
		{
	case ST_Amino:
		Model = "localaff";
		return;
	case ST_DNA:
	case ST_RNA:
		Model = "localaffnuc";
		return;
		}
	asserta(false);
	}

void GetGlobalModel(SeqDB &DB, string &Model)
	{
	SEQ_TYPE SeqType = DB.GuessSeqType();
	switch (SeqType)
		{
	case ST_Amino:
		Model = "global";
		return;
	case ST_DNA:
	case ST_RNA:
		Model = "globalnuc";
		return;
		}
	asserta(false);
	}

Mx<float> &GetSimMxf()
	{
	return g_SimMxf;
	}

float **GetSimMx()
	{
	return g_SimMxf.GetData();
	}

void MaskSimMxSelf()
	{
	Mx<float> &Simf = GetSimMxf();
	SeqDB &DB = *Simf.m_SeqDB;
	unsigned Id = Simf.m_IdA;
	asserta(Simf.m_IdB == Id);
	float **SimMx = GetSimMx();

	const unsigned L = DB.GetSeqLength(Id);
	for (unsigned i = 0; i < L; ++i)
		for (unsigned j = 0; j <= i + opt_mmband && j < L; ++j)
			SimMx[i+1][j+1] = LOG_ZERO;
	}

static void MaskSimMxSelfOffset()
	{
	Mx<float> &Simf = GetSimMxf();
	SeqDB &DB = *Simf.m_SeqDB;

	unsigned IdA = Simf.m_IdA;
	unsigned IdB = Simf.m_IdB;

	unsigned LA = DB.GetSeqLength(IdA);
	unsigned LB = DB.GetSeqLength(IdB);

	unsigned LoA = DB.GetLo(IdA);
	unsigned LoB = DB.GetLo(IdB);

	const byte *A = DB.GetSeq(IdA);
	const byte *B = DB.GetSeq(IdB);

	float **SimMx = GetSimMx();

/***
I,J pos in original seq.
i,j pos in sub-seq.

	I = i + LoA
	J = j + LoB

Equivalent pos:
	I = J
	i + LoA = j + LoB
	j = i + LoA - LoB
***/

	for (unsigned i = 0; i < LA; ++i)
		{
		if (i + LoA < LoB)
			continue;

		unsigned j = i + LoA - LoB;
		if (j >= LB)
			continue;

		asserta(A[i] == B[j]);
		unsigned jlo = (j >= opt_mmband ? j - opt_mmband : 0);
		unsigned jhi = j + opt_mmband;
		if (jhi >= LB)
			jhi = LB - 1;
		for (unsigned j2 = jlo; j2 <= jhi; ++j2)
			SimMx[i+1][j2+1] = LOG_ZERO;
		}
	}

void SetSimMx(SeqDB &DB, unsigned IdA, unsigned IdB)
	{
	const unsigned LA = DB.GetSeqLength(IdA);
	const unsigned LB = DB.GetSeqLength(IdB);
	g_SimMxf.Alloc("Sim", LA+1, LB+1, &DB, IdA, IdB);
	float **Sim = g_SimMxf.GetData();

	const byte *A = DB.GetSeq(IdA);
	const byte *B = DB.GetSeq(IdB);
	float **SubstMx = GetSubstMx();

#if	DEBUG
	for (unsigned i = 0; i <= LA; ++i)
		Sim[i][0] = 0;
	for (unsigned j = 0; j <= LB; ++j)
		Sim[0][j] = 0;
#endif

	for (unsigned i = 0; i < LA; ++i)
		{
		byte a = A[i];
		const float *SubstMxRow = SubstMx[a];
		float *SimRow = Sim[i+1];
		for (unsigned j = 0; j < LB; ++j)
			{
			byte b = B[j];
			SimRow[j+1] = SubstMxRow[b];
			}
		}

// Special-cases for self-alignments
	if (IdA == IdB)
		MaskSimMxSelf();
	else if ((DB.GetLo(IdA) > 0 || DB.GetLo(IdB) > 0)
	  && DB.GetLabel(IdA) == DB.GetLabel(IdB))
		MaskSimMxSelfOffset();
	}

// Special case for self because it requires a local model.
void FwdBwdSelf(SeqDB &DB, unsigned Id, Mx<float> &PPMx)
	{
	string Model;
	GetLocalModel(DB, Model);
	FWD_BWD FB = SetModel(Model);
	SetSimMx(DB, Id, Id);
	MaskSimMxSelf();
	FB(PPMx);

// Zero out lower triangle. Can do this 2x faster by not
// computing it in the first place.
	float **PP = PPMx.GetData();
	const unsigned L = DB.GetSeqLength(Id);
	for (unsigned i = 0; i <= L; ++i)
		for (unsigned j = 0; j <= i + opt_mmband && j <= L; ++j)
			PP[i][j] = 0;
	}

//void FwdBwdGlobal(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &PPMx)
//	{
//	string Model;
//	GetGlobalModel(DB, Model);
//	FWD_BWD FB = SetModel(Model);
//	SetSimMx(DB, IdA, IdB);
//	FB(PPMx);
//	}

void FwdBwdLocal(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &PPMx)
	{
	string Model;
	GetLocalModel(DB, Model);
	FWD_BWD FB = SetModel(Model);
	SetSimMx(DB, IdA, IdB);
	FB(PPMx);
	}

void FwdBwdLocal(SeqDB &DB, unsigned IdA, unsigned IdB, SparseMx &SPPMx)
	{
	Mx<float> PPMx;
	FwdBwdLocal(DB, IdA, IdB, PPMx);
	SPPMx.FromMx(PPMx);
	}

void FwdBwd(Mx<float> &PPMx)
	{
	Mx<float> &Simf = GetSimMxf();
	unsigned LA = Simf.m_RowCount - 1;
	unsigned LB = Simf.m_ColCount - 1;

	if (opt_model.find('+') == string::npos)
		{
		FWD_BWD FB = SetModel(opt_model);
		FB(PPMx);
		}
	else
		{
		vector<string> Fields;
		Split(opt_model, Fields, '+');
		if (Fields.size() != 2)
			Die("Invalid dual model %s", opt_model.c_str());

		const string &Model1 = Fields[0];
		const string &Model2 = Fields[1];

		FWD_BWD FB1 = SetModel(Model1);
		Mx<float> PPMx1;
		float Score1 = FB1(PPMx1);

		FWD_BWD FB2 = SetModel(Model2);
		Mx<float> PPMx2;
		float Score2 = FB2(PPMx2);

		PPMx.Alloc("PP", LA+1, LB+1, Simf.m_SeqDB, Simf.m_IdA, Simf.m_IdB);

		float **PP1 = PPMx1.GetData();
		float **PP2 = PPMx2.GetData();
		float **PP = PPMx.GetData();

		if (opt_dualbest || opt_mixea || opt_sumlog)
			{
			string Path;
			float ExpectedAccuracy1 = Viterbi(PPMx1, Path);
			float ExpectedAccuracy2 = Viterbi(PPMx2, Path);

			if (opt_mixea)
				{
				float w1 = ExpectedAccuracy1/(ExpectedAccuracy1 + ExpectedAccuracy2);
				float w2 = ExpectedAccuracy2/(ExpectedAccuracy1 + ExpectedAccuracy2);
				for (unsigned i = 1; i <= LA; ++i)
					for (unsigned j = 1; j <= LB; ++j)
						PP[i][j] = float(w1*PP1[i][j] + w2*PP2[i][j]);
				}
			else if (opt_sumlog)
				{
				float TotalScore = SumLog2(Score1, Score2);
				float w1 = EXP(Score1 - TotalScore);
				float w2 = EXP(Score2 - TotalScore);
				for (unsigned i = 1; i <= LA; ++i)
					for (unsigned j = 1; j <= LB; ++j)
						PP[i][j] = float(w1*PP1[i][j] + w2*PP2[i][j]);
				}
			else
				{
				if (ExpectedAccuracy1 > ExpectedAccuracy2)
					PPMx.Copy(PPMx1, "PP");
				else
					PPMx.Copy(PPMx2, "PP");
				}
			}
		else
			{
			for (unsigned i = 1; i <= LA; ++i)
				for (unsigned j = 1; j <= LB; ++j)
					PP[i][j] = float(opt_mix*PP1[i][j] + (1-opt_mix)*PP2[i][j]);
			}
		}

	PPMx.Smooth(opt_smooth);
	float **PP = PPMx.GetData();
	for (unsigned i = 0; i <= LA; ++i)
		PP[i][0] = 0;
	for (unsigned j = 0; j <= LB; ++j)
		PP[0][j] = 0;
	}

void FwdBwd(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &PPMx)
	{
	if (opt_xlat)
		{
		FwdBwdXlat(DB, IdA, IdB, PPMx);
		return;
		}

	if (IdA == IdB)
		{
		FwdBwdSelf(DB, IdA, PPMx);
		return;
		}

	SetSubstMx(opt_model);

	SetSimMx(DB, IdA, IdB);
	FwdBwd(PPMx);
	}

void FwdBwd(SeqDB &DB, unsigned IdA, unsigned IdB, SparseMx &SPP)
	{
	Mx<float> PP;
	FwdBwd(DB, IdA, IdB, PP);
	SPP.FromMx(PP);
	}
