#include "myutils.h"
#include "seqdb.h"
#include "tree.h"
#include "isgap.h"

void SeqDB::LogMe(const SeqDB * /*Input*/) const
	{
	asserta(m_Aligned);

	const unsigned SeqCount = GetSeqCount();
	const unsigned ColCount = GetColCount();
	const unsigned BlockCount = (ColCount + opt_rowlen - 1)/opt_rowlen;
	const unsigned MaxHi = GetMaxHi();

	Log("\n");
	if (m_Name == "")
		Log("Multiple alignment:\n");
	else
		Log("%s\n", m_Name.c_str());
	Log("%u seqs, %u cols", SeqCount, ColCount);
	if (m_Accuracy >= 0.0f)
		Log(", estimated accuracy %.0f%%", m_Accuracy*100.0f);
	Log("\n");

// dumb way to get log10...
	char Tmp[16];
	sprintf(Tmp, "%u", MaxHi+1);
	const unsigned npos = (unsigned) strlen(Tmp);
	const unsigned nlab = GetMaxShortLabelLength();

	vector<unsigned> Pos;
	Pos.reserve(SeqCount);
	for (unsigned i = 0; i < SeqCount; ++i)
		{
		if (m_Strands[i])
			Pos.push_back(GetLo(i));
		else
			Pos.push_back(GetHi(i));
		}

	for (unsigned BlockIndex = 0; BlockIndex < BlockCount; ++BlockIndex)
		{
		if (BlockIndex > 0)
			Log("\n");

		unsigned ColStart = BlockIndex*opt_rowlen;
		unsigned ColEnd = ColStart + opt_rowlen - 1;
		if (ColEnd >= ColCount)
			ColEnd = ColCount - 1;
		unsigned RowLength = ColEnd - ColStart + 1;

		for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
			{
			const byte *Seq = GetSeq(SeqIndex);

			unsigned p = Pos[SeqIndex];
			unsigned StartPos = UINT_MAX;
			unsigned EndPos = UINT_MAX;
			bool Strand = m_Strands[SeqIndex];
			for (unsigned ColIndex = ColStart; ColIndex <= ColEnd; ++ColIndex)
				{
				byte c = Seq[ColIndex];
				if (isgap(c))
					continue;
				if (StartPos == UINT_MAX)
					StartPos = p;
				EndPos = p;
				if (Strand)
					++p;
				else
					--p;
				}
			Pos[SeqIndex] = p;
			string ShortLabel;
			GetShortLabel(SeqIndex, ShortLabel);
			Log("%*.*s", nlab, nlab, ShortLabel.c_str());
			if (StartPos == UINT_MAX)
				Log(" %*.*s", npos, npos, "-");
			else
				Log(" %*u", npos, StartPos+1);

			if (IsNucleo())
				Log(" %c", pom(GetStrand(SeqIndex)));

			Log(" %*.*s", RowLength, RowLength, Seq + ColStart);

			if (EndPos == UINT_MAX)
				Log(" -");
			else
				Log(" %u", EndPos+1);

			if (BlockIndex + 1 == BlockCount)
				Log("(%u)", GetFullLength(SeqIndex));

			Log("\n");
			}

	// Conservation row
		string Line;
		GetSumLine(Line, ColStart, ColEnd);

		Log("%*.*s", nlab, nlab, "");
		Log(" %*.*s", npos, npos, "");
		if (IsNucleo())
			Log(" %c ", ' ');
		Log("%s", Line.c_str());
		Log("\n");
		}
	}

void SeqDB::ToMAF(FILE *f) const
	{
	asserta(m_Aligned);
	fprintf(f, "\n");
	if (!m_Name.empty())
		fprintf(f, "# %s\n", m_Name.c_str());
	fprintf(f, "a\n");

// dumb way to get log10...
	char Tmp[16];
	const unsigned MaxPos = GetMaxMAFPos();
	sprintf(Tmp, "%u", MaxPos);
	const unsigned npos = (unsigned) strlen(Tmp);

	const unsigned nlab = GetMaxShortLabelLength();

	sprintf(Tmp, "%u", GetMaxUngappedSeqLength());
	const unsigned nunl = (unsigned) strlen(Tmp);

	sprintf(Tmp, "%u", GetMaxFullLength());
	const unsigned nfull = (unsigned) strlen(Tmp);

 //s hg16.chr7    27707221 13 + 158545518 gcagctgaaaaca
	const unsigned SeqCount = GetSeqCount();
	const unsigned ColCount = GetColCount();
	for (unsigned Id = 0; Id < SeqCount; ++Id)
		{
		string Label;
		GetShortLabel(Id, Label);
		fprintf(f, "s %*.*s", nlab, nlab, Label.c_str());

		unsigned FullLength = GetFullLength(Id);
		unsigned UngappedLength = GetUngappedSeqLength(Id);

		bool Strand = GetStrand(Id);
		unsigned Pos = GetMAFPos(Id);

		fprintf(f, " %*u %*u", npos, Pos, nunl, UngappedLength);

		fprintf(f, " %c %*u %*.*s", pom(Strand), nfull, FullLength, ColCount, ColCount, GetSeq(Id));
		fprintf(f, "\n");
		}

	string Line;
	GetSumLine(Line);
	fprintf(f, "# %*.*s", nlab, nlab, "");
	fprintf(f, " %*.*s %*.*s", npos, npos, "", nunl, nunl, "");
	fprintf(f, " %c %*.*s %s", ' ', nfull, nfull, "", Line.c_str());
	fprintf(f, "\n");
	}
