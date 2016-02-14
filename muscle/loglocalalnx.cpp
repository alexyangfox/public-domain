#include "myutils.h"
#include "seqdb.h"
#include "isgap.h"
#include "params.h"

void GetLetterCounts(const string &Path, unsigned &NA, unsigned &NB);
float Pct(float x, float y);
byte CodonToAA(const byte *DNA);

static const char *FrameStr(unsigned Frame)
	{
	if (Frame == 0)
		return "F0";
	else if (Frame == 1)
		return "F1";
	else if (Frame == 2)
		return "F2";
	Die("Invalid frame");
	ureturn("?");
	}

void LogLocalAlnX(const byte *A, const byte *B, unsigned LoA, unsigned LoB,
  const string &Path)
	{
	asserta(!Path.empty());

	unsigned FrameA = LoA%3;
	unsigned FrameB = LoB%3;

	const char *sFrameA = FrameStr(FrameA);
	const char *sFrameB = FrameStr(FrameB);

	unsigned NA;
	unsigned NB;
	GetLetterCounts(Path, NA, NB);
	NA *= 3;
	NB *= 3;

	unsigned HiA = LoA + NA - 1;
	unsigned HiB = LoB + NB - 1;

	unsigned MaxPos = max(HiA, HiB);
	char sPos[16];
	sprintf(sPos, "%u", MaxPos);
	unsigned nPos = (unsigned) strlen(sPos);

	const unsigned PathLength = SIZE(Path);
	const unsigned BlockCount = (PathLength*4 + opt_rowlen - 1)/opt_rowlen;

	unsigned PosA = LoA;
	unsigned PosB = LoB;

	unsigned ColCount = 0;
	unsigned MatchCount = 0;
	unsigned PairCount = 0;
	unsigned GapCount = 0;
	unsigned PathPos = 0;
	for (unsigned BlockIndex = 0; BlockIndex < BlockCount; ++BlockIndex)
		{
		unsigned FirstPosA = PosA;
		unsigned LastPosA = PosA;
		string RowA;
		string AAsA;
		unsigned PathStartPos = PathPos;
		for (unsigned CodonIndex = 0; CodonIndex < opt_rowlen/4; ++CodonIndex)
			{
			if (PathPos >= PathLength)
				break;
			char c = Path[PathPos++];
			if (c == 'M' || c == 'D')
				{
				char AA = CodonToAA(A + PosA);
				AAsA.push_back(AA);
				RowA.push_back(A[PosA++]);
				RowA.push_back(A[PosA++]);
				LastPosA = PosA;
				RowA.push_back(A[PosA++]);
				}
			else
				{
				AAsA.push_back('-');
				RowA.push_back('-');
				RowA.push_back('-');
				RowA.push_back('-');
				}
			RowA.push_back(' ');
			}

		unsigned FirstPosB = PosB;
		unsigned LastPosB = PosB;
		string RowB;
		string AAsB;
		PathPos = PathStartPos;
		for (unsigned CodonIndex = 0; CodonIndex < opt_rowlen/4; ++CodonIndex)
			{
			if (PathPos >= PathLength)
				break;
			char c = Path[PathPos++];
			if (c == 'M' || c == 'I')
				{
				char AA = CodonToAA(B + PosB);
				AAsB.push_back(AA);
				RowB.push_back(B[PosB++]);
				RowB.push_back(B[PosB++]);
				LastPosB = PosB;
				RowB.push_back(B[PosB++]);
				}
			else
				{
				AAsB.push_back('-');
				RowB.push_back('-');
				RowB.push_back('-');
				RowB.push_back('-');
				}
			RowB.push_back(' ');
			}
		
		for (unsigned i = 0; i < SIZE(AAsA); ++i)
			{
			++ColCount;
			byte a = AAsA[i];
			byte b = AAsB[i];
			if (a == '-' || b == '-')
				++GapCount;
			else 
				{
				++PairCount;
				if (a == b)
					++MatchCount;
				}
			}

		if (BlockIndex > 0)
			Log("\n");

		Log("%*u %s %s%u\n",
		  nPos, FirstPosA, sFrameA, RowA.c_str(), LastPosA);

		Log("%*.*s  %s", nPos, nPos, "", "  ");
		for (unsigned i = 0; i < SIZE(AAsA); ++i)
			Log("%3c ", AAsA[i]);
		Log("\n");

		Log("%*.*s  %s", nPos, nPos, "", "  ");
		for (unsigned i = 0; i < SIZE(AAsB); ++i)
			Log("%3c ", AAsB[i]);
		Log("\n");

		Log("%*u %s %s%u\n",
		  nPos, FirstPosB, sFrameB, RowB.c_str(), LastPosB);
		}
	Log("\n");
	Log("Identities %u/%u (%.1f%%), gaps %u/%u (%.1f%%)\n",
	  MatchCount, PairCount, Pct(MatchCount, PairCount),
	  GapCount, ColCount, Pct(GapCount, ColCount));
	}

void LogLocalAlnX(SeqDB &DB, unsigned SeqIndex1, unsigned SeqIndex2,
  unsigned Start1, unsigned Start2, const string &Path)
	{
	const byte *A = DB.GetSeq(SeqIndex1);
	const byte *B = DB.GetSeq(SeqIndex2);
	Log("\n");
	Log(">%s\n", DB.GetLabel(SeqIndex1).c_str());
	Log(">%s\n", DB.GetLabel(SeqIndex2).c_str());
	LogLocalAlnX(A, B, Start1, Start2, Path);
	}
