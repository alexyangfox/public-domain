#include "myutils.h"
#include "sumlog.h"
#include "params.h"
#include "mx.h"
#include "seqdb.h"

#define		m(x)	static Mx<float> g_Bwd##x;
	m(M)
	m(D)
	m(I)
	m(X)
	m(Y)
#undef m

void BwdFull()
	{
	Mx<float> &Simf = GetSimMxf();

	const unsigned LA = Simf.m_RowCount - 1;
	const unsigned LB = Simf.m_ColCount - 1;

#define		m(x)	g_Bwd##x.Alloc("Full_Bwd"#x, LA+1, LB+1, Simf.m_SeqDB, Simf.m_IdA, Simf.m_IdB);	 \
					float **Bwd##x = g_Bwd##x.GetData();
	m(M)
	m(D)
	m(I)
	m(X)
	m(Y)
#undef m

	const float * const *SimMx = Simf.GetData();	

// Init origin
	BwdM[LA][LB] = float (0) /*ME*/;
	BwdD[LA][LB] = float (0) /*DE*/;
	BwdI[LA][LB] = float (0) /*IE*/;
	BwdX[LA][LB] = float (0) /*XE*/;
	BwdY[LA][LB] = float (0) /*YE*/;
	
// Init zero'th row A
	{
	for (int i = (int) LA; i >= 0; --i)
		{
		BwdM[i][0] = LOG_ZERO;
		BwdD[i][0] = LOG_ZERO;
		BwdI[i][0] = LOG_ZERO;
		BwdX[i][0] = LOG_ZERO;
		BwdY[i][0] = LOG_ZERO;
		}
	
	float s = TransRII;
	float t = TransRYY;
	for (int i = (int) LA-1; i > 0; --i)
		{
		float s1 = BwdD[i+1][LB] + TransRMD;
		float s2 = BwdX[i+1][LB] + TransRMX;
		BwdM[i][LB] = SumLog2(s1, s2);
		
		BwdI[i][LB] = LOG_ZERO;
		BwdD[i][LB] = s;
		s += TransRDD;
		
		BwdY[i][LB] = LOG_ZERO;
		BwdX[i][LB] = t;
		t += TransRXX;
		}
	}
	
// Init zero'th row B
	{
	for (int j = (int) LB; j >= 0; --j)
		{
		BwdM[0][j] = LOG_ZERO;
		BwdD[0][j] = LOG_ZERO;
		BwdI[0][j] = LOG_ZERO;
		BwdX[0][j] = LOG_ZERO;
		BwdY[0][j] = LOG_ZERO;
		}
	
	float s = TransRDD;
	float t = TransRXX;
	for (int j = (int) LB-1; j > 0; --j)
		{
		float s1 = BwdI[LA][j+1] + TransRMI;
		float s2 = BwdY[LA][j+1] + TransRMY;
		BwdM[LA][j] = SumLog2(s1, s2);
		
		BwdD[LA][j] = LOG_ZERO;
		BwdI[LA][j] = s;
		s += TransRDD;
		
		BwdX[LA][j] = LOG_ZERO;
		BwdY[LA][j] = t;
		t += TransRXX;
		}
	}
	
	const float *SimMxRow;
	
// Main loop
	for (int i = (int) LA-1; i > 0; --i)
		{
		SimMxRow = SimMx[i+1];
		for (int j = (int) LB-1; j > 0; --j)
			{
			float Match = SimMxRow[j+1];
			
		// Mx
			{
			float MM = BwdM[i+1][j+1] + Match;
			float MD = BwdD[i+1][j] + TransMD;
			float MI = BwdI[i][j+1] + TransMI;
			float MX = BwdX[i+1][j] + TransMX;
			float MY = BwdY[i][j+1] + TransMY;
			BwdM[i][j] = SumLog5(MM, MD, MI, MX, MY);
			}
			
		// Dx
			{
			float DM = BwdM[i+1][j+1] + Match;
			float DD = BwdD[i+1][j] + TransDD;
			BwdD[i][j] = SumLog2(DM, DD);
			}
			
		// Ix
			{
			float IM = BwdM[i+1][j+1] + Match;
			float II = BwdI[i][j+1] + TransII;
			BwdI[i][j] = SumLog2(IM, II);
			}
			
		// Xx
			{
			float XM = BwdM[i+1][j+1] + Match;
			float XX = BwdX[i+1][j] + TransXX;
			BwdX[i][j] = SumLog2(XM, XX);
			}
			
		// Yx
			{
			float YM = BwdM[i+1][j+1] + Match;
			float YY = BwdY[i][j+1] + TransYY;
			BwdY[i][j] = SumLog2(YM, YY);
			}
			}
		}
	}
