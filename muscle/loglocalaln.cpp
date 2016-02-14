#include "myutils.h"
#include "seqdb.h"
#include "isgap.h"
#include "params.h"
#include "hit.h"

void GetLetterCounts(const string &Path, unsigned &NA, unsigned &NB);
void LogSelfMatchCartoon(unsigned L, unsigned StartA, unsigned EndA,
  unsigned StartB, unsigned EndB);
void LogLocalMatchCartoon(unsigned LA, unsigned LB, unsigned StartA,
  unsigned StartB, unsigned EndA, unsigned EndB, bool Self);
unsigned Overlap(unsigned StartA, unsigned EndA, unsigned StartB, unsigned EndB);
void LogInvertCartoon(unsigned LA, unsigned LB, unsigned StartA,
  unsigned EndA, unsigned StartB, unsigned EndB, bool Self);
void LogSelfInvertCartoon(unsigned L, unsigned StartA, unsigned EndA,
  unsigned StartB, unsigned EndB);
byte CompLetter(byte c);

float Pct(float x, float y)
	{
	if (y == 0.0f)
		return 0.0f;
	return (x*100.0f)/y;
	}

char ProbStrengthSymbol(float Prob)
	{
	if (Prob > 0.9)
		return '*';
	else if (Prob > 0.75)
		return '|';
	else if (Prob > 0.5)
		return ':';
	else if (Prob > 0.25)
		return '.';
	return ' ';
	}

char MatchStrengthSymbolNucleo(byte Letter1, byte Letter2)
	{
	if (isgap(Letter1) || isgap(Letter2))
		return ' ';
	Letter1 = toupper(Letter1);
	Letter2 = toupper(Letter2);
	if (Letter1 == Letter2)
		return '|';
	return ' ';
	}

char MatchStrengthSymbolAmino(byte Letter1, byte Letter2)
	{
	float **SubstMx = GetSubstMx();
	if (isgap(Letter1) || isgap(Letter2))
		return ' ';
	Letter1 = toupper(Letter1);
	Letter2 = toupper(Letter2);
	if (Letter1 == Letter2)
		return Letter1;

	float Score = SubstMx[Letter1][Letter2];
	if (Score > 0.2)
		return '+';
	else if (Score > -0.5)
		return ' ';
	return '?';
	}

static void GetSimStrNucleo(const string &a, const string &b, string &s)
	{
	s.clear();
	const unsigned L = SIZE(a);
	asserta(SIZE(b) == L);
	for (unsigned i = 0; i < L; ++i)
		{
		char c = a[i];
		char d = b[i];
		s.push_back(MatchStrengthSymbolNucleo((byte) c, (byte) d));
		}
	}

void GetSimStrAmino(const string &a, const string &b, string &s)
	{
	s.clear();
	const unsigned L = SIZE(a);
	asserta(SIZE(b) == L);
	for (unsigned i = 0; i < L; ++i)
		{
		char c = a[i];
		char d = b[i];
		s.push_back(MatchStrengthSymbolAmino((byte) c, (byte) d));
		}
	}

bool IsPalindrome(unsigned Start1, unsigned End1, unsigned Start2, unsigned End2)
	{
	unsigned L1 = End1 - Start1 + 1;
	unsigned L2 = End2 - Start2 + 1;
	unsigned Length = (L1 + L2)/2;
	unsigned Ov = Overlap(Start1, End1, Start2, End2);
	return Ov > 0 && float(Ov)/float(Length) > 0.9;
	}

