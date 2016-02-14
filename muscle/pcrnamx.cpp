#include "myutils.h"

//namespace PROBCONS_RNA { // to avoid global name pollution
/////////////////////////////////////////////////////////////////
// Defaults.h (from PROBCONS RNA version 1.1)

float initDistrib1Default[] = { 0.9588437676f, 0.0205782652f, 0.0205782652f };
float gapOpen1Default[] = { 0.0190259293f, 0.0190259293f };
float gapExtend1Default[] = { 0.3269913495f, 0.3269913495f };

float initDistrib2Default[] = { 0.9615409374f, 0.0000004538f, 0.0000004538f, 0.0192291681f, 0.0192291681f };
float gapOpen2Default[] = { 0.0082473317f, 0.0082473317f, 0.0107844425f, 0.0107844425f };
float gapExtend2Default[] = { 0.3210460842f, 0.3210460842f, 0.3298229277f, 0.3298229277f };

string alphabetDefault = "ACGUTN";
float emitSingleDefault[6] = {
  0.2270790040f, 0.2422080040f, 0.2839320004f, 0.2464679927f, 0.2464679927f, 0.0003124650f 
};

float emitPairsDefault[6][6] = {
  { 0.1487240046f, 0.0184142999f, 0.0361397006f, 0.0238473993f, 0.0238473993f, 0.0000375308f },
  { 0.0184142999f, 0.1583919972f, 0.0275536999f, 0.0389291011f, 0.0389291011f, 0.0000815823f },
  { 0.0361397006f, 0.0275536999f, 0.1979320049f, 0.0244289003f, 0.0244289003f, 0.0000824765f },
  { 0.0238473993f, 0.0389291011f, 0.0244289003f, 0.1557479948f, 0.1557479948f, 0.0000743985f },
  { 0.0238473993f, 0.0389291011f, 0.0244289003f, 0.1557479948f, 0.1557479948f, 0.0000743985f },
  { 0.0000375308f, 0.0000815823f, 0.0000824765f, 0.0000743985f, 0.0000743985f, 0.0000263252f }
};

static int cvt(char c)
	{
	for (int i = 0; i < 6; ++i)
		if (alphabetDefault[i] == c)
			return i;
	return -1;
	}

void LogProbconsRNAMatrix()
	{
	const unsigned N = 6;

	Log("//\t     ");
	for (unsigned i = 0; i < N; ++i)
		Log("%12c", alphabetDefault[i]);
	Log("\n");

	for (unsigned i = 0; i < N; ++i)
		{
		char ci = alphabetDefault[i];
		Log("/* %c */\t{ ", ci);
		for (unsigned j = 0; j < N; ++j)
			{
			float pi = emitSingleDefault[i];
			float pj = emitSingleDefault[j];
			float pij = emitPairsDefault[i][j];
			float s = log(pij) - log(pi) - log(pj);
			Log("%10.5ff,", s);
			}
		Log("  }, // %c\n", ci);
		}

	Log("    ");
	for (unsigned i = 0; i < N; ++i)
		Log("%10c", alphabetDefault[i]);
	Log("\n");

	for (unsigned i = 0; i < N; ++i)
		{
		char ci = alphabetDefault[i];
		Log("%c   ", ci);
		for (unsigned j = 0; j < N; ++j)
			{
			float pi = emitSingleDefault[i];
			float pj = emitSingleDefault[j];
			float pij = emitPairsDefault[i][j];
			float s = log(pij) - log(pi) - log(pj);
			Log("%10.5f", s);
			}
		Log("\n", ci);
		}

	string Alpha2("ARNDCQEGHILKMFPSTWYV");
	Log("    ");
	for (unsigned i = 0; i < 20; ++i)
		Log("%10c", Alpha2[i]);
	Log("\n");

	for (unsigned i = 0; i < 20; ++i)
		{
		char ci = Alpha2[i];
		Log("%c   ", ci);
		int ii = cvt(ci);
		for (unsigned j = 0; j < 20; ++j)
			{
			char cj = Alpha2[j];
			int jj = cvt(cj);
			float s = 0.0f;
			if (ii >= 0 && jj >= 0)
				{
				float pi = emitSingleDefault[ii];
				float pj = emitSingleDefault[jj];
				float pij = emitPairsDefault[ii][jj];
				s = log(pij) - log(pi) - log(pj);
				}
			Log("%10.5f", s);
			}
		Log("\n", ci);
		}
	}

//} // end namespace PROBCONS_RNA
