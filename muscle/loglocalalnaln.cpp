#include "myutils.h"
#include "seqdb.h"
#include "isgap.h"
#include "params.h"
#include "hit.h"

void GetLetterCounts(const string &Path, unsigned &i, unsigned &j);
byte CompLetter(byte c);

void LogLocalAlnAln(SeqDB &DBA, SeqDB &DBB, unsigned LoA, unsigned LoB,
  const string &Path, bool Inverted, bool Nucleo)
	{
	asserta(!Path.empty());

	const unsigned SeqCountA = DBA.GetSeqCount();
	const unsigned SeqCountB = DBB.GetSeqCount();

	unsigned MaxShortLabelLengthA = DBA.GetMaxShortLabelLength();
	unsigned MaxShortLabelLengthB = DBB.GetMaxShortLabelLength();
	unsigned nLab = max(MaxShortLabelLengthA, MaxShortLabelLengthB);

	unsigned NA;
	unsigned NB;
	GetLetterCounts(Path, NA, NB);

	unsigned MaxHiA = DBA.GetMaxHi();
	unsigned MaxHiB = DBB.GetMaxHi();

	unsigned MaxMaxHi = max(MaxHiA, MaxHiB);
	char sCol[16];
	sprintf(sCol, "%u", MaxMaxHi);
	unsigned nCol = (unsigned) strlen(sCol);

	const unsigned ColCount = SIZE(Path);
	const unsigned BlockCount = (ColCount + opt_rowlen - 1)/opt_rowlen;

	unsigned ColA = LoA;
	unsigned ColB = (Inverted ? LoB+NB-1 : LoB);

	for (unsigned BlockIndex = 0; BlockIndex < BlockCount; ++BlockIndex)
		{
		unsigned ColStart = BlockIndex*opt_rowlen;
		unsigned ColEnd = ColStart + opt_rowlen;
		if (ColEnd > ColCount)
			ColEnd = ColCount;
		Log("\n");
		unsigned BlockStartColA = ColA;
		unsigned BlockEndColA = BlockStartColA;
		for (unsigned SeqIndexA = 0; SeqIndexA < SeqCountA; ++SeqIndexA)
			{
			const byte *A = DBA.GetSeq(SeqIndexA);
			string RowA;
			ColA = BlockStartColA;
			for (unsigned ColIndex = ColStart; ColIndex < ColEnd; ++ColIndex)
				{
				char c = Path[ColIndex];
				if (c == 'M' || c == 'D')
					{
					RowA.push_back(A[ColA]);
					BlockEndColA = ColA++;
					}
				else
					RowA.push_back('-');
				}
			unsigned StartPos = DBA.ColToPos(SeqIndexA, BlockStartColA);
			unsigned EndPos = DBA.ColToPos(SeqIndexA, BlockEndColA);

			string Label;
			DBA.GetShortLabel(SeqIndexA, Label);
			const char *Strand = (Nucleo ? "+ " : "");
			Log("%*.*s  %*u %s%s %u\n",
			  nLab, nLab, Label.c_str(), nCol, StartPos+1, Strand, RowA.c_str(), EndPos+1);
			}
		ColA = BlockEndColA;
		Log("\n");

		unsigned BlockStartColB = ColB;
		unsigned BlockEndColB = BlockStartColB;
		for (unsigned SeqIndexB = 0; SeqIndexB < SeqCountB; ++SeqIndexB)
			{
			const byte *B = DBB.GetSeq(SeqIndexB);
			string RowB;
			ColB = BlockStartColB;
			for (unsigned ColIndex = ColStart; ColIndex < ColEnd; ++ColIndex)
				{
				char c = Path[ColIndex];
				if (c == 'M' || c == 'I')
					{
					char b = B[ColB];
					if (Inverted)
						{
						RowB.push_back(CompLetter(b));
						asserta(ColB > 0 || ColIndex + 1 == ColEnd);
						BlockEndColB = ColB--;
						}
					else
						{
						RowB.push_back(b);
						BlockEndColB = ColB++;
						}
					}
				else
					RowB.push_back('-');
				}

			unsigned StartPos = DBB.ColToPos(SeqIndexB, BlockStartColB);
			unsigned EndPos = DBB.ColToPos(SeqIndexB, BlockEndColB);

			const char *Strand = "";
			if (Nucleo)
				Strand = (Inverted ? "- " : "+ ");
			string Label;
			DBB.GetShortLabel(SeqIndexB, Label);
			Log("%*.*s  %*u %s%s %u\n",
			  nLab, nLab, Label.c_str(), nCol, StartPos+1, Strand, RowB.c_str(), EndPos+1);
			}
		ColB = BlockEndColB;
		}
	Log("\n");
	}

void LogLocalAlnAlnHit(SeqDB &DBA, SeqDB &DBB, const HitData &Hit)
	{
	LogLocalAlnAln(DBA, DBB, Hit.LoA, Hit.LoB, Hit.Path, !Hit.Strand, DBA.IsNucleo());
	}