void LogLocalAln(const byte *A, const byte *B, unsigned LoA, unsigned LoB,
  unsigned LoOffsetA, unsigned LoOffsetB, const string &Path, bool Inverted,
  bool Nucleo)
	{
	asserta(!Path.empty());

	unsigned NA;
	unsigned NB;
	GetLetterCounts(Path, NA, NB);

	unsigned HiA = LoA + LoOffsetA + NA - 1;
	unsigned HiB = LoB + LoOffsetB + NB - 1;
	unsigned HiOffsetB = LoOffsetB + NB - 1;

	unsigned MaxPos = max(HiA, HiB);
	char sPos[16];
	sprintf(sPos, "%u", MaxPos);
	unsigned nPos = (unsigned) strlen(sPos);

	const unsigned ColCount = SIZE(Path);
	const unsigned BlockCount = (ColCount + opt_rowlen - 1)/opt_rowlen;

	unsigned OffsetA = LoOffsetA;
	unsigned OffsetB = (Inverted ? HiOffsetB : LoOffsetB);

	unsigned PosA = LoA + OffsetA;
	unsigned PosB = LoB + OffsetB;

	unsigned MatchCount = 0;
	unsigned PairCount = 0;
	unsigned GapCount = 0;
	for (unsigned BlockIndex = 0; BlockIndex < BlockCount; ++BlockIndex)
		{
		unsigned ColStart = BlockIndex*opt_rowlen;
		unsigned ColEnd = ColStart + opt_rowlen;
		if (ColEnd > ColCount)
			ColEnd = ColCount;

		unsigned FirstPosA = PosA;
		unsigned LastPosA = PosA;
		string RowA;
		for (unsigned ColIndex = ColStart; ColIndex < ColEnd; ++ColIndex)
			{
			char c = Path[ColIndex];
			if (c == 'M' || c == 'D')
				{
				LastPosA = PosA;
				RowA.push_back(A[OffsetA++]);
				++PosA;
				}
			else
				RowA.push_back('-');
			}

		unsigned FirstPosB = PosB;
		unsigned LastPosB = PosB;
		string RowB;
		for (unsigned ColIndex = ColStart; ColIndex < ColEnd; ++ColIndex)
			{
			char c = Path[ColIndex];
			if (c == 'M' || c == 'I')
				{
				LastPosB = PosB;
				char b = B[OffsetB];
				if (Inverted)
					{
					RowB.push_back(CompLetter(b));
					asserta(PosB > 0 || (PosB == 0 && ColIndex == ColEnd - 1));
					--PosB;
					--OffsetB;
					}
				else
					{
					RowB.push_back(b);
					++PosB;
					++OffsetB;
					}
				}
			else
				RowB.push_back('-');
			}

		string Sim;
		if (Nucleo)
			GetSimStrNucleo(RowA, RowB, Sim);
		else
			GetSimStrAmino(RowA, RowB, Sim);

		unsigned L = SIZE(RowA);
		asserta(SIZE(RowB) == L);
		for (unsigned i = 0; i < L; ++i)
			{
			char c = Path[ColStart + i];
			if (c == 'M')
				{
				++PairCount;
				if (toupper(RowA[i]) == toupper(RowB[i]))
					++MatchCount;
				}
			else
				++GapCount;
			}

		const char *Strand = (Nucleo ? "+ " : "");
		Log("\n");
		Log("%*u %s%s %u\n",
		  nPos, FirstPosA+1, Strand, RowA.c_str(), LastPosA+1);

		Strand = (Nucleo ? "  " : "");
		Log("%*.*s %s%s\n", nPos, nPos, "", Strand, Sim.c_str());

		Strand = "";
		if (Nucleo)
			Strand = (Inverted ? "- " : "+ ");
		Log("%*u %s%s %u\n",
		  nPos, FirstPosB+1, Strand, RowB.c_str(), LastPosB+1);
		}
	Log("\n");
	Log("Identities %u/%u (%.1f%%), gaps %u/%u (%.1f%%)\n",
	  MatchCount, PairCount, Pct(MatchCount, PairCount),
	  GapCount, ColCount, Pct(GapCount, ColCount));
	}

//void LogSelfAln(SeqDB &DB, unsigned Id, unsigned Starti,
//  unsigned Startj, const string &Path)
//	{
//	unsigned L = DB.GetSeqLength(Id);
//	const byte *Seq = DB.GetSeq(Id);
//	unsigned Ni;
//	unsigned Nj;
//	GetLetterCounts(Path, Ni, Nj);
//	unsigned Endi = Starti + Ni - 1;
//	unsigned Endj = Startj + Nj - 1;
//	unsigned LoPos = DB.GetLo(Id);
//	// bool Repeat = (Overlap(Starti, Endi, Startj, Endj) > 0);
//
//	Log("\n");
//	Log("Self:\n");
//	Log(">%s\n", DB.GetLabel(Id).c_str());
//	LogLocalAln(Seq, Seq, LoPos, LoPos, Starti, Startj, Path,
//	  false, DB.IsNucleo());
//	LogSelfMatchCartoon(L, Starti, Endi, Startj, Endj);
//	}

