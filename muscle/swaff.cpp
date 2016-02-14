#include "myutils.h"
#include "mx.h"
#include "best.h"
#include "seqdb.h"
#include "params.h"
#include "hit.h"
#include <algorithm>

void LogLocalAln(SeqDB &DB, unsigned IdA, unsigned IdB,
  unsigned Starti, unsigned Startj, const string &Path);

#define		m(x)	static Mx<float> g_Fwd##x; static Mx<char> g_TB##x;
	m(M)
	m(D)
	m(I)
#undef m

float SWAff(HitData &Hit)
	{
	Mx<float> &Simf = GetSimMxf();

	const float * const *SimMx = Simf.GetData();

	const unsigned LA = Simf.m_RowCount - 1;
	const unsigned LB = Simf.m_ColCount - 1;

#define		m(x)	g_Fwd##x.Alloc("SAff_Fwd"#x, LA+1, LB+1, Simf.m_SeqDB, Simf.m_IdA, Simf.m_IdB);	\
					g_TB##x.Alloc("SWAff_TB"#x, LA+1, LB+1, Simf.m_SeqDB, Simf.m_IdA, Simf.m_IdB);	\
					float **Fwd##x = g_Fwd##x.GetData();											\
					char **TB##x = g_TB##x.GetData();
	m(M)
	m(D)
	m(I)
#undef m

	for (unsigned i = 0; i <= LA; ++i)
		{
		FwdM[i][0] = 0;
		FwdD[i][0] = LOG_ZERO;
		FwdI[i][0] = LOG_ZERO;
		TBM[i][0] = 'S';
		TBD[i][0] = '?';
		TBI[i][0] = '?';
		}

	for (unsigned j = 0; j <= LB; ++j)
		{
		FwdM[0][j] = 0;
		FwdD[0][j] = LOG_ZERO;
		FwdI[0][j] = LOG_ZERO;
		TBM[0][j] = 'S';
		TBD[0][j] = '?';
		TBI[0][j] = '?';
		}

// Main loop
	float BestScore = LOG_ZERO;
	unsigned Besti = UINT_MAX;
	unsigned Bestj = UINT_MAX;
	for (unsigned i = 0; i < LA; ++i)
		{
		const float *SimMxRow = SimMx[i+1];
		for (unsigned j = 0; j < LB; ++j)
			{
		// xM
			{
			float Match = SimMxRow[j+1];
			float MM = FwdM[i][j] + Match;
			float DM = FwdD[i][j] + Match;
			float IM = FwdI[i][j] + Match;
			float SM = 0;
			float Score;
			Best4(MM, DM, IM, SM, 'M', 'D', 'I', 'S', Score, TBM[i+1][j+1]);
			FwdM[i+1][j+1] = Score;
			if (Score > BestScore)
				{
				BestScore = Score;
				Besti = i+1;
				Bestj = j+1;
				}
			}
			
		// xD
			{
			float MD = FwdM[i][j+1] + TransMD;
			float DD = FwdD[i][j+1] + TransDD;
			Best2(MD, DD, 'M', 'D', FwdD[i+1][j+1], TBD[i+1][j+1]);
			}
			
		// xI
			{
			float MI = FwdM[i+1][j] + TransMI;
			float II = FwdI[i+1][j] + TransII;
			Best2(MI, II, 'M', 'I', FwdI[i+1][j+1], TBI[i+1][j+1]);
			}
			}
		}

	if (opt_trace)
		{
		g_FwdM.LogMe();
		g_FwdD.LogMe();
		g_FwdI.LogMe();
		g_TBM.LogMe();
		g_TBD.LogMe();
		g_TBI.LogMe();
		}

	if (Besti == UINT_MAX)
		{
		Hit.LoA = 0;
		Hit.LoB = 0;
		Hit.HiA = 0;
		Hit.HiB = 0;
		Hit.Score = LOG_ZERO;
		Hit.User = UINT_MAX;
		Hit.Path.clear();
		return LOG_ZERO;
		}

	Hit.HiA = Besti - 1;
	Hit.HiB = Bestj - 1;

	unsigned i = Besti;
	unsigned j = Bestj;
	char State = 'M';
	if (opt_trace)
		{
		Log("BestScore=%g\n", BestScore);
		Log("Besti,j=%u,%u\n", Besti, Bestj);
		Log("    i      j  State\n");
		Log("-----  -----  -----\n");
		}
	Hit.Path.clear();
	for (;;)
		{
		switch (State)
			{
		case 'M':
			{
			State = TBM[i][j];
			--i;
			--j;
			break;
			}
		case 'D':
			{
			State = TBD[i][j];
			--i;
			break;
			}
		case 'I':
			{
			State = TBI[i][j];
			--j;
			break;
			}
		default:
			asserta(false);
			}

		if (opt_trace)
			Log("%5u  %5u  %5c\n", i, j, State);

		if (State == 'S')
			break;

		Hit.Path.push_back(State);
		}

	Hit.LoA = i+1;
	Hit.LoB = j+1;
	reverse(Hit.Path.begin(), Hit.Path.end());
	Hit.LogMe(true);
	return BestScore;
	}

void SWAff(SeqDB &DB)
	{
	string Model;
	GetLocalModel(DB, Model);
	SetModel(Model);

	const unsigned SeqCount = DB.GetSeqCount();
	for (unsigned IdA = 0; IdA < SeqCount; ++IdA)
		{
		for (unsigned IdB = IdA + 1; IdB < SeqCount; ++IdB)
			{
			SetSimMx(DB, IdA, IdB);

			HitData Hit;
			SWAff(Hit);
			LogLocalAln(DB, IdA, IdB, Hit.LoA, Hit.LoB, Hit.Path);

			DB.RevComp(IdB);

			SetSimMx(DB, IdA, IdB);

			SWAff(Hit);
			LogLocalAln(DB, IdA, IdB, Hit.LoA, Hit.LoB, Hit.Path);

			DB.RevComp(IdB);
			}
		}
	}
