#include "myutils.h"
#include "seqdb.h"
#include "mx.h"
#include "params.h"

static Mx<float> g_SimMxf;

float **GetSimMx(SeqDB &DB, unsigned IdA, unsigned IdB)
	{
	const unsigned LA = DB.GetSeqLength(IdA);
	const unsigned LB = DB.GetSeqLength(IdB);
	if (g_SimMxf.m_SeqDB == &DB && g_SimMxf.m_IdA == IdA && g_SimMxf.m_IdB == IdB)
		return g_SimMxf.GetData();

	g_SimMxf.Alloc("Sim", LA+1, LB+1, &DB, IdA, IdB);

	float **Sim = g_SimMxf.GetData();
	float **SubstMx = GetSubstMx();

	const byte *A = DB.GetSeq(IdA);
	const byte *B = DB.GetSeq(IdB);

	for (unsigned i = 0; i <= LA; ++i)
		Sim[i][0] = 0.0f;

	for (unsigned j = 0; j <= LB; ++j)
		Sim[0][j] = 0.0f;

	for (unsigned i = 0; i < LA; ++i)
		{
		byte a = A[i];
		float *SubstRow = SubstMx[a];
		float *SimRow = Sim[i+1];
		for (unsigned j = 0; j < LB; ++j)
			{
			byte b = B[j];
			SimRow[j+1] = SubstRow[b];
			}
		}

	if (IdA == IdB)
		{
		for (unsigned i = 1; i <= LA; ++i)
			Sim[i][i] = 0;
		}

	return Sim;
	}
