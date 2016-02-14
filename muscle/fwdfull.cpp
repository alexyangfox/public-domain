#include "myutils.h"
#include "sumlog.h"
#include "params.h"
#include "mx.h"
#include "seqdb.h"

#define		m(x)	static Mx<float> g_Fwd##x;
	m(M)
	m(D)
	m(I)
	m(X)
	m(Y)
#undef m

void FwdFull()
	{
	Mx<float> &Simf = GetSimMxf();

	const unsigned LA = Simf.m_RowCount - 1;
	const unsigned LB = Simf.m_ColCount - 1;

#define		m(x)	g_Fwd##x.Alloc("Full_Fwd"#x, LA+1, LB+1, Simf.m_SeqDB, Simf.m_IdA, Simf.m_IdB);	 \
					float **Fwd##x = g_Fwd##x.GetData();
	m(M)
	m(D)
	m(I)
	m(X)
	m(Y)
#undef m

	const float * const *SimMx = Simf.GetData();
	const float *SimMxRow;
	
	FwdM[0][0] = float (0);
	FwdD[0][0] = LOG_ZERO;
	FwdI[0][0] = LOG_ZERO;
	FwdX[0][0] = LOG_ZERO;
	FwdY[0][0] = LOG_ZERO;
	
// Init zero'th row A
	{
	float s = TransSD;
	float x = TransSX;
	for (unsigned i = 0; i < LA; ++i)
		{
		if (i > 0)
			{
			s += TransLDD;
			x += TransLXX;
			}
		FwdM[i+1][0] = LOG_ZERO;
		FwdD[i+1][0] = s;
		FwdX[i+1][0] = x;
		FwdI[i+1][0] = LOG_ZERO;
		FwdY[i+1][0] = LOG_ZERO;
		}
	}
	
// Init zero'th row B
	{
	float s = TransSI;
	float x = TransSY;
	for (unsigned j = 0; j < LB; ++j)
		{
		if (j > 0)
			{
			s += TransLII;
			x += TransLYY;
			}
		FwdM[0][j+1] = LOG_ZERO;
		FwdD[0][j+1] = LOG_ZERO;
		FwdX[0][j+1] = LOG_ZERO;
		FwdI[0][j+1] = s;
		FwdY[0][j+1] = x;
		}
	}
	
// Main loop
	for (unsigned i = 0; i < LA; ++i)
		{
		SimMxRow = SimMx[i+1];
		const bool RightA = (i == LA-1);
		for (unsigned j = 0; j < LB; ++j)
			{
			const bool RightB = (j == LB-1);
			
		// xM
			{
			float Match = SimMxRow[j+1];
			float MM = FwdM[i][j];
			float DM = FwdD[i][j];
			float IM = FwdI[i][j];
			float XM = FwdX[i][j];
			float YM = FwdY[i][j];
			FwdM[i+1][j+1] = SumLog5(MM, DM, IM, XM, YM) + Match;
			}
			
		// xD
			{
			float EdgeMD = (RightB ? TransRMD : TransMD);
			float EdgeDD = (RightB ? TransRDD : TransDD);
			float MD = FwdM[i][j+1] + EdgeMD;
			float DD = FwdD[i][j+1] + EdgeDD;
			FwdD[i+1][j+1] = SumLog2(MD, DD);
			}
			
		// xI
			{
			float EdgeMI = (RightA ? TransRMI : TransMI);
			float EdgeII = (RightA ? TransRII : TransII);
			float MI = FwdM[i+1][j] + EdgeMI;
			float II = FwdI[i+1][j] + EdgeII;
			FwdI[i+1][j+1] = SumLog2(MI, II);
			}
			
		// xX
			{
			float EdgeMX = (RightB ? TransRMX : TransMX);
			float EdgeXX = (RightB ? TransRXX : TransXX);
			float MX = FwdM[i][j+1] + EdgeMX;
			float XX = FwdX[i][j+1] + EdgeXX;
			FwdX[i+1][j+1] = SumLog2(MX, XX);
			}
			
		// xY
			{
			float EdgeMY = (RightA ? TransRMY : TransMY);
			float EdgeYY = (RightA ? TransRYY : TransYY);
			float MY = FwdM[i+1][j] + EdgeMY;
			float YY = FwdY[i+1][j] + EdgeYY;
			FwdY[i+1][j+1] = SumLog2(MY, YY);
			}
			}
		}
	}
