#include "myutils.h"
#include "mx.h"
#include "sparsemx.h"
#include "seqdb.h"
#include "params.h"
#include "info.h"
#include <algorithm>

void IterateLocalFB(const string &Description, vector<string> &Paths,
  vector<unsigned> &Startis, vector<unsigned> &Startjs, vector<float> &Scores);
void GetLetterCounts(const string &Path, unsigned &i, unsigned &j);
void LogInvert(SeqDB &DB, unsigned IdA, unsigned IdB,
  unsigned StartA, unsigned StartB, const string &Path);
bool IsPalindrome(unsigned Start1, unsigned End1, unsigned Start2, unsigned End2);

static vector<InvertInfo> g_InvertInfos;

const vector<InvertInfo> &GetInvertInfos()
	{
	return g_InvertInfos;
	}

byte CompLetter(byte c)
	{
	switch (c)
		{
	case 'A': return 'T';
	case 'C': return 'G';
	case 'G': return 'C';
	case 'T': return 'A';

	case 'a': return 't';
	case 'c': return 'g';
	case 'g': return 'c';
	case 't': return 'a';
		}
	return c;
	}

void RevComp(const byte *Seq, byte *RevCompSeq, unsigned L)
	{
	for (unsigned i = 0; i < L; ++i)
		{
		byte c = Seq[i];
		RevCompSeq[L-i-1] = CompLetter(c);
		}
	}

void RevComp(byte *Seq, unsigned L)
	{
	for (unsigned i = 0; i < L/2; ++i)
		{
		byte c1 = Seq[i];
		byte c2 = Seq[L-i-1];

		Seq[i] = CompLetter(c2);
		Seq[L-i-1] = CompLetter(c1);
		}
	if (L%2 != 0)
		{
		byte c = Seq[L/2];
		Seq[L/2] = CompLetter(c);
		}
	}

void RevComp(string &Seq)
	{
	const unsigned L = SIZE(Seq);
	for (unsigned i = 0; i < L/2; ++i)
		{
		byte c1 = Seq[i];
		byte c2 = Seq[L-i-1];

		Seq[i] = CompLetter(c2);
		Seq[L-i-1] = CompLetter(c1);
		}
	if (L%2 != 0)
		{
		byte c = Seq[L/2];
		Seq[L/2] = CompLetter(c);
		}
	}

void ComputeInvertsPair(SeqDB &DB, unsigned InputSeqIndex1, unsigned InputSeqIndex2,
  vector<string> &Paths, vector<unsigned> &Startis, vector<unsigned> &Startjs,
  vector<float> &Scores)
	{
	Paths.clear();
	Startis.clear();
	Startjs.clear();

	bool Self = (InputSeqIndex1 == InputSeqIndex2);

	string Label1 = DB.GetLabel(InputSeqIndex1);
	string RevCompLabel2str = string(DB.GetLabel(InputSeqIndex2)) + string(".revcomp");

	const unsigned L1 = DB.GetSeqLength(InputSeqIndex1);
	const unsigned L2 = DB.GetSeqLength(InputSeqIndex2);

	byte *Seq1 = DB.GetSeq(InputSeqIndex1);
	byte *Seq2 = DB.GetSeq(InputSeqIndex2);

	unsigned BuffLen = L1 + L2 + RevCompLabel2str.size() + 1;
	char *Buffer = myalloc<char>(BuffLen);
	byte *RevCompSeq2 = (byte *) (Buffer + L1);
	char *RevCompLabel2 = Buffer + L1 + L2;

	unsigned n = RevCompLabel2str.size();
	memcpy(RevCompLabel2, RevCompLabel2str.c_str(), n);
	RevCompLabel2[n] = 0;

	memcpy(Buffer, Seq1, L1);
	RevComp(Seq2, RevCompSeq2, L2); 

	SeqDB DBPair;
	DBPair.AppendSeq(Label1.c_str(), Seq1, L1);
	DBPair.AppendSeq(RevCompLabel2, RevCompSeq2, L2);

	SetSimMx(DBPair, 0, 1);
	vector<string> Paths2;
	vector<unsigned> Startis2;
	vector<unsigned> Startjs2;
	vector<float> Scores2;
	IterateLocalFB("Inverted", Paths2, Startis2, Startjs2, Scores2);

	const unsigned PathCount = SIZE(Paths2);
	if (PathCount == 0)
		return;

	for (unsigned PathIndex = 0; PathIndex < PathCount; ++PathIndex)
		{
		const string &Path = Paths2[PathIndex];
		unsigned Starti = Startis2[PathIndex];
		unsigned Startj = Startjs2[PathIndex];
		float Score = Scores2[PathIndex];

		unsigned Ni;
		unsigned Nj;
		GetLetterCounts(Path, Ni, Nj);

		unsigned Endi = Starti + Ni - 1;
//		unsigned Endj = Startj + Nj - 1;

		unsigned StartjInv = L2 - Startj - Nj;
		if (Self && StartjInv < Starti)
			continue;

		unsigned EndjInv = StartjInv + Nj - 1;
		bool Pal = Self && IsPalindrome(Starti, Endi, StartjInv, EndjInv);
		unsigned Length = (Ni + Nj)/2;
		if (Pal && Length < 2*opt_minlocallen)
			continue;

		Paths.push_back(Path);
		Startis.push_back(Starti);
		Startjs.push_back(StartjInv);
		Scores.push_back(Score);

		LogInvert(DB, InputSeqIndex1, InputSeqIndex2, Starti, StartjInv, Path);

		InvertInfo II;
		II.InputSeqIndex1 = InputSeqIndex1;
		II.InputSeqIndex2 = InputSeqIndex2;
		II.Label1 = DB.GetLabel(InputSeqIndex1);
		II.Label2 = DB.GetLabel(InputSeqIndex2);
		II.Start1 = Starti;
		II.End1 = Endi;
		II.Start2 = StartjInv;
		II.End2 = EndjInv;

		g_InvertInfos.push_back(II);
		}
	}

