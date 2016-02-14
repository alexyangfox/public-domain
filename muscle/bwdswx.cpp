#include "myutils.h"
#include "sumlog.h"
#include "params.h"
#include "mx.h"
#include "seqdb.h"

#define		m(x)	static Mx<float> g_Bwd##x;
	m(M)
	m(D)
	m(I)
#undef m

void BwdSWXCRF()
	{
	Mx<float> &Simf = GetSimMxf();

	unsigned IdA = Simf.m_IdA;
	unsigned IdB = Simf.m_IdB;

	SeqDB &DB = *Simf.m_SeqDB;
	const float * const *SimMx = Simf.GetData();

	const unsigned LA = DB.GetSeqLength(IdA);
	const unsigned LB = DB.GetSeqLength(IdB);

	const int iLA = (int) LA;
	const int iLB = (int) LB;

#define		m(x)	g_Bwd##x.Alloc("SWXCRF_Bwd"#x, LA+1, LB+1, &DB, IdA, IdB);	 \
					float **Bwd##x = g_Bwd##x.GetData();
	m(M)
	m(D)
	m(I)
#undef m

	for (unsigned i = 0; i <= LA; ++i)
		for (unsigned  j = 0; j <= LB; ++j)
			{
			BwdM[i][j] = LOG_ZERO;
			BwdD[i][j] = LOG_ZERO;
			BwdI[i][j] = LOG_ZERO;
			}

	for (int i = (int) LA; i > 0; --i)
		for (unsigned j = LB-1; j <= LB; ++j)
			BwdM[i][j] = 0;

	for (int j = (int) LB; j > 0; --j)
		for (unsigned i = LA-1; i <= LA; ++i)
			BwdM[i][j] = 0;

	for (unsigned i = LA-1; i <= LA; ++i)
		for (unsigned j = LB-1; j <= LB; ++j)
			BwdM[i][j] = 0;

// Main loop
	for (int i = iLA-3; i > 0; --i)
		{
		const float *SimMxRow = SimMx[i+1];
		for (int j = iLB-3; j > 0; --j)
			{
			const float Match = SimMxRow[j+1];
			const float BMM = BwdM[i+3][j+3] + Match;
		// Mx
			{
			float MM = BMM;
			float MD = BwdD[i+3][j] + TransMD;
			float MI = BwdI[i][j+3] + TransMI;
			float ME = 0;
			BwdM[i][j] = SumLog4(MM, MD, MI, ME);
			}
			
		// Dx
			{
			float DM = BMM;
			float DD = BwdD[i+3][j] + TransDD;
			BwdD[i][j] = SumLog2(DM, DD);
			}
			
		// Ix
			{
			float IM = BMM;
			float II = BwdI[i][j+3];
			BwdI[i][j] = SumLog2(IM, II);
			}
			}
		}
	}
