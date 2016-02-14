#include "myutils.h"
#include "msa.h"
#include "tree.h"

void MSA::Clear()
	{
	const unsigned SeqCount = GetSeqCount();
	for (unsigned i = 0; i < SeqCount; ++i)
		myfree(m_Seqs[i]);
	m_Seqs.clear();
	m_Ids.clear();
	m_Labels.clear();
	m_ShortLabels.clear();
	m_IdToSeqIndex.clear();
	m_Name.clear();
	m_ColCount = 0;
	m_SeqDB = 0;
	m_Accuracy = -1.0f;
	}

void MSA::AddSeq(const char *Label, byte *Seq, unsigned ColCount, unsigned Id)
	{
	byte *s = myalloc<byte>(ColCount);
	memcpy(s, Seq, ColCount);
	unsigned SeqCount = SIZE(m_Seqs);
	if (SeqCount == 0)
		m_ColCount = ColCount;
	else if (ColCount != m_ColCount)
		Die("MSA::AddSeq, ColCount=%u m_ColCount=%u", ColCount, m_ColCount);
	if (Id >= SIZE(m_IdToSeqIndex))
		m_IdToSeqIndex.resize(Id+1, UINT_MAX);

	m_Labels.push_back(Label);
	m_Seqs.push_back(s);
	m_Ids.push_back(Id);
	m_IdToSeqIndex[Id] = SeqCount;
	}

unsigned MSA::GetMaxLabelLength() const
	{
	if (m_SeqDB == 0)
		return 0;
	unsigned MaxLabelLength = 0;
	const unsigned SeqCount = GetSeqCount();
	for (unsigned i = 0; i < SeqCount; ++i)
		{
		unsigned Id = GetId(i);
		const char *Label = m_SeqDB->GetLabel(Id);
		unsigned L = unsigned(strlen(Label));
		if (L > MaxLabelLength)
			MaxLabelLength = L;
		}
	return MaxLabelLength;
	}

unsigned MSA::GetMaxShortLabelLength()
	{
	unsigned MaxLabelLength = 0;
	const unsigned SeqCount = GetSeqCount();
	for (unsigned i = 0; i < SeqCount; ++i)
		{
		unsigned Id = GetId(i);
		const char *Label = GetShortLabelByIndex(i);
		unsigned L = unsigned(strlen(Label));
		if (L > MaxLabelLength)
			MaxLabelLength = L;
		}
	return MaxLabelLength;
	}

unsigned MSA::GetLetterCount(unsigned SeqIndex) const
	{
	unsigned Count = 0;
	const byte *s = GetSeqByIndex(SeqIndex);
	for (unsigned i = 0; i < m_ColCount; ++i)
		if (!isgap(s[i]))
			++Count;
	return Count;
	}

void MSA::StripGapCols()
	{
	const unsigned OldColCount = m_ColCount;
	const unsigned SeqCount = GetSeqCount();
	unsigned NewColCount = 0;
	for (unsigned OldColIndex = 0; OldColIndex < OldColCount; ++OldColIndex)
		{
		bool AllGaps = true;
		for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
			{
			if (!isgap(m_Seqs[SeqIndex][OldColIndex]))
				{
				AllGaps = false;
				break;
				}
			}
		if (AllGaps)
			continue;
		if (NewColCount != OldColIndex)
			{
			assert(NewColCount < OldColIndex);
			for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
				m_Seqs[SeqIndex][NewColCount] = m_Seqs[SeqIndex][OldColIndex];
			}
		++NewColCount;
		}
	m_ColCount = NewColCount;
	}

void MSA::Copy(const MSA &rhs)
	{
	Clear();

	m_ColCount = rhs.m_ColCount;
	m_Seqs.clear();
	const unsigned SeqCount = rhs.GetSeqCount();
	for (unsigned i = 0; i < SeqCount; ++i)
		{
		byte *s = myalloc<byte>(m_ColCount);
		memcpy(s, rhs.m_Seqs[i], m_ColCount);
		m_Seqs.push_back(s);
		}
	m_Ids = rhs.m_Ids;
	m_IdToSeqIndex  = rhs.m_IdToSeqIndex;
	m_SeqDB = rhs.m_SeqDB;
	m_Labels = rhs.m_Labels;
#if	DEBUG
	Validate();
#endif
	}

