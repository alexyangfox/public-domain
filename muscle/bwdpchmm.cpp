#include "myutils.h"
#include "sumlog.h"
#include "params.h"
#include "mx.h"
#include "seqdb.h"

extern float *g_PCEmit;

#define		m(x)	static Mx<float> g_Bwd##x;
	m(M)
	m(D)
	m(I)
	m(X)
	m(Y)
#undef m

void BwdPCHMM(SeqDB &DB, unsigned IdA, unsigned IdB)
	{
	const byte *A = DB.GetSeq(IdA);
	const byte *B = DB.GetSeq(IdB);

	unsigned LA = DB.GetLength(IdA);
	unsigned LB = DB.GetLength(IdB);

#define		m(x)	g_Bwd##x.Alloc("PCHMM_Bwd"#x, LA+1, LB+1, &DB, IdA, IdB);	 \
					float **Bwd##x = g_Bwd##x.GetData();
	m(M)
	m(D)
	m(I)
	m(X)
	m(Y)
#undef m

	float **Mx = GetSubstMx();
	
// Init origin
	BwdM[LA][LB] = TransME;
	BwdD[LA][LB] = TransDE;
	BwdI[LA][LB] = TransIE;
	BwdX[LA][LB] = TransXE;
	BwdY[LA][LB] = TransYE;
	
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
	
	float s = TransDD + TransDE;
	float t = TransXX + TransXE;
	for (int i = (int) LA-1; i > 0; --i)
		{
		byte a;
		float *MxRow;
		a = (byte) A[i];
		MxRow = Mx[a];
		float Emit = g_PCEmit[a];
		s += Emit;
		t += Emit;
		float s1 = BwdD[i+1][LB] + TransMD + Emit;
		float s2 = BwdX[i+1][LB] + TransMX + Emit;
		BwdM[i][LB] = SumLog2(s1, s2);
		
		BwdI[i][LB] = LOG_ZERO;
		BwdD[i][LB] = s;
		s += TransDD;
		
		BwdY[i][LB] = LOG_ZERO;
		BwdX[i][LB] = t;
		t += TransXX;
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
	
	float s = TransII + TransIE;
	float t = TransYY + TransYE;
	for (int j = (int) LB-1; j > 0; --j)
		{
		byte b;
		b = (byte) B[j];
		float Emit = g_PCEmit[b];
		s += Emit;
		t += Emit;
		float s1 = BwdI[LA][j+1] + TransMD + Emit;
		float s2 = BwdY[LA][j+1] + TransMX + Emit;
		BwdM[LA][j] = SumLog2(s1, s2);
		
		BwdD[LA][j] = LOG_ZERO;
		BwdI[LA][j] = s;
		s += TransII;
		
		BwdX[LA][j] = LOG_ZERO;
		BwdY[LA][j] = t;
		t += TransYY;
		}
	}
	
	byte a;
	float *MxRow;
	byte b;
	
// Main loop
	for (int i = (int) LA-1; i > 0; --i)
		{
		a = (byte) A[i];
		const float EmitA = g_PCEmit[a];
		MxRow = Mx[a];
		for (int j = (int) LB-1; j > 0; --j)
			{
			b = (byte) B[j];
			const float Match = MxRow[b];
			const float EmitB = g_PCEmit[b];
			
			const float BMM = BwdM[i+1][j+1] + Match;
		// Mx
			{
			float MM = TransMM + BMM;
			float MD = BwdD[i+1][j] + TransMD + EmitA;
			float MI = BwdI[i][j+1] + TransMI + EmitB;
			float MX = BwdX[i+1][j] + TransMX + EmitA;
			float MY = BwdY[i][j+1] + TransMY + EmitB;
			BwdM[i][j] = SumLog5(MM, MD, MI, MX, MY);
			}
			
		// Dx
			{
			float DM = TransDM + BMM;
			float DD = BwdD[i+1][j] + TransDD + EmitA;
			BwdD[i][j] = SumLog2(DM, DD);
			}
			
		// Ix
			{
			float IM = TransIM + BMM;
			float II = BwdI[i][j+1] + TransII + EmitB;
			BwdI[i][j] = SumLog2(IM, II);
			}
			
		// Xx
			{
			float XM = TransXM + BMM;
			float XX = BwdX[i+1][j] + TransXX + EmitA;
			BwdX[i][j] = SumLog2(XM, XX);
			}
			
		// Yx
			{
			float YM = TransYM + BMM;
			float YY = BwdY[i][j+1] + TransYY + EmitB;
			BwdY[i][j] = SumLog2(YM, YY);
			}
			}
		}
	}
