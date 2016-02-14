#include "myutils.h"
#include "best.h"
#include "mx.h"
#include "sumlog.h"
#include "seqdb.h"
#include "params.h"

void FwdSWXCRF();
void BwdSWXCRF();

float FwdBwdSWXCRF(Mx<float> &PPMx)
	{
	Mx<float> &Simf = GetSimMxf();

	unsigned IdA = Simf.m_IdA;
	unsigned IdB = Simf.m_IdB;

	SeqDB &DB = *Simf.m_SeqDB;

	const unsigned LA = DB.GetSeqLength(IdA);
	const unsigned LB = DB.GetSeqLength(IdB);

	FwdSWXCRF();
	BwdSWXCRF();

#define m(x)	float **x = MxBase::Getf("SWXCRF_"#x);
	m(FwdM)
	m(BwdM)
#undef m

	PPMx.Alloc("SWXCRF_PP", LA+1, LB+1, &DB, IdA, IdB);
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
			if (Prob < -0.1f || Prob > 1.1f)
				Warning("Prob=%g", Prob);

			PP[i][j] = Prob;
			}
		}

	if (opt_trace)
		{
		MxBase::LogAll();
		Log("SWCRFX TotalScore=%g (%s,%s)\n",
		  TotalScore, DB.GetLabel(IdA).c_str(), DB.GetLabel(IdB).c_str());
		}

	return TotalScore;
	}
