#include "myutils.h"
#include "seqdb.h"
#include "params.h"
#include "mx.h"

float FwdBwdPCCRF(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &PPMx);
float FwdBwdFull(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &PPMx);

static void Cvt()
	{
	extern float *g_PCEmit;

	float **SubstMx = GetSubstMx();
	for (unsigned i = 0; i < 256; ++i)
		for (unsigned j = 0; j < 256; ++j)
			SubstMx[i][j] -= (g_PCEmit[i] + g_PCEmit[j]);

	for (unsigned i = 0; i < 256; ++i)
		g_PCEmit[i] = 0;

	TransSM -= TransMM;
	TransDM -= TransMM;
	TransIM -= TransMM;
	TransXM -= TransMM;
	TransYM -= TransMM;

	TransSD -= TransMM/2;
	TransSI -= TransMM/2;
	TransSX -= TransMM/2;
	TransSY -= TransMM/2;

	TransMD -= TransMM/2;
	TransMX -= TransMM/2;
	TransSD -= TransMM/2;
	TransSI -= TransMM/2;
	TransSX -= TransMM/2;
	TransSY -= TransMM/2;
	TransDD -= TransMM/2;
	TransXX -= TransMM/2;

	TransME = TransSM;
	TransDE = TransSD;
	TransIE = TransSI;
	TransXE = TransSX;
	TransYE = TransSY;

	TransMM = 0;

	TransSD -= TransSM;
	TransSI -= TransSM;
	TransSX -= TransSM;
	TransSY -= TransSM;
	TransSM = 0;

	TransDE -= TransME;
	TransIE -= TransME;
	TransXE -= TransME;
	TransYE -= TransME;
	TransME = 0;

	TransMD += TransDM;
	TransMI += TransDM;
	TransSD += TransDM;
	TransSI += TransDM;
	TransDE -= TransDM;
	TransIE -= TransDM;
	TransDM = 0;
	TransIM = 0;

	TransMX += TransXM;
	TransMY += TransXM;
	TransSX += TransXM;
	TransSY += TransXM;
	TransXE -= TransXM;
	TransYE -= TransXM;
	TransXM = 0;
	TransYM = 0;

	LogParams();
	}

void PC2CRF(SeqDB &DB)
	{
	Mx<float> PP1;
	Mx<float> PP2;
	Mx<float> PP3;

	SetModelProbconsHMM();
	FwdBwdPCCRF(DB, 0, 1, PP1);

	Cvt();
	FwdBwdPCCRF(DB, 0, 1, PP2);

	SetModelProbconsCRF();
	FwdBwdFull(DB, 0, 1, PP3);

	PP1.LogMe();
	PP2.LogMe();
	PP3.LogMe();
	assert(PP1.Eq(PP2));
	assert(PP1.Eq(PP3));
	}
