#include <algorithm>
#include "myutils.h"
#include "best.h"
#include "mx.h"
#include "sumlog.h"
#include "seqdb.h"

static Mx<float> g_SumPP;
static Mx<char> g_TB;

float PPViterbi(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &PPMx, string &Path)
	{
	Path.clear();

	const unsigned LA = DB.GetSeqLength(IdA);
	const unsigned LB = DB.GetSeqLength(IdB);

	float **PP = PPMx.GetData();

	g_SumPP.Alloc("SumPP", LA+1, LB+1, &DB, IdA, IdB);
	g_TB.Alloc("TB", LA+1, LB+1, &DB, IdA, IdB);
	float **SumPP = g_SumPP.GetData();
	char **TB = g_TB.GetData();

	for (unsigned i = 0; i <= LA; ++i)
		{
		SumPP[i][0] = 0;
		TB[i][0] = 'S';
		}

	for (unsigned j = 0; j <= LB; ++j)
		{
		SumPP[0][j] = 0;
		TB[0][j] = 'S';
		}

	if (opt_trace)
		{
		Log("\n");
		Log("  i    j         Pij     Si-1j-1       Sij-1       Si-1j        Best  TB\n");
		Log("===  ===  ==========  ==========  ==========  ==========  ==========  ==\n");
		}
	for (unsigned i = 1; i <= LA; ++i)
		{
		for (unsigned j = 1; j <= LB; ++j)
			{
			float p = PP[i][j];
			float M = p + SumPP[i-1][j-1];
			float D = SumPP[i-1][j];
			float I = SumPP[i][j-1];
			Best3(M, D, I, 'M', 'D', 'I', SumPP[i][j], TB[i][j]);
			if (opt_trace)
				Log("%3u  %3u  %10.3g  %10.3g  %10.3g  %10.3g  %10.3g  %2c\n",
				  i, j, p, SumPP[i-1][j-1], SumPP[i][j-1], SumPP[i-1][j], SumPP[i][j], TB[i][j]);
			}
		}

	float BestScore = 0;
	unsigned Besti = LA;
	unsigned Bestj = LB;
	for (unsigned i = 1; i <= LA; ++i)
		{
		if (SumPP[i][LB] > BestScore)
			{
			BestScore = SumPP[i][LB];
			Besti = i;
			Bestj = LB;
			}
		}

	for (unsigned j = 1; j <= LB; ++j)
		{
		if (SumPP[LA][j] > BestScore)
			{
			BestScore = SumPP[LA][j];
			Besti = LA;
			Bestj = j;
			}
		}

	float ExpectedAccuracy = BestScore/min(LA, LB);

	unsigned i = Besti;
	unsigned j = Bestj;
	for (unsigned k = i; k < LA; ++k)
		Path.push_back('D');

	for (unsigned k = j; k < LB; ++k)
		Path.push_back('I');

	for (;;)
		{
		char State = TB[i][j];
		if (opt_trace)
			Log("TB i=%u j=%u SumPP=%c\n", i, j, State);
		if (State == 'S')
			{
			asserta(i == 0 || j == 0);
			break;
			}
		Path.push_back(State);
		switch (State)
			{
		case 'M':
			--i;
			--j;
			break;
		case 'D':
			--i;
			break;
		case 'I':
			--j;
			break;
		default:
			Die("Illegal state 0x%02x=%c in Viterbi traceback",
			  (unsigned char) State, State);
			}
		}

	for (int k = (int) i; k > 0; --k)
		Path.push_back('D');

	for (int k = (int) j; k > 0; --k)
		Path.push_back('I');

	reverse(Path.begin(), Path.end());
	if (opt_trace)
		{
		Log("%s\n", Path.c_str());
		Log("Best score = %g LA=%u LB=%u min=%u\n", BestScore, LA, LB, min(LA, LB));
		Log("Expected accuracy = %g\n", ExpectedAccuracy);
		}
	return ExpectedAccuracy;
	}