void ComputeInverts(SeqDB &DB)
	{
	string Model;
	GetLocalModel(DB, Model);
	SetModel(Model);

	const unsigned SeqCount = DB.GetSeqCount();
	const unsigned PairCount = (SeqCount*(SeqCount + 1))/2;
	unsigned Counter = 0;
	for (unsigned SeqIndex1 = 0; SeqIndex1 < SeqCount; ++SeqIndex1)
		{
		const unsigned InputSeqIndex1 = SeqIndex1;//@@
		const string &Label1 = DB.GetLabel(InputSeqIndex1);

		for (unsigned SeqIndex2 = SeqIndex1; SeqIndex2 < SeqCount; ++SeqIndex2)
			{
			const unsigned InputSeqIndex2 = SeqIndex2;//@@
			const string &Label2 = DB.GetLabel(InputSeqIndex2);

			ProgressStep(Counter++, PairCount, "Align inverts %.16s,%.16s",
			  Label1.c_str(), Label2.c_str());

			vector<string> Paths;
			vector<unsigned> Startis;
			vector<unsigned> Startjs;
			vector<float> Scores;
			ComputeInvertsPair(DB, InputSeqIndex1, InputSeqIndex2, Paths, Startis, Startjs,
			  Scores);
//			unsigned Count = SIZE(Paths);
			}
		}
	}

void LogInvertReport()
	{
	Log("\n");
	if (g_InvertInfos.empty())
		{
		Log("No inversions found.\n");
		return;
		}

	Log("Inversions:\n");
	Log(" Seq1   Seq2                Label1                Label2  From1    To1  Length1  From2    To2  Length2\n");
	Log("-----  -----  --------------------  --------------------  -----  -----  -------  -----  -----  -------\n");
	sort(g_InvertInfos.begin(), g_InvertInfos.end());

	const unsigned N = SIZE(g_InvertInfos);
	unsigned LastId1 = UINT_MAX;
	for (unsigned i = 0; i < N; ++i)
		{
		const InvertInfo &TI = g_InvertInfos[i];
		if (i > 0 && TI.InputSeqIndex1 != LastId1)
			Log("\n");
		Log("%5u", TI.InputSeqIndex1+1);
		Log("  %5u", TI.InputSeqIndex2+1);
		Log("  %20.20s", TI.Label1.c_str());
		Log("  %20.20s", TI.Label2.c_str());
		unsigned Length1 = TI.End1 - TI.Start1 + 1;
		unsigned Length2 = TI.End2 - TI.Start2 + 1;
		Log("  %5u  %5u  %7u  %5u  %5u  %7u",
		  TI.Start1+1, TI.End1+1, Length1, TI.Start2+1, TI.End2+1, Length2);
		Log("\n");

		LastId1 = TI.InputSeqIndex1;
		}
	}
