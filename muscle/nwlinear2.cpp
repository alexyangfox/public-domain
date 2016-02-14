#include <algorithm>
#include "myutils.h"
#include "sumlog.h"
#include "mx.h"

static Mx<float> g_M;

float NWLinear2(Mx<float> &MatchMxf, unsigned LoA, unsigned HiA,
  unsigned LoB, unsigned HiB, string &Path)
	{
	float **MatchMx = MatchMxf.GetData();

	unsigned LA = HiA - LoA + 1;
	unsigned LB = HiB - LoB + 1;
	g_M.Alloc("NWLinear2M", LA+1, LB+1);
	float **M = g_M.GetData();

	M[0][0] = 0;
	for (unsigned i = 1; i <= LA; ++i)
		M[i][0] = 0;

	for (unsigned j = 1; j <= LB; ++j)
		M[0][j] = 0;

	for (unsigned i = 1; i <= LA; ++i)
		{
		float *ptrMiRow = M[i];
		float *ptrMi_1 = M[i-1];
		const float *MatchRow = MatchMx[LoA+i-1];
		for (unsigned j = 1; j <= LB; ++j)
			{
			float MM = *ptrMi_1++ + MatchRow[LoB+j-1];
			float DM = *ptrMi_1;
			float IM = *ptrMiRow++;
			if (MM >= DM && MM >= IM)
				*ptrMiRow = MM;
			else if (DM >= MM && DM >= IM)
				*ptrMiRow = DM;
			else
				*ptrMiRow = IM;
			}
		}

	if (opt_trace)//done
		{
		Log("\n");
		Log("S=\n");
		g_M.LogMe();
		}

	float BestScore = LOG_ZERO;
	unsigned Besti = LA;
	unsigned Bestj = LB;

	Besti = LA;
	Bestj = LB;
	for (unsigned i = 1; i <= LA; ++i)
		{
		if (M[i][LB] > BestScore)
			{
			BestScore = M[i][LB];
			Besti = i;
			Bestj = LB;
			}
		}
	for (unsigned j = 1; j <= LB; ++j)
		{
		if (M[LA][j] > BestScore)
			{
			BestScore = M[LA][j];
			Besti = LA;
			Bestj = j;
			}
		}

	unsigned i = Besti;
	unsigned j = Bestj;
	for (unsigned k = i; k < LA; ++k)
		Path.push_back('D');

	for (unsigned k = j; k < LB; ++k)
		Path.push_back('I');

	for (;;)
		{
		if (i == 0 || j == 0)
			break;
		float MM = M[i-1][j-1] + MatchMx[LoA+i-1][LoB+j-1];
		float DM = M[i-1][j];
		float IM = M[i][j-1];
		float XM = M[i][j];
		if (XM == MM)
			{
			Path.push_back('M');
			--i;
			--j;
			continue;
			}
		else if (XM == DM)
			{
			Path.push_back('D');
			--i;
			continue;
			}
		else if (XM == IM)
			{
			Path.push_back('I');
			--j;
			continue;
			}
	// Rounding error, find closest match
		double dM = fabs(XM - MM);
		double dD = fabs(XM - DM);
		double dI = fabs(XM - IM);
		if (dM <= dD && dM <= dI)
			{
			Path.push_back('M');
			--i;
			--j;
			continue;
			}
		else if (dD <= dM && dD <= dI)
			{
			Path.push_back('D');
			--i;
			continue;
			}
		else
			{
			Path.push_back('I');
			--j;
			continue;
			}
		}

	for (int k = (int) i; k > 0; --k)
		Path.push_back('D');

	for (int k = (int) j; k > 0; --k)
		Path.push_back('I');

	reverse(Path.begin(), Path.end());
	if (opt_trace)
		Log("%s\n", Path.c_str());

	return BestScore;
	}
