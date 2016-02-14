#include "myutils.h"
#include "mx.h"
#include "params.h"
#include "mx.h"
#include "best.h"
#include <algorithm>

void LogRepeatAln(const byte *A, const byte *B, unsigned /*LA*/, unsigned LB,
  const string &Path);

/***
Repeated match of motif B to sequence A.

M[i][j] = score of best match ending at A[i-1] and B[j-1] (the two
last letters are not necessarily aligned).

M[i][0] = score of best match of A[i-1] to subsequences of B
assuming that A[i-1] does not match.
***/

float RepeatDP(const byte *A, const byte *B, unsigned LA, unsigned LB, float e,
  float t, string &Path)
	{
#if	0
	Path.clear();

	float **SubstMx = GetSubstMx();

	Mx<float> Mf;
	Mx<unsigned> TBf;

	Mf.Alloc("RepeatM", LA+1, LB+1);
	TBf.Alloc("RepeatTB", LA+1, LB+1);

	Mf.Init(0);
	TBf.Init(UINT_MAX);

	float **M = Mf.GetData();
	char **TB = TBf.GetData();

	for (unsigned i = 1; i <= LA; ++i)
		{
		byte s = A[i-1];

		float NoNewMatch = M[i-1][0];
		float BestScore = 0;
		unsigned Prev = UINT_MAX;
		for (unsigned j = 1; j <= LB; ++j)
			{
			float Score = M[i-1][j] - t;
			if (Score > BestScore)
				{
				BestScore = Score;
				Prev = j;
				}
			}
		Best3(MM, DM, IM, 'M', 'D', 'I', M[i][0], TB[i][0]);
		for (unsigned j = 1; j <= LB; ++j)
			{
			byte m = B[j-1];
			float MM = M[i-1][j-1] + SubstMx[s][m];
			float DM = M[i-1][j] + e;
			float IM = M[i][j-1] + e;
			Best3(MM, DM, IM, 'M', 'D', 'I', M[i][j], TB[i][j]);
			}
		}
	float Score = M[LA][LB];

	if (opt_trace)
		{
		Mf.LogMe();
		TBf.LogMe();
		}

	unsigned i = LA;
	unsigned j = LB;

	for (;;)
		{
		if (i == 0)
			break;
		char c = TB[i][j];
		Path.push_back(c);
		switch (c)
			{
		case 'M':
			--i;
			if (j == 1)
				j = LB;
			else
				--j;
			break;
		case 'D':
			--i;
			break;
		case 'I':
			if (j == 1)
				j = LB;
			else
				--j;
			break;
		default:
			asserta(false);
			}
		}

	reverse(Path.begin(), Path.end());
#if	DEBUG
	{
	void GetLetterCounts(const string &Path, unsigned &i, unsigned &j);
	unsigned Ni;
	unsigned Nj;
	GetLetterCounts(Path, Ni, Nj);
	asserta(Ni == LA);
	}
#endif

	//if (opt_trace)
	//	LogRepeatAln(A, B, LA, LB, Path);
	Mf.Free();
	TBf.Free();

	return Score;
#endif
	return 0;
	}

void TestRepeatDP()
	{
	const char *A = "LEFTTERMINALMQTIFINSERTMQTIGRIGHT";
	const char *B = "MQTIF";
	const unsigned LA = (unsigned) strlen(A);
	const unsigned LB = (unsigned) strlen(B);

	opt_trace = true;
	string Path;
//	RepeatDP((const byte *) A, (const byte *) B, LA, LB, -0.5f, Path);
	}
