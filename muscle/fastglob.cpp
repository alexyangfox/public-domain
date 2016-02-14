#include "myutils.h"
#include "seqdb.h"
#include "params.h"
#include <algorithm>

float GetFractId(const byte *Seq1, const byte *Seq2, const string &Path,
  unsigned Lo1, unsigned Lo2);

static Mx<float> g_DPMemf;

float FastEstimateFractId(SeqDB &DB, unsigned IdA, unsigned IdB)
	{
	float **SubstMx = GetSubstMx();

	const byte *A = DB.GetSeq(IdA);
	const byte *B = DB.GetSeq(IdB);
	const unsigned LA = DB.GetSeqLength(IdA);
	const unsigned LB = DB.GetSeqLength(IdB);

	g_DPMemf.Alloc("FastGlobal", LA+1, LB+1, &DB, IdA, IdB);
	float **M = g_DPMemf.GetData();

	M[0][0] = 0;
	for (unsigned i = 1; i <= LA; ++i)
		M[i][0] = 0;

	for (unsigned j = 1; j <= LB; ++j)
		M[0][j] = 0;

	for (unsigned i = 1; i <= LA; ++i)
		{
		byte a = A[i-1];
		const float *Mx_a = SubstMx[a];
		float *ptrMiRow = M[i];
		float *ptrMi_1 = M[i-1];
		const byte *ptrB = B;
		float *ptrMiRowEnd = M[i] + LB;
		for (;;)
			{
			byte b = *ptrB++;
			float MM = *ptrMi_1++ + Mx_a[b];
			float DM = *ptrMi_1;
			float IM = *ptrMiRow++;
			if (MM >= DM && MM >= IM)
				*ptrMiRow = MM;
			else if (DM >= MM && DM >= IM)
				*ptrMiRow = DM;
			else
				*ptrMiRow = IM;
			if (ptrMiRow == ptrMiRowEnd)
				break;
			}
		}

	float BestScore = LOG_ZERO;
	unsigned Besti = LA;
	unsigned Bestj = LB;

	for (unsigned i = 1; i <= LA; ++i)
		{
		if (M[i][LB] > BestScore)
			{
			BestScore = M[i][LB];
			Besti = i;
			Bestj = LB;
			}
		}

	for (unsigned j = 1; j <= LB; ++j)
		{
		if (M[LA][j] > BestScore)
			{
			BestScore = M[LA][j];
			Besti = LA;
			Bestj = j;
			}
		}

	unsigned i = Besti;
	unsigned j = Bestj;

	unsigned PairCount = 0;
	unsigned SameCount = 0;
	for (;;)
		{
		if (i == 0 || j == 0)
			break;
		byte a = A[i-1];
		byte b = B[j-1];
		float MM = M[i-1][j-1] + SubstMx[a][b];
		float DM = M[i-1][j];
		float IM = M[i][j-1];
		float XM = M[i][j];
		if (XM == MM)
			{
			++PairCount;
			if (toupper(a) == toupper(b))
				++SameCount;
			--i;
			--j;
			continue;
			}
		else if (XM == DM)
			{
			--i;
			continue;
			}
		else if (XM == IM)
			{
			--j;
			continue;
			}
	// Rounding error, find closest match
		float dM = fabs(XM - MM);
		float dD = fabs(XM - DM);
		float dI = fabs(XM - IM);
		if (dM <= dD && dM <= dI)
			{
			++PairCount;
			if (toupper(a) == toupper(b))
				++SameCount;
			--i;
			--j;
			continue;
			}
		else if (dD <= dM && dD <= dI)
			{
			--i;
			continue;
			}
		else
			{
			--j;
			continue;
			}
		}
	if (PairCount == 0)
		return 0.0f;
	return float(SameCount)/float(PairCount);
	}

void LogGlobalAln(SeqDB &DB, unsigned IdA, unsigned IdB, const string &Path)
	{
	const byte *A = DB.GetSeq(IdA);
	const byte *B = DB.GetSeq(IdB);

	Log("\n");
	unsigned i = 0;
	unsigned j = 0;
	for (unsigned k = 0; k < SIZE(Path); ++k)
		{
		char c = Path[k];
		if (c == 'M' || c == 'D')
			Log("%c", A[i++]);
		else
			Log("-");
		}
	Log("\n");
	for (unsigned k = 0; k < SIZE(Path); ++k)
		{
		char c = Path[k];
		if (c == 'M' || c == 'I')
			Log("%c", B[j++]);
		else
			Log("-");
		}
	Log("\n");
	}

void SeqDB::ComputeFastIdMx()
	{
	SetBLOSUM70C();

	const unsigned SeqCount = GetSeqCount();
	m_IdMxf.Alloc("IdMx", SeqCount, SeqCount);
	float **IdMx = m_IdMxf.GetData();
	for (unsigned SeqIndex1 = 0; SeqIndex1 < SeqCount; ++SeqIndex1)
		{
		IdMx[SeqIndex1][SeqIndex1] = 1.0f;
		for (unsigned SeqIndex2 = SeqIndex1 + 1; SeqIndex2 < SeqCount; ++SeqIndex2)
			{
			string Path;
			float Id = FastEstimateFractId(*this, SeqIndex1, SeqIndex2);
			IdMx[SeqIndex1][SeqIndex2] = Id;
			IdMx[SeqIndex2][SeqIndex1] = Id;
			}
		}
	}
