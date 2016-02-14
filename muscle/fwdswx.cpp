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

void FwdSWXCRF()
	{
	Mx<float> &Simf = GetSimMxf();

	unsigned IdA = Simf.m_IdA;
	unsigned IdB = Simf.m_IdB;

	SeqDB &DB = *Simf.m_SeqDB;
	const float * const *SimMx = Simf.GetData();

	const unsigned LA = DB.GetSeqLength(IdA);
	const unsigned LB = DB.GetSeqLength(IdB);

#define		m(x)	g_Fwd##x.Alloc("SWXCRF_Fwd"#x, LA+1, LB+1, &DB, IdA, IdB);	 \
					float **Fwd##x = g_Fwd##x.GetData();
	m(M)
	m(D)
	m(I)
#undef m

	//for (unsigned j = 0; j <= LB; ++j)//@@
	//	for (unsigned i = 0; i <= LA; ++i)
	//		{
	//		FwdM[i][j] = LOG_ZERO;
	//		FwdD[i][j] = LOG_ZERO;
	//		FwdI[i][j] = LOG_ZERO;
	//		}

	for (unsigned i = 0; i <= LA; ++i)//@@
		for (unsigned j = 0; j <= LB; ++j)
			{
			FwdM[i][j] = LOG_ZERO;
			FwdD[i][j] = LOG_ZERO;
			FwdI[i][j] = LOG_ZERO;
			}

// Main loop
	for (unsigned i = 2; i < LA; ++i)
		{
		const float *SimMxRow = SimMx[i+1];
		for (unsigned j = 2; j < LB; ++j)
			{
		// xM
			{
			float Match = SimMxRow[j+1];
			float MM = FwdM[i-2][j-2];
			float DM = FwdD[i-2][j-2];
			float IM = FwdI[i-2][j-2];
			float SM = 0;
			FwdM[i+1][j+1] = SumLog4(MM, DM, IM, SM) + Match;
			}
			
		// xD
			{
			float MD = FwdM[i-2][j+1] + TransMD;
			float DD = FwdD[i-2][j+1] + TransDD;
			FwdD[i+1][j+1] = SumLog2(MD, DD);
			}
			
		// xI
			{
			float MI = j >= 2 ? FwdM[i+1][j-2] + TransMI : LOG_ZERO;
			float II = j >= 2 ? FwdI[i+1][j-2] + TransII : LOG_ZERO;
			FwdI[i+1][j+1] = SumLog2(MI, II);
			}
			}
		}
	}
