#include "myutils.h"
#include "sumlog.h"
#include "params.h"
#include "mx.h"
#include "seqdb.h"

extern float *g_PCEmit;

#define		m(x)	static Mx<float> g_Fwd##x;
	m(M)
	m(D)
	m(I)
	m(X)
	m(Y)
#undef m

void FwdPCHMM(SeqDB &DB, unsigned IdA, unsigned IdB)
	{
	const byte *A = DB.GetSeq(IdA);
	const byte *B = DB.GetSeq(IdB);

	unsigned LA = DB.GetLength(IdA);
	unsigned LB = DB.GetLength(IdB);

#define		m(x)	g_Fwd##x.Alloc("PCHMM_Fwd"#x, LA+1, LB+1, &DB, IdA, IdB);	 \
					float **Fwd##x = g_Fwd##x.GetData();
	m(M)
	m(D)
	m(I)
	m(X)
	m(Y)
#undef m

	float **Mx = GetSubstMx();
	
// Init origin
	byte a;
	float *MxRow;
	byte b;
	
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
		a = (byte) A[i];
		MxRow = Mx[a];
		float Emit = g_PCEmit[a];
		s += Emit;
		x += Emit;
		if (i > 0)
			{
			s += TransDD;
			x += TransXX;
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
		b = (byte) B[j];
		float Emit = g_PCEmit[b];
		s += Emit;
		x += Emit;
		if (j > 0)
			{
			s += TransII;
			x += TransYY;
			}
		FwdM[0][j+1] = LOG_ZERO;
		FwdD[0][j+1] = LOG_ZERO;
		FwdX[0][j+1] = LOG_ZERO;
		FwdI[0][j+1] = s;
		FwdY[0][j+1] = x;
		}
	}
	
// Main loop
	float EdgeMM = TransSM;
	for (unsigned i = 0; i < LA; ++i)
		{
		byte a = A[i];
		float Emita = g_PCEmit[a];
		MxRow = Mx[a];
		for (unsigned j = 0; j < LB; ++j)
			{
			byte b = B[j];
			float Emitb = g_PCEmit[b];
			
		// xM
			{
			float Match = MxRow[b];
			float MM = FwdM[i][j] + EdgeMM;
			float DM = FwdD[i][j] + TransDM;
			float IM = FwdI[i][j] + TransIM;
			float XM = FwdX[i][j] + TransXM;
			float YM = FwdY[i][j] + TransYM;
			FwdM[i+1][j+1] = SumLog5(MM, DM, IM, XM, YM) + Match;
			EdgeMM = TransMM;
			}
			
		// xD
			{
			float MD = FwdM[i][j+1] + TransMD;
			float DD = FwdD[i][j+1] + TransDD;
			FwdD[i+1][j+1] = SumLog2(MD, DD) + Emita;
			}
			
		// xI
			{
			float MI = FwdM[i+1][j] + TransMI;
			float II = FwdI[i+1][j] + TransII;
			FwdI[i+1][j+1] = SumLog2(MI, II) + Emitb;
			}
			
		// xX
			{
			float MX = FwdM[i][j+1] + TransMX;
			float XX = FwdX[i][j+1] + TransXX;
			FwdX[i+1][j+1] = SumLog2(MX, XX) + Emita;
			}
			
		// xY
			{
			float MY = FwdM[i+1][j] + TransMY;
			float YY = FwdY[i+1][j] + TransYY;
			FwdY[i+1][j+1] = SumLog2(MY, YY) + Emitb;
			}
			}
		}
	}