unsigned MSA::GetMaxId() const
	{
	const unsigned SeqCount = SIZE(m_Ids);
	unsigned MaxId = 0;
	for (unsigned i = 0; i < SeqCount; ++i)
		MaxId = max(MaxId, m_Ids[i]);
	return MaxId;
	}

void MSA::CopySubset(const MSA &rhs, vector<unsigned> &Ids)
	{
	Clear();

	m_ColCount = rhs.m_ColCount;
	m_Seqs.clear();
	const unsigned SeqCount = SIZE(Ids);
	unsigned MaxId = 0;
	for (unsigned i = 0; i < SeqCount; ++i)
		MaxId = max(MaxId, Ids[i]);
	m_IdToSeqIndex.clear();
	m_IdToSeqIndex.resize(MaxId+1, UINT_MAX);
	for (unsigned i = 0; i < SeqCount; ++i)
		{
		unsigned Id = Ids[i];
		byte *s = myalloc<byte>(m_ColCount);
		const byte *Seq = rhs.GetSeqById(Id);
		unsigned rhsSeqIndex = rhs.GetIndex(Id);
		const char *Label = rhs.GetLabelByIndex(rhsSeqIndex);
		memcpy(s, Seq, m_ColCount);
		m_Seqs.push_back(s);
		m_Labels.push_back(Label);
		m_IdToSeqIndex[Id] = i;
		}
	m_Ids = Ids;
	m_SeqDB = rhs.m_SeqDB;
#if	DEBUG
	Validate();
#endif
	}

void MSA::SetColCase(const vector<bool> &Upper)
	{
	unsigned SeqCount = GetSeqCount();
	unsigned ColCount = GetColCount();
	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{
		byte *Seq = m_Seqs[SeqIndex];
		for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
			{
			char c = Seq[ColIndex];
			if (Upper[ColIndex])
				Seq[ColIndex] = toupper(c);
			else
				Seq[ColIndex] = tolower(c);
			}
		}
	}

void MSA::Validate() const
	{
	if (m_SeqDB == 0)
		return;
	const unsigned SeqCount = SIZE(m_Seqs);
	for (unsigned i = 0; i < SeqCount; ++i)
		{
		unsigned Id = GetId(i);
		unsigned L = m_SeqDB->GetSeqLength(Id);
		asserta(GetLetterCount(i) == L);
		unsigned Pos = 0;
		const byte *DBSeq = m_SeqDB->GetSeq(Id);
		const byte *MSASeq = m_Seqs[i];
		for (unsigned j = 0; j < m_ColCount; ++j)
			{
			byte c = MSASeq[j];
			if (isgap(c))
				continue;
			asserta(toupper(c) == toupper(DBSeq[Pos++]));
			}
		}
	}

void MSA::ToFasta(const string &FileName)
	{
	FILE *f = CreateStdioFile(FileName);

	unsigned SeqCount = GetSeqCount();
	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{
		unsigned Id = GetId(SeqIndex);
		const char *Label = GetLabelByIndex(SeqIndex);
		fprintf(f, ">%s\n", Label);
		fprintf(f, "%*.*s\n", m_ColCount, m_ColCount, m_Seqs[SeqIndex]);
		}

	CloseStdioFile(f);
	}

static bool MSASortOnTreeNode(const Tree &t, unsigned NodeIndex, void *ptrIds)
	{
	if (t.IsInternal(NodeIndex))
		return true;

	unsigned Id = t.GetUser(NodeIndex);

	vector<unsigned> &Ids = *((vector<unsigned> *) ptrIds);
	Ids.push_back(Id);
	return true;
	}

void MSA::SortByTree(Tree &t)
	{
	vector<unsigned> Ids;
	t.Traverse(MSASortOnTreeNode, &Ids);
	SortByIds(Ids);
	}

