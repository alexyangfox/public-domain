#include "myutils.h"
#include "seqdb.h"
#include "isgap.h"
#include "info.h"

unsigned UpdateCartoon(string &Cartoon,
  unsigned SeqLenBases, unsigned SeqStartPixel,
  unsigned FromBase, unsigned ToBase, char c);
unsigned UpdateCartoon(string &Cartoon,
  unsigned SeqLenBases, unsigned SeqStartPixel,
  unsigned FromBase, unsigned ToBase, const string &s);
bool IsPalindrome(unsigned Start1, unsigned End1, unsigned Start2, unsigned End2);

void LogMSACartoon(SeqDB &msa)
	{
	unsigned WIDTH = 80;

	const unsigned SeqCount = msa.GetSeqCount();
	const unsigned ColCount = msa.GetColCount();
	if (WIDTH > ColCount)
		WIDTH = ColCount;

	vector<unsigned> InputSeqIndexToRowIndex(SeqCount, UINT_MAX);
	for (unsigned i = 0; i < SeqCount; ++i)
		{
		unsigned InputSeqIndex = msa.GetUser(i);
		asserta(InputSeqIndex < SeqCount);
		asserta(InputSeqIndexToRowIndex[InputSeqIndex] == UINT_MAX);
		InputSeqIndexToRowIndex[InputSeqIndex] = i;
		}

	vector<string> Rows(SeqCount);
	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{
		string &Row = Rows[SeqIndex];
		Row.resize(WIDTH, 'x');
		}

	const vector<RepeatInfo> &Repeats = GetRepeatInfos();
	const vector<DupeInfo> &Dupes = GetDupeInfos();
	const vector<InvertInfo> &Inverts = GetInvertInfos();

	for (unsigned i = 0; i < SIZE(Repeats); ++i)
		{
		const RepeatInfo &Info = Repeats[i];
		unsigned RowIndex = InputSeqIndexToRowIndex[Info.InputSeqIndex];
		string &Row = Rows[RowIndex];
		unsigned Start = msa.PosToCol(RowIndex, Info.Start);
		unsigned End = msa.PosToCol(RowIndex, Info.End);
		UpdateCartoon(Row, ColCount, 0, Start, End, 'R');
		}

	for (unsigned i = 0; i < SIZE(Dupes); ++i)
		{
		const DupeInfo &Info = Dupes[i];
		unsigned RowIndex = InputSeqIndexToRowIndex[Info.InputSeqIndex];
		string &Row = Rows[RowIndex];
		unsigned Start1 = msa.PosToCol(RowIndex, Info.Start1);
		unsigned Start2 = msa.PosToCol(RowIndex, Info.Start2);
		unsigned End1 = msa.PosToCol(RowIndex, Info.End1);
		unsigned End2 = msa.PosToCol(RowIndex, Info.End2);
		UpdateCartoon(Row, ColCount, 0, Start1, End1, '1');
		UpdateCartoon(Row, ColCount, 0, Start2, End2, '2');
		}

	for (unsigned i = 0; i < SIZE(Inverts); ++i)
		{
		const InvertInfo &Info = Inverts[i];
		unsigned SeqIndex1 = Info.InputSeqIndex1;
		unsigned SeqIndex2 = Info.InputSeqIndex2;
		if (SeqIndex1 == SeqIndex2 &&
		  IsPalindrome(Info.Start1, Info.End1, Info.Start2, Info.End2))
			{
			unsigned RowIndex = InputSeqIndexToRowIndex[Info.InputSeqIndex1];
			string &Row = Rows[RowIndex];
			unsigned Start1 = msa.PosToCol(RowIndex, Info.Start1);
			unsigned Start2 = msa.PosToCol(RowIndex, Info.Start2);
			unsigned End1 = msa.PosToCol(RowIndex, Info.End1);
			unsigned End2 = msa.PosToCol(RowIndex, Info.End2);
			unsigned MinStart = min(Start1, Start2);
			unsigned MaxEnd = max(End1, End2);
			UpdateCartoon(Row, ColCount, 0, MinStart, MaxEnd, "<>");
			}
		else
			{
			unsigned RowIndex1 = InputSeqIndexToRowIndex[Info.InputSeqIndex1];
			unsigned RowIndex2 = InputSeqIndexToRowIndex[Info.InputSeqIndex2];
			string &Row1 = Rows[RowIndex1];
			string &Row2 = Rows[RowIndex2];
			unsigned Start1 = msa.PosToCol(RowIndex1, Info.Start1);
			unsigned Start2 = msa.PosToCol(RowIndex2, Info.Start2);
			unsigned End1 = msa.PosToCol(RowIndex1, Info.End1);
			unsigned End2 = msa.PosToCol(RowIndex2, Info.End2);
			UpdateCartoon(Row1, ColCount, 0, Start1, End1, '>');
			UpdateCartoon(Row2, ColCount, 0, Start2, End2, '<');
			}
		}

	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{
		string &Row = Rows[SeqIndex];
		const byte *Seq = msa.GetSeq(SeqIndex);
		for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
			{
			char c = Seq[ColIndex];
			if (isgap(c))
				UpdateCartoon(Row, ColCount, 0, ColIndex, ColIndex, '-');
			}
		}

	Log("\n");
	Log("MSA cartoon, one symbol = %.1f bases\n", float(ColCount)/float(WIDTH));
	const unsigned nLab = msa.GetMaxShortLabelLength();
	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{
		string &Row = Rows[SeqIndex];
		string Label;
		msa.GetShortLabel(SeqIndex, Label);
		Log("%*.*s  %s\n", nLab, nLab, Label.c_str(), Row.c_str());
		}
	}
