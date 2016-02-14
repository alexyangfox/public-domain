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

void BwdSWCRF()
	{
	Mx<float> &Simf = GetSimMxf();

	const float * const *SimMx = Simf.GetData();

	const unsigned LA = Simf.m_RowCount - 1;
	const unsigned LB = Simf.m_ColCount - 1;

#define		m(x)	g_Bwd##x.Alloc("SWCRF_Bwd"#x, LA+1, LB+1, Simf.m_SeqDB, Simf.m_IdA, Simf.m_IdB);	 \
					float **Bwd##x = g_Bwd##x.GetData();
	m(M)
	m(D)
	m(I)
#undef m

	for (int i = (int) LA; i >= 0; --i)
		{
		BwdM[i][0] = LOG_ZERO;
		BwdD[i][0] = LOG_ZERO;
		BwdI[i][0] = LOG_ZERO;
		}

	for (int j = (int) LB; j >= 0; --j)
		{
		BwdM[0][j] = LOG_ZERO;
		BwdD[0][j] = LOG_ZERO;
		BwdI[0][j] = LOG_ZERO;
		}
	
	for (int i = (int) LA; i > 0; --i)
		{
		BwdM[i][LB] = 0;
		BwdI[i][LB] = LOG_ZERO;
		BwdD[i][LB] = LOG_ZERO;
		}

	for (int j = (int) LB; j > 0; --j)
		{
		BwdM[LA][j] = 0;
		BwdD[LA][j] = LOG_ZERO;
		BwdI[LA][j] = LOG_ZERO;
		}

	BwdM[LA][LB] = 0;
	BwdD[LA][LB] = LOG_ZERO;
	BwdI[LA][LB] = LOG_ZERO;
	
// Main loop
	for (int i = (int) LA-1; i > 0; --i)
		{
		const float *SimMxRow = SimMx[i+1];
		for (int j = (int) LB-1; j > 0; --j)
			{
			float Match = SimMxRow[j+1];
			const float BMM = BwdM[i+1][j+1] + Match;
		// Mx
			{
			float MM = BMM;
			float MD = BwdD[i+1][j] + TransMD;
			float MI = BwdI[i][j+1] + TransMI;
			float ME = 0;
			BwdM[i][j] = SumLog4(MM, MD, MI, ME);
			}
			
		// Dx
			{
			float DM = BMM;
			float DD = BwdD[i+1][j] + TransDD;
			BwdD[i][j] = SumLog2(DM, DD);
			}
			
		// Ix
			{
			float IM = BMM;
			float II = BwdI[i][j+1] + TransII;
			BwdI[i][j] = SumLog2(IM, II);
			}
			}
		}
	}