void MSA::SortByIds(const vector<unsigned> &Ids)
	{
	vector<byte *> SortedSeqs;
	vector<unsigned> SortedIdToSeqIndex;

	vector<byte *> OldSeqs = m_Seqs;
	vector<unsigned> OldIds = m_Ids;
	vector<unsigned> OldIdToSeqIndex = m_IdToSeqIndex;

	const unsigned SeqCount = SIZE(m_Seqs);
	asserta(SIZE(Ids) == SeqCount);
	for (unsigned i = 0; i < SeqCount; ++i)
		{
		unsigned Id = Ids[i];
		unsigned Index = OldIdToSeqIndex[Id];

		m_Ids[i] = Id;
		m_Seqs[i] = OldSeqs[Index];
		m_IdToSeqIndex[Id] = i;
		}
	}

void MSA::GetCol(unsigned ColIndex, string &Col) const
	{
	Col.clear();
	if (ColIndex >= m_ColCount)
		Die("MSA::GetCol(%u), %u cols", ColIndex, m_ColCount);
	const unsigned SeqCount = GetSeqCount();
	for (unsigned i = 0; i < SeqCount; ++i)
		Col.push_back(m_Seqs[i][ColIndex]);
	}

float MSA::GetPctIdPair(unsigned Index1, unsigned Index2) const
	{
	byte *Seq1 = GetSeqByIndex(Index1);
	byte *Seq2 = GetSeqByIndex(Index2);
	const unsigned ColCount = GetColCount();
	unsigned PairCount = 0;
	unsigned IdCount = 0;
	for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
		{
		char Letter1 = Seq1[ColIndex];
		char Letter2 = Seq2[ColIndex];
		if (isgap(Letter1) || isgap(Letter2))
			continue;
		++PairCount;
		if (toupper(Letter1) == toupper(Letter2))
			++IdCount;
		}
	return PairCount == 0 ? 0.0f : float(IdCount)*100.0f/float(PairCount);
	}

float MSA::GetAvgPctId() const
	{
	const unsigned SeqCount = GetSeqCount();
	float TotalId = 0.0f;
	unsigned PairCount = 0;
	for (unsigned i = 0; i < SeqCount; ++i)
		for (unsigned j = i+1; j < SeqCount; ++j)
			{
			TotalId += GetPctIdPair(i, j);
			++PairCount;
			}
	return PairCount == 0 ? 0.0f : TotalId/PairCount;
	}

void MSA::FromAlignedDB(SeqDB &DB)
	{
	if (!DB.m_Aligned)
		Die("MSA::FromAlignedDB, not aligned");

	Clear();
	m_SeqDB = &DB;
	const unsigned SeqCount = DB.GetSeqCount();
	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{
		const unsigned L = DB.GetSeqLength(SeqIndex);
		byte *Seq = DB.GetSeq(SeqIndex);
		const char *Label = DB.GetLabel(SeqIndex);
		AddSeq(Label, Seq, L, SeqIndex);
		}
	}

unsigned MSA::GetSeqLength(unsigned Index) const
	{
	const byte *Seq = GetSeqByIndex(Index);
	const unsigned ColCount = GetColCount();
	unsigned Length = 0;
	for (unsigned i = 0; i < ColCount; ++i)
		if (!isgap(Seq[i]))
			++Length;
	return Length;
	}

unsigned MSA::GetMaxSeqLength() const
	{
	const unsigned SeqCount = GetSeqCount();
	unsigned MaxLength = 0;
	for (unsigned i = 0; i < SeqCount; ++i)
		{
		unsigned Length = GetSeqLength(i);
		if (Length > MaxLength)
			MaxLength = Length;
		}
	return MaxLength;
	}

const char *MSA::GetShortLabelByIndex(unsigned Index) 
	{
	if (m_ShortLabels.empty())
		m_ShortLabels.resize(GetSeqCount());

	string &ShortLabel = m_ShortLabels[Index];
	if (!ShortLabel.empty())
		return ShortLabel.c_str();

	const char *Label = GetLabelByIndex(Index);
	if (strlen(Label) <= opt_maxlabel)
		ShortLabel = Label;
	else
		for (unsigned i = 0; i < opt_maxlabel; ++i)
			ShortLabel.push_back(Label[i]);
	return ShortLabel.c_str();
	}