void LogSelfInvert(SeqDB &DB, unsigned Id, unsigned Starti,
  unsigned Startj, const string &Path)
	{
	unsigned L = DB.GetSeqLength(Id);
	const byte *Seq = DB.GetSeq(Id);
	unsigned Ni;
	unsigned Nj;
	GetLetterCounts(Path, Ni, Nj);
	unsigned Endi = Starti + Ni - 1;
	unsigned Endj = Startj + Nj - 1;
	unsigned LoPos = DB.GetLo(Id);

	bool Pal = IsPalindrome(Starti, Endi, Startj, Endj);

	Log("\n");
	if (Pal)
		Log("Palindrome:\n");
	else
		Log("Inverted duplication:\n");

	Log(">%s\n", DB.GetLabel(Id).c_str());

	LogLocalAln(Seq, Seq, LoPos, LoPos, Starti, Startj, Path, true, DB.IsNucleo());
	LogSelfInvertCartoon(L, Starti, Endi, Startj, Endj);
	}

void LogInvert(SeqDB &DB, unsigned IdA, unsigned IdB, unsigned StartA,
  unsigned StartB, const string &Path)
	{
	if (IdA == IdB)
		{
		LogSelfInvert(DB, IdA, StartA, StartB, Path);
		return;
		}

	Log("\n");
	Log("Inversion:\n");
	Log(">%s\n", DB.GetLabel(IdA).c_str());
	Log(">%s\n", DB.GetLabel(IdB).c_str());

	unsigned LA = DB.GetSeqLength(IdA);
	unsigned LB = DB.GetSeqLength(IdB);

	const byte *A = DB.GetSeq(IdA);
	const byte *B = DB.GetSeq(IdB);

	unsigned NA;
	unsigned NB;
	GetLetterCounts(Path, NA, NB);
	unsigned EndA = StartA + NA - 1;
	unsigned EndB = StartB + NB - 1;

	unsigned LoA = DB.GetLo(IdA);
	unsigned LoB = DB.GetLo(IdB);

	LogLocalAln(A, B, LoA, LoB, StartA, StartB, Path, true, DB.IsNucleo());
	LogInvertCartoon(LA, LB, StartA, EndA, StartB, EndB, false);
	}

void LogLocalAlnDB(SeqDB &DB, unsigned IdA, unsigned IdB, unsigned LoOffsetA,
  unsigned LoOffsetB, const string &Path, bool Inverted)
	{
	bool Nucleo = DB.IsNucleo();

	Log("\n");
	if (IdA == IdB)
		{
		Log("Self:\n");
		Log(">%s\n", DB.GetLabel(IdA).c_str());
		}
	else
		{
		Log("Local:\n");
		Log(">%s\n", DB.GetLabel(IdA).c_str());
		Log(">%s\n", DB.GetLabel(IdB).c_str());
		}

	const byte *A = DB.GetSeq(IdA);
	const byte *B = DB.GetSeq(IdB);
	unsigned LoA = DB.GetLo(IdA);
	unsigned LoB = DB.GetLo(IdB);
	LogLocalAln(A, B, LoA, LoB, LoOffsetA, LoOffsetB, Path, Inverted, Nucleo);

	unsigned LA = DB.GetSeqLength(IdA);
	unsigned LB = DB.GetSeqLength(IdB);
	unsigned Ni;
	unsigned Nj;
	GetLetterCounts(Path, Ni, Nj);
	unsigned Endi = LoOffsetA + Ni - 1;
	unsigned Endj = LoOffsetB + Nj - 1;

	if (IdA == IdB)
		{
		if (Inverted)
			LogInvertCartoon(LA, LB, LoOffsetA, Endi, LoOffsetB, Endj, true);
		else
			LogSelfMatchCartoon(LA, LoOffsetA, Endi, LoOffsetB, Endj);
		}
	else
		{
		if (Inverted)
			LogInvertCartoon(LA, LB, LoOffsetA, Endi, LoOffsetB, Endj, false);
		else
			LogLocalMatchCartoon(LA, LB, LoOffsetA, Endi, LoOffsetB, Endj, false);
		}
	}

void LogLocalAlnHit(SeqDB &DB, unsigned IdA, unsigned IdB, const HitData &Hit)
	{
	LogLocalAlnDB(DB, IdA, IdB, Hit.LoA, Hit.LoB, Hit.Path, !Hit.Strand);
	}
