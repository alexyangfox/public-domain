#include "myutils.h"
#include "sumlog.h"
#include "params.h"
#include "mx.h"
#include "seqdb.h"

#define		m(x)	static Mx<float> g_Fwd##x;
	m(M)
	m(D)
	m(I)
#undef m

void FwdSWCRF()
	{
	Mx<float> &Simf = GetSimMxf();

	const float * const *SimMx = Simf.GetData();

	const unsigned LA = Simf.m_RowCount - 1;
	const unsigned LB = Simf.m_ColCount - 1;

#define		m(x)	g_Fwd##x.Alloc("SWCRF_Fwd"#x, LA+1, LB+1, Simf.m_SeqDB, Simf.m_IdA, Simf.m_IdB);	 \
					float **Fwd##x = g_Fwd##x.GetData();
	m(M)
	m(D)
	m(I)
#undef m

	for (unsigned i = 0; i <= LA; ++i)
		{
		FwdM[i][0] = LOG_ZERO;
		FwdD[i][0] = LOG_ZERO;
		FwdI[i][0] = LOG_ZERO;
		}

	for (unsigned j = 0; j <= LB; ++j)
		{
		FwdM[0][j] = LOG_ZERO;
		FwdD[0][j] = LOG_ZERO;
		FwdI[0][j] = LOG_ZERO;
		}

// Main loop
	for (unsigned i = 0; i < LA; ++i)
		{
		const float *SimMxRow = SimMx[i+1];
		for (unsigned j = 0; j < LB; ++j)
			{
		// xM
			{
			float Match = SimMxRow[j+1];
			float MM = FwdM[i][j];
			float DM = FwdD[i][j];
			float IM = FwdI[i][j];
			float SM = 0;
			FwdM[i+1][j+1] = SumLog4(MM, DM, IM, SM) + Match;
			}
			
		// xD
			{
			float MD = FwdM[i][j+1] + TransMD;
			float DD = FwdD[i][j+1] + TransDD;
			FwdD[i+1][j+1] = SumLog2(MD, DD);
			}
			
		// xI
			{
			float MI = FwdM[i+1][j] + TransMI;
			float II = FwdI[i+1][j] + TransII;
			FwdI[i+1][j+1] = SumLog2(MI, II);
			}
			}
		}
	}
