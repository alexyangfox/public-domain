#include "myutils.h"
#include "best.h"
#include "mx.h"
#include "sumlog.h"
#include "seqdb.h"

void FwdPCHMM(SeqDB &DB, unsigned IdA, unsigned IdB);
void BwdPCHMM(SeqDB &DB, unsigned IdA, unsigned IdB);

float FwdBwdPCHMM(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &PPMx)
	{
	const unsigned LA = DB.GetLength(IdA);
	const unsigned LB = DB.GetLength(IdB);

	FwdPCHMM(DB, IdA, IdB);
	BwdPCHMM(DB, IdA, IdB);

#define m(x)	float **x = MxBase::Getf("PCHMM_"#x);
	m(FwdM)
	m(FwdD)
	m(FwdI)
	m(FwdX)
	m(FwdY)
	m(BwdM)
	m(BwdD)
	m(BwdI)
	m(BwdX)
	m(BwdY)
#undef m

	PPMx.Alloc("PCHMM_PP", LA+1, LB+1, &DB, IdA, IdB);
	float **PP = PPMx.GetData();

	float TotalScore = SumLog5(
	  FwdM[LA][LB] + BwdM[LA][LB],
	  FwdD[LA][LB] + BwdD[LA][LB],
	  FwdI[LA][LB] + BwdI[LA][LB],
	  FwdX[LA][LB] + BwdX[LA][LB],
	  FwdY[LA][LB] + BwdY[LA][LB]);

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
		Log("PCHMM TotalScore=%g (%s,%s)\n", TotalScore, DB.GetLabel(IdA), DB.GetLabel(IdB));
		}

	return TotalScore;
	}
