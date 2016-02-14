#include "myutils.h"
#include "seqdb.h"
#include "isgap.h"

void SeqDB::ReadSeqs(const string &FileName)
	{
	Clear();

	Progress("Reading %s\n", FileName.c_str());
	FILE *f = 0;
	if (FileName == "")
		f = stdin;
	else
		f = OpenStdioFile(FileName);

	unsigned FileSize;
	m_Buffer = ReadAllStdioFile(f, FileSize);
	if (FileSize == 0)
		return;

	enum
		{
		AtStart,
		InLabel,
		InSeq,
		} State = AtStart;

	vector<string> LongSeqLabels;
	vector<unsigned> LongSeqLengths;

	byte *Ptr = m_Buffer;
	char *Label = 0;
	unsigned SeqLength = 0;
	for (unsigned i = 0; i < FileSize; ++i)
		{
		ProgressStep(i, FileSize, "%u seqs", SIZE(m_Labels));
		assert(Ptr <= m_Buffer + i);
		byte c = m_Buffer[i];
		bool eol = (c == '\n' || c == '\r');

		switch (State)
			{
		case AtStart:
			if (isspace(c))
				continue;
			else if (c == '>')
				goto StartLabel;
			else
				{
				if (isprint(c))
					Die("Invalid sequence data, expected '>' but got '%c'", c);
				else
					Die("Invalid sequence data, expected '>' but got non-printing byte 0x%02x", c);
				}
			continue;

		case InLabel:
			if (eol)
				{
				*Ptr++ = 0;
				goto StartSeq;
				}
			else
				*Ptr++ = c;
			continue;

		case InSeq:
			if (isspace(c))
				continue;
			else if (c == '>')
				goto StartLabel;
			else if (!isprint(c))
				Warning("non-printing byte 0x%02x ignored in sequence", c);
			else
				{
				*Ptr++ = c;
				++SeqLength;
				}
			continue;

		default:
			asserta(false);
			}

	StartLabel:
		{
		assert(SIZE(m_Labels) == SIZE(m_Seqs));
		State = InLabel;
		if (!m_Labels.empty())
			{
			if (SeqLength == 0)
				Die("Empty sequence in '%.32s'", FileName.c_str());
			if (SeqLength > opt_maxseqlen)
				{
				LongSeqLabels.push_back(m_Labels.back());
				LongSeqLengths.push_back(SeqLength);
				m_Labels.pop_back();
				m_Seqs.pop_back();
				Label = (char *) (Ptr);
				continue;
				}
			m_Lengths.push_back(SeqLength);
			}
		Label = (char *) (Ptr);
		continue;
		}

	StartSeq:
		m_Labels.push_back(Label);
		SeqLength = 0;
		State = InSeq;
		m_Seqs.push_back((byte *) Ptr);
		continue;
		}

	if (SeqLength == 0)
		Die("Empty sequence in '%.32s'", FileName.c_str());
	m_Lengths.push_back(SeqLength);
	asserta(Ptr <= m_Buffer + FileSize);

	const unsigned SeqCount = SIZE(m_Seqs);
	asserta(SIZE(m_Labels) == SeqCount);
	asserta(SIZE(m_Lengths) == SeqCount);

	unsigned TotalLength = 0;
	unsigned MaxLength = 0;
	m_Aligned = true;
	for (unsigned i = 0; i < SeqCount; ++i)
		{
		unsigned Length = m_Lengths[i];
		if (Length != m_Lengths[0])
			m_Aligned = false;
		if (Length > MaxLength)
			MaxLength = Length;
		m_Lengths.push_back(Length);
		m_FullLengths.push_back(Length);
		TotalLength += Length;
		m_Users.push_back(i);
		m_Weights.push_back(1.0f);
		m_Los.push_back(0);
		m_Strands.push_back(true);
		}

	if (SeqCount == 0)
		ProgressLog("%s: No sequences found\n", FileName.c_str());
	else
		ProgressLog("%s: %u seqs, max len %u, avg len %u\n",
		  FileName.c_str(), SeqCount, MaxLength, TotalLength/SeqCount);

	if (!LongSeqLabels.empty())
		{
		Warning("%u long sequence(s) discarded (max length is %u, use --maxseqlen to change)",
		  SIZE(LongSeqLabels), opt_maxseqlen);
		Log("\n");
		Log("Long sequences discarded:\n");
		Log("    Length  Label\n");
		Log("----------  -----\n");
		for (unsigned i = 0; i < SIZE(LongSeqLabels); ++i)
			Log("%10u  %s\n", LongSeqLengths[i], LongSeqLabels[i].c_str());
		}

	if (FileName != "")
		CloseStdioFile(f);
	}

void SeqDB::ToFasta(const string &FileName) const
	{
	FILE *f = CreateStdioFile(FileName);
	for (unsigned SeqIndex = 0; SeqIndex < GetSeqCount(); ++SeqIndex)
		ToFasta(f, SeqIndex);
	CloseStdioFile(f);
	}

void SeqDB::ToFasta(FILE *f, unsigned SeqIndex) const
	{
	asserta(SeqIndex < SIZE(m_Seqs));
	fprintf(f, ">%s\n", GetLabel(SeqIndex).c_str());
	unsigned L = GetSeqLength(SeqIndex);
	const byte *Seq = GetSeq(SeqIndex);
	unsigned BlockCount = (L + opt_rowlen - 1)/opt_rowlen;
	for (unsigned BlockIndex = 0; BlockIndex < BlockCount; ++BlockIndex)
		{
		unsigned From = BlockIndex*opt_rowlen;
		unsigned To = From + opt_rowlen;
		if (To >= L)
			To = L;
		for (unsigned Pos = From; Pos < To; ++Pos)
			fputc(Seq[Pos], f);
		fputc('\n', f);
		}
	}
