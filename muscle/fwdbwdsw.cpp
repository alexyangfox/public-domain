#include "myutils.h"
#include "best.h"
#include "mx.h"
#include "sumlog.h"
#include "seqdb.h"
#include "params.h"

void FwdSWCRF();
void BwdSWCRF();

float FwdBwdSWCRF(Mx<float> &PPMx)
	{
	Mx<float> &Simf = GetSimMxf();

	const unsigned LA = Simf.m_RowCount - 1;
	const unsigned LB = Simf.m_ColCount - 1;

	FwdSWCRF();
	BwdSWCRF();

#define m(x)	float **x = MxBase::Getf("SWCRF_"#x);
	m(FwdM)
	m(BwdM)
#undef m

	PPMx.Alloc("SWCRF_PP", LA+1, LB+1, Simf.m_SeqDB, Simf.m_IdA, Simf.m_IdB);
	float **PP = PPMx.GetData();

	float TotalScore = LOG_ZERO;
	for (unsigned i = 1; i <= LA; ++i)
		for (unsigned j = 1; j <= LB; ++j)
			TotalScore = SumLog2(TotalScore, FwdM[i][j]);

	TotalScore = SumLog2(TotalScore, 0);

	for (unsigned i = 0; i <= LA; ++i)
		PP[i][0] = 0;
	for (unsigned j = 0; j <= LB; ++j)
		PP[0][j] = 0;

	for (unsigned i = 1; i <= LA; ++i)
		{
		for (unsigned j = 1; j <= LB; ++j)
			{
			float Score = FwdM[i][j] + BwdM[i][j];
			float LogProb = Score - TotalScore;
			float Prob = EXP(LogProb);
			//if (Prob < -0.1f || Prob > 1.1f)
			//	Warning("Prob=%g", Prob);

			PP[i][j] = Prob;
			}
		}

	return TotalScore;
	}
