#include <ctype.h>
#include "myutils.h"
#include "seqdb.h"
#include "isgap.h"
#include "regex.h"
#include "params.h"

void RevComp(byte *Seq, unsigned L);
void RevComp(string &s);

/***
SeqDB (Sequence Database) is a general-purpose container for
sets of aligned or unaligned sequences. If m_Aligned is true
then this is a multiple alignment.

The sequence data itself may be stored in this or elsewhere, e.g.
in some other SeqDB object. Memory is managed via the m_Buffer
member; if it is non-zero, some sequence data is stored in this
object and that memory will be freed when this object is destroyed.

A SeqDB may hold a set of match posterior matrices (m_SPPs) and
other associated data such as a guide tree.
***/

SeqDB::SeqDB()
	{
	Clear();
	}

void SeqDB::Clear()
	{
	ClearSPPs();
	m_Name.clear();
	m_Labels.clear();
	m_Seqs.clear();
	m_Lengths.clear();
	m_FullLengths.clear();
	m_Weights.clear();
	m_Users.clear();
	m_AccMxf.Clear();
	m_IdMxf.Clear();
	m_Los.clear();
	m_Strands.clear();

	m_Buffer = 0;
	m_Aligned = false;
	m_BufferSize = 0;
	m_BufferPos = 0;
	m_Accuracy = -1.0f;
	}

void SeqDB::ClearSPPs()
	{
	const unsigned N = SIZE(m_SPPs);
	for (unsigned i = 0; i < N; ++i)
		delete m_SPPs[i];
	m_SPPs.clear();
	}

SeqDB::~SeqDB()
	{
	Clear();
	}

static bool isDNA(byte Letter)
	{
	Letter = byte(toupper(Letter));
	return Letter == 'A' || Letter == 'C' || Letter == 'G' || Letter == 'T' || Letter == 'N';
	}

static bool isRNA(byte Letter)
	{
	Letter = byte(toupper(Letter));
	return Letter == 'A' || Letter == 'C' || Letter == 'G' || Letter == 'U' || Letter == 'N';
	}

SEQ_TYPE SeqDB::GetSeqType() const
	{
	return GuessSeqType();
	}

SEQ_TYPE SeqDB::GuessSeqType() const
	{
	const unsigned TryCount = 100;
	unsigned SeqCount = SIZE(m_Seqs);
	if (SeqCount == 0)
		return ST_Amino;

	unsigned DNACount = 0;
	unsigned RNACount = 0;
	unsigned NonNucCount = 0;

	for (unsigned Try = 0; Try < TryCount; ++Try)
		{
		unsigned Id = unsigned(rand()%SeqCount);
		const byte *Seq = m_Seqs[Id];
		unsigned L = GetSeqLength(Id);
		if (L == 0)
			continue;
		unsigned Pos = unsigned(rand()%L);
		byte Letter = Seq[Pos];
		bool DNA = isDNA(Letter);
		bool RNA = isRNA(Letter);
		if (DNA)
			++DNACount;
		if (RNA)
			++RNACount;
		if (!DNA && !RNA)
			++NonNucCount;
		}
	SEQ_TYPE SeqType = ST_Unknown;
	if (NonNucCount > DNACount && NonNucCount > RNACount)
		SeqType = ST_Amino;
	else if (DNACount > NonNucCount && DNACount > RNACount)
		SeqType = ST_DNA;
	else
		SeqType = ST_RNA;
	return SeqType;
	}

void SeqDB::AddSeq(const string &Label, byte *Seq, unsigned L, float Weight,
  unsigned User, unsigned Lo, unsigned FullLength, bool Strand)
	{
	m_Aligned = (m_Seqs.empty() || L == m_Lengths[0]);

	if (Lo > 0 && FullLength == UINT_MAX)
		Die("SeqDB::AddSeq, FullLength not set");

	if (FullLength == UINT_MAX)
		FullLength = L;

	m_Labels.push_back(Label);
	m_Seqs.push_back(Seq);
	m_Lengths.push_back(L);
	m_Weights.push_back(Weight);
	m_Users.push_back(User);
	m_Los.push_back(Lo);
	m_FullLengths.push_back(FullLength);
	m_Strands.push_back(Strand);
	}

unsigned SeqDB::GetMaxHi() const
	{
	unsigned MaxHi = 0;
	const unsigned SeqCount = GetSeqCount();
	for (unsigned Id = 0; Id < SeqCount; ++Id)
		{
		unsigned Hi = GetHi(Id);
		if (Hi > MaxHi)
			MaxHi = Hi;
		}
	return MaxHi;
	}

unsigned SeqDB::GetMaxMAFPos() const
	{
	unsigned MaxPos = 0;
	const unsigned SeqCount = GetSeqCount();
	for (unsigned Id = 0; Id < SeqCount; ++Id)
		{
		unsigned Pos = GetMAFPos(Id);
		if (Pos > MaxPos)
			MaxPos = Pos;
		}
	return MaxPos;
	}

void SeqDB::Reserve(unsigned Bytes)
	{
	Clear();

	m_Buffer = myalloc<byte>(Bytes);
	m_BufferSize = Bytes;
	m_BufferPos = 0;
	}

unsigned SeqDB::AppendSeq(const SeqDB &DB, unsigned SeqIndex)
	{
	const byte *Seq = DB.GetSeq(SeqIndex);
	const string &Label = DB.GetLabel(SeqIndex);
	float Weight = DB.GetSeqWeight(SeqIndex);
	unsigned L = DB.GetSeqLength(SeqIndex);
	unsigned User = DB.GetUser(SeqIndex);
	return AppendSeq(Label, Seq, L, Weight, User);
	}

unsigned SeqDB::AppendSeq(const string &Label, const byte *Seq, unsigned L,
  float Weight, unsigned User, unsigned Lo, unsigned FullLength, bool Strand)
	{
	if (Lo > 0 && FullLength == UINT_MAX)
		Die("SeqDB::AddSeq, FullLength not set");

	if (FullLength == UINT_MAX)
		FullLength = L;

	m_Aligned = (m_Seqs.empty() || L == m_Lengths[0]);
	unsigned Index = SIZE(m_Seqs);
	unsigned Bytes = L + 1;
	if (m_BufferPos + Bytes > m_BufferSize)
		{
		m_BufferSize = m_BufferPos + L;
		byte *NewBuffer = (byte *) realloc(m_Buffer, m_BufferSize);
		if (NewBuffer == 0)
			Die("Out of memory");

		size_t Diff = (NewBuffer - m_Buffer);
		for (unsigned i = 0; i < GetSeqCount(); ++i)
			m_Seqs[i] += Diff;

		m_Buffer = NewBuffer;
		}

	byte *BuffSeq = (byte *) m_Buffer + m_BufferPos;
	memcpy(BuffSeq, Seq, L);
	m_BufferPos += L;

	m_Labels.push_back(Label);
	m_Seqs.push_back(BuffSeq);
	m_Weights.push_back(Weight);
	m_Lengths.push_back(L);
	m_Users.push_back(User);
	m_Los.push_back(Lo);
	m_FullLengths.push_back(FullLength);
	m_Strands.push_back(Strand);

	return Index;
	}

unsigned SeqDB::GetSeqBytes() const
	{
	unsigned Bytes = 0;
	const unsigned SeqCount = GetSeqCount();
	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{
		const unsigned SeqLength = GetSeqLength(SeqIndex);
		Bytes += SeqLength;
		}
	return Bytes;
	}

void SeqDB::SetUser(unsigned SeqIndex, unsigned User)
	{
	assert(SeqIndex < SIZE(m_Users));
	m_Users[SeqIndex] = User;
	}

SparseMx &SeqDB::GetSPP(unsigned SeqIndex1, unsigned SeqIndex2, bool &Transpose)
	{
	unsigned PairIndex = GetPairIndex(SeqIndex1, SeqIndex2, Transpose);
	asserta(PairIndex < SIZE(m_SPPs));
	return *m_SPPs[PairIndex];
	}

void SeqDB::GetPP(unsigned SeqIndex1, unsigned SeqIndex2, Mx<float> &PP)
	{
	bool Transpose;
	SparseMx &SPP = GetSPP(SeqIndex1, SeqIndex2, Transpose);
	asserta(!Transpose);
	SPP.ToMxf(PP);
	}

void SeqDB::GetPP(unsigned SeqIndex1, unsigned SeqIndex2, Mx<float> &PP,
  bool &Transpose)
	{
	SparseMx &SPP = GetSPP(SeqIndex1, SeqIndex2, Transpose);
	SPP.ToMxf(PP);
	}

byte SeqDB::Get(unsigned SeqIndex, unsigned ColIndex) const
	{
	assert(ColIndex < GetSeqLength(SeqIndex));
	const byte *Seq = GetSeq(SeqIndex);
	return Seq[ColIndex];
	}

void SeqDB::GetCol(unsigned ColIndex, string &Col) const
	{
	Col.clear();
	asserta(ColIndex < GetColCount());
	for (unsigned SeqIndex = 0; SeqIndex < GetSeqCount(); ++SeqIndex)
		Col.push_back(m_Seqs[SeqIndex][ColIndex]);
	}

void SeqDB::CopySubset(const SeqDB &rhs, const vector<unsigned> &SeqIndexes)
	{
	Clear();
	const unsigned N = SIZE(SeqIndexes);
	for (unsigned i = 0; i < N; ++i)
		{
		unsigned SeqIndex = SeqIndexes[i];
		const byte *Seq = rhs.GetSeq(SeqIndex);
		const string &Label = rhs.GetLabel(SeqIndex);
		unsigned L = rhs.GetSeqLength(SeqIndex);
		float Weight = rhs.GetSeqWeight(SeqIndex);
		unsigned User = rhs.GetUser(SeqIndex);
		AppendSeq(Label, Seq, L, Weight, User);
		}
	m_Aligned = rhs.m_Aligned;
	}

void SeqDB::Copy(const SeqDB &rhs)
	{
	Clear();
	const unsigned N = rhs.GetSeqCount();
	for (unsigned SeqIndex = 0; SeqIndex < N; ++SeqIndex)
		{
		const byte *Seq = rhs.GetSeq(SeqIndex);
		const string &Label = rhs.GetLabel(SeqIndex);
		unsigned L = rhs.GetSeqLength(SeqIndex);
		float Weight = rhs.GetSeqWeight(SeqIndex);
		unsigned User = rhs.GetUser(SeqIndex);
		AppendSeq(Label, Seq, L, Weight, User);
		}
	m_Aligned = rhs.m_Aligned;
	}

float SeqDB::GetPctId(unsigned SeqIndex1, unsigned SeqIndex2) const
	{
	if (!m_Aligned)
		Die("SeqDB::GetPctId, not aligned");

	const byte *Seq1 = GetSeq(SeqIndex1);
	const byte *Seq2 = GetSeq(SeqIndex2);
	const unsigned ColCount = GetColCount();
	unsigned IdCount = 0;
	unsigned PairCount = 0;
	for (unsigned i = 0; i < ColCount; ++i)
		{
		byte a = Seq1[i];
		byte b = Seq2[i];
		if (isgap(a) || isgap(b))
			continue;
		++PairCount;
		if (toupper(a) == toupper(b))
			++IdCount;
		}
	if (PairCount == 0)
		return 0.0f;
	return float(IdCount)*100.0f/float(PairCount);
	}

float SeqDB::GetAvgPctId() const
	{
	const unsigned SeqCount = GetSeqCount();
	if (SeqCount == 0)
		return 0.0f;
	float Sum = 0.0f;
	for (unsigned i = 0; i < SeqCount; ++i)
		for (unsigned j = i + 1; j < SeqCount; ++j)
			Sum += GetPctId(i, j);
	return Sum/GetPairCount();
	}

void SeqDB::StripGapCols()
	{
	const unsigned OldColCount = GetColCount();
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
	for (unsigned i = 0; i < SeqCount; ++i)
		m_Lengths[i] = NewColCount;
	}

unsigned SeqDB::GetMaxShortLabelLength() const
	{
	unsigned MaxLength = 0;
	for (unsigned i = 0; i < GetSeqCount(); ++i)
		{
		string s;
		GetShortLabel(i, s);
		unsigned L = SIZE(s);
		if (L > MaxLength)
			MaxLength = L;
		}
	return MaxLength;
	}

unsigned SeqDB::GetMaxUngappedSeqLength() const
	{
	unsigned MaxLength = 0;
	for (unsigned i = 0; i < GetSeqCount(); ++i)
		{
		unsigned L = GetUngappedSeqLength(i);
		if (L > MaxLength)
			MaxLength = L;
		}
	return MaxLength;
	}

unsigned SeqDB::GetMaxFullLength() const
	{
	unsigned MaxLength = 0;
	for (unsigned i = 0; i < GetSeqCount(); ++i)
		{
		unsigned L = GetFullLength(i);
		if (L > MaxLength)
			MaxLength = L;
		}
	return MaxLength;
	}

void SeqDB::SetColCase(const vector<bool> &Upper)
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

void SeqDB::GetPctIdMx(Mx<float> &Distf) const
	{
	const unsigned SeqCount = GetSeqCount();
	Distf.Alloc("Dist_PctId", SeqCount, SeqCount);
	float **Dist = Distf.GetData();
	for (unsigned i = 0; i < SeqCount; ++i)
		{
		Dist[i][i] = 100.0f;
		for (unsigned j = i+1; j < SeqCount; ++j)
			{
			float PctId = GetPctId(i, j);
			Dist[i][j] = PctId;
			Dist[j][i] = PctId;
			}
		}
	}

void SeqDB::RevComp(unsigned SeqIndex)
	{
	byte *Seq = GetSeq(SeqIndex);
	const unsigned L = GetSeqLength(SeqIndex);
	::RevComp(Seq, L);
	}

void SeqDB::RevComp()
	{
	for (unsigned i = 0; i < GetSeqCount(); ++i)
		RevComp(i);
	}

void SeqDB::ComputeAccsAndIds(vector<float> &AvgAccs, vector<float> &FractIds,
  float &Acc, float &FractId)
	{
	ComputeAccAndIdMxs();
	Acc = m_AccMxf.GetOffDiagAvgs(AvgAccs);
	FractId = m_IdMxf.GetOffDiagAvgs(FractIds);
	}

unsigned SeqDB::GetSeqIndex(const string &Label) const
	{
	const unsigned SeqCount = GetSeqCount();
	for (unsigned i = 0; i < SeqCount; ++i)
		if (m_Labels[i] == Label)
			return i;
	Die("SeqDB::GetSeqIndex(%.16s), not found", Label.c_str());
	ureturn(UINT_MAX);
	}

unsigned SeqDB::ColToPos(unsigned SeqIndex, unsigned Col) const
	{
	unsigned ColCount = GetColCount();
	asserta(Col < ColCount);
	const byte *Seq = GetSeq(SeqIndex);
	unsigned Pos = UINT_MAX;
	if (GetStrand(SeqIndex))
		{
		unsigned Pos = GetLo(SeqIndex);
		for (unsigned i = 0; i < Col; ++i)
			if (!isgap(Seq[i]))
				++Pos;
		return Pos;
		}
	else
		{
		Pos = GetLo(SeqIndex);
		for (int i = (int) ColCount - 1; i >= 0; --i)
			{
			if (i == (int) Col)
				return Pos;
			if (!isgap(Seq[i]))
				++Pos;
			}
		}
	return UINT_MAX;
	}

unsigned SeqDB::GetHi(unsigned SeqIndex) const
	{
	return GetLo(SeqIndex) + GetUngappedSeqLength(SeqIndex) - 1;
	}

unsigned SeqDB::GetMAFPos(unsigned SeqIndex) const
	{
	if (GetStrand(SeqIndex))
		return GetLo(SeqIndex);
	else
		return GetFullLength(SeqIndex) - GetHi(SeqIndex) - 1;
	}

unsigned SeqDB::PosToCol(unsigned SeqIndex, unsigned Pos) const
	{
	const unsigned ColCount = GetColCount();
	const byte *Seq = GetSeq(SeqIndex);
	unsigned Pos2 = UINT_MAX;
	for (unsigned Col = 0; Col < ColCount; ++Col)
		{
		if (isgap(Seq[Col]))
			continue;
		if (Pos2 == UINT_MAX)
			Pos2 = 0;
		else
			++Pos2;
		if (Pos2 == Pos)
			return Col;
		}
	return UINT_MAX;
	}

static bool SeqDBSortOnTreeNode(const Tree &t, unsigned NodeIndex, void *ptrUsers)
	{
	if (t.IsInternal(NodeIndex))
		return true;

	unsigned User = t.GetUser(NodeIndex);

	vector<unsigned> &Users = *((vector<unsigned> *) ptrUsers);
	Users.push_back(User);
	return true;
	}

void SeqDB::SortByTree(Tree &t)
	{
	vector<unsigned> Users;
	t.Traverse(SeqDBSortOnTreeNode, &Users);

	const unsigned SeqCount = GetSeqCount();
	vector<unsigned> UserToSeqIndex(SeqCount, UINT_MAX);
	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{
		unsigned User = m_Users[SeqIndex];
		asserta(User < SeqCount);
		asserta(UserToSeqIndex[User] == UINT_MAX);
		UserToSeqIndex[User] = SeqIndex;
		}

	vector<unsigned> SortOrder;
	for (unsigned i = 0; i < SeqCount; ++i)
		{
		unsigned User = Users[i];
		unsigned SeqIndex = UserToSeqIndex[User];
		SortOrder.push_back(SeqIndex);
		}

	Sort(SortOrder);
	}

void SeqDB::Sort(const vector<unsigned> &SortOrder)
	{
	ClearSPPs();
	m_AccMxf.Clear();
	m_IdMxf.Clear();

	const unsigned SeqCount = GetSeqCount();
	asserta(SIZE(SortOrder) == SeqCount);

	vector<byte *> OldSeqs = m_Seqs;
	vector<string> OldLabels = m_Labels;
	vector<unsigned> OldUsers = m_Users;
	vector<unsigned> OldLengths = m_Lengths;
	vector<float> OldWeights = m_Weights;

	for (unsigned i = 0; i < SeqCount; ++i)
		{
		unsigned j = SortOrder[i];
		asserta(j < SeqCount);
		m_Seqs[i] = OldSeqs[j];
		m_Labels[i] = OldLabels[j];
		m_Users[i] = OldUsers[j];
		m_Lengths[i] = OldLengths[j];
		m_Weights[i] = OldWeights[j];
		}
	}

void SeqDB::SortByUser()
	{
	const unsigned SeqCount = GetSeqCount();
	vector<unsigned> SortOrder(SeqCount, UINT_MAX);
	for (unsigned i = 0; i < SeqCount; ++i)
		{
		unsigned User = m_Users[i];
		asserta(User < SeqCount);
		asserta(SortOrder[User] == UINT_MAX);
		SortOrder[User] = i;
		}
	Sort(SortOrder);
	}

void SeqDB::StripGaps(unsigned SeqIndex)
	{
	unsigned L = GetSeqLength(SeqIndex);
	byte *Seq = GetSeq(SeqIndex);
	unsigned LNew = 0;
	for (unsigned i = 0; i < L; ++i)
		{
		char c = Seq[i];
		if (!isgap(c))
			Seq[LNew++] = c;
		}
	m_Lengths[SeqIndex] = LNew;
	}

unsigned SeqDB::GetUngappedSeqLength(unsigned SeqIndex) const
	{
	unsigned L = GetSeqLength(SeqIndex);
	const byte *Seq = GetSeq(SeqIndex);
	unsigned U = 0;
	for (unsigned i = 0; i < L; ++i)
		if (!isgap(Seq[i]))
			++U;
	return U;
	}

void SeqDB::GetShortLabel(unsigned Id, string &ShortLabel) const
	{
	ShortLabel.clear();

	const string &Label = GetLabel(Id);
	if (opt_labelregex != "")
		{
		re_comp(opt_labelregex.c_str());

		bool Matched = re_exec(Label.c_str());
		if (Matched)
			{
			unsigned L = GetGroupLength(1);
			if (L > 0)
				{
				const char *Grp = GetGroupStart(1);
				ShortLabel.reserve(L);
				for (unsigned i = 0; i < L; ++i)
					ShortLabel.push_back(Grp[i]);
				return;
				}
			}
		}

	if (SIZE(Label) <= opt_maxlabel)
		ShortLabel = Label;
	else
		for (unsigned i = 0; i < opt_maxlabel; ++i)
			ShortLabel.push_back(Label[i]);
	}

static void GetPosFromLabel(const string &Label, const string &RegEx,
  string &ShortLabel, unsigned &Lo, unsigned &FullLength)
	{
	ShortLabel = Label;
	Lo = 0;
	FullLength = UINT_MAX;

	re_comp(RegEx.c_str());

	bool Matched = re_exec(Label.c_str());
	if (!Matched)
		goto Fail;

	{
	unsigned L = GetGroupLength(1);
	if (L == 0)
		goto Fail;

	ShortLabel.clear();
	const char *Grp = GetGroupStart(1);
	for (unsigned i = 0; i < L; ++i)
		ShortLabel.push_back(Grp[i]);
	}

	{
	unsigned L = GetGroupLength(2);
	if (L == 0)
		goto Fail;
	string Tmp;
	const char *Grp = GetGroupStart(2);
	for (unsigned i = 0; i < L; ++i)
		Tmp.push_back(Grp[i]);
	Lo = unsigned(atoi(Tmp.c_str()));
	}

	{
	unsigned L = GetGroupLength(3);
	if (L == 0)
		goto Fail;

	string Tmp;
	const char *Grp = GetGroupStart(3);
	for (unsigned i = 0; i < L; ++i)
		Tmp.push_back(Grp[i]);
	FullLength = unsigned(atoi(Tmp.c_str()));
	}

	return;

Fail:
	if (Label.size() <= 16)
		Warning("Label '%.16s' does not match pos regex", Label.c_str());
	else
		Warning("Label '%.16s...' does not match pos regex", Label.c_str());

	ShortLabel = Label;
	Lo = 0;
	FullLength = UINT_MAX;
	}

void SeqDB::SetPosFromLabels(const string &RegEx)
	{
	for (unsigned i = 0; i < GetSeqCount(); ++i)
		{
		const string &Label = GetLabel(i);
		unsigned Lo;
		string ShortLabel;
		unsigned FullLength;
		GetPosFromLabel(Label, RegEx, ShortLabel, Lo, FullLength);
		m_Los[i] = Lo;
		m_FullLengths[i] = FullLength;
		m_Labels[i] = ShortLabel;
		}
	}

static bool MatchLabel(const string &TreeLabel, const string &SeqLabel)
	{
	if (!opt_treeregex)
		return TreeLabel == SeqLabel;
	re_comp(TreeLabel.c_str());
	return re_exec(SeqLabel.c_str());
	}

void SeqDB::BindTree(Tree &t) const
	{
	unsigned SeqCount = GetSeqCount();
	unsigned NodeCount = t.GetNodeCount();

	for (unsigned NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
		{
		if (t.IsLeaf(NodeIndex))
			continue;
		t.SetUser(NodeIndex, UINT_MAX);
		}

	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{		
		bool Found = false;
		const string &SeqLabel = GetLabel(SeqIndex);
		for (unsigned NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
			{
			if (!t.IsLeaf(NodeIndex))
				continue;
			const string &LeafLabel = t.GetLabel(NodeIndex);
			if (MatchLabel(LeafLabel, SeqLabel))
				{
				Found = true;
				t.SetUser(NodeIndex, SeqIndex);
				break;
				}
			}
		if (!Found)
			Die("Sequence '%.16s' not found in tree", SeqLabel.c_str());
		}

	vector<string> MissingLeafLabels;
	for (unsigned NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
		{
		if (!t.IsLeaf(NodeIndex))
			continue;
		if (t.GetUser(NodeIndex) == UINT_MAX)
			MissingLeafLabels.push_back(t.GetLabel(NodeIndex));
		}

	for (unsigned i = 0; i < SIZE(MissingLeafLabels); ++i)
		{
		const string &Label = MissingLeafLabels[i];
		unsigned NodeIndex = t.GetNodeIndex(Label);
		t.DeleteLeaf(NodeIndex);
		}
	}

void SeqDB::FromColRange(const SeqDB &DB, unsigned FromCol, unsigned ToCol)
	{
	Clear();
	const unsigned SeqCount = DB.GetSeqCount();
	const unsigned ColCount = DB.GetColCount();
	asserta(FromCol <= ToCol);
	asserta(ToCol < ColCount);
	unsigned L = ToCol - FromCol + 1;
	for (unsigned Id = 0; Id < SeqCount; ++Id)
		{
		byte *Seq = DB.GetSeq(Id);
		const string &Label = DB.GetLabel(Id);
		unsigned FullLength = DB.GetFullLength(Id);
		float Weight = DB.GetSeqWeight(Id);
		bool Strand = DB.GetStrand(Id);
		unsigned User = DB.GetUser(Id);
		unsigned Lo = Strand ? DB.ColToPos(Id, FromCol) : DB.ColToPos(Id, ToCol);

		AddSeq(Label, Seq+FromCol, L, Weight, User, Lo, FullLength, Strand);
		}
	}

void SeqDB::FromSeq(const SeqDB &DB, unsigned Id)
	{
	Clear();
	byte *Seq = DB.GetSeq(Id);
	const string &Label = DB.GetLabel(Id);
	unsigned L = DB.GetSeqLength(Id);
	unsigned FullLength = DB.GetFullLength(Id);
	float Weight = DB.GetSeqWeight(Id);
	unsigned Lo = DB.GetLo(Id);
	bool Strand = DB.GetStrand(Id);
	unsigned User = DB.GetUser(Id);

	AddSeq(Label, Seq, L, Weight, User, Lo, FullLength, Strand);
	}

static char ConsCountsToSymbol(unsigned MaxCount, unsigned LetterCount)
	{
	if (LetterCount < 2)
		return ' ';
	if (MaxCount == LetterCount)
		return '*';
	double f = double(MaxCount)/double(LetterCount);
	if (f >= 0.75)
		return ':';
	if (f >= 0.5 && LetterCount > 2)
		return '.';
	return ' ';
	}

static char GetMatchSymbolNucleo(byte Letter1, byte Letter2)
	{
	if (isgap(Letter1) || isgap(Letter2))
		return ' ';
	Letter1 = toupper(Letter1);
	Letter2 = toupper(Letter2);
	if (Letter1 == Letter2)
		return '*';
	return ' ';
	}

static char GetMatchSymbolAmino(byte Letter1, byte Letter2)
	{
	float **SubstMx = GetSubstMx();
	if (isgap(Letter1) || isgap(Letter2))
		return ' ';
	Letter1 = toupper(Letter1);
	Letter2 = toupper(Letter2);
	if (Letter1 == Letter2)
		return '*';

	float Score = SubstMx[Letter1][Letter2];
	if (Score > 0.2)
		return '+';
	else if (Score > -0.5)
		return ' ';
	return '?';
	}

void SeqDB::GetSumLine(string &Line, unsigned FromCol, unsigned ToCol) const
	{
	Line.clear();

	if (ToCol == UINT_MAX)
		ToCol = GetColCount() - 1;
	else
		asserta(ToCol < GetColCount());

	const unsigned SeqCount = GetSeqCount();
	if (SeqCount == 2)
		{
		const byte *A = GetSeq(0);
		const byte *B = GetSeq(1);
		bool Nuc = IsNucleo();
		for (unsigned Col = FromCol; Col <= ToCol; ++Col)
			{
			byte a = A[Col];
			byte b = B[Col];
			char c = Nuc ? GetMatchSymbolNucleo(a, b) : GetMatchSymbolAmino(a, b);
			Line.push_back(c);
			}
		return;
		}

	for (unsigned ColIndex = FromCol; ColIndex <= ToCol; ++ColIndex)
		{
		vector<unsigned> LetterCounts(256, 0);
		unsigned LetterCount = 0;
		unsigned MaxCount = 0;
		for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
			{
			const byte *Seq = GetSeq(SeqIndex);
			int c = Seq[ColIndex];
			if (isgap((char) c) || iswildcard((char) c))
				continue;
			c = toupper(c);
			if (c >= 0 && c < 256)
				{
				++LetterCount;
				unsigned NewCount = LetterCounts[c] + 1;
				LetterCounts[c] = NewCount;
				if (NewCount > MaxCount)
					MaxCount = NewCount;
				}
			}
		char c = ConsCountsToSymbol(MaxCount, LetterCount);
		Line.push_back(c);
		}
	}

// Compare sequences in two SeqDBs.
// Tyical use is DB=input sequences, this=MAF block or other subset.
// Require sequence in this <= sequence in DB.
void SeqDB::Validate(const SeqDB &DB) const
	{
	const unsigned SeqCount = GetSeqCount();
	for (unsigned Id = 0; Id < SeqCount; ++Id)
		{
		const string &Label = GetLabel(Id);
		unsigned Id2 = DB.GetSeqIndex(Label);

		bool Strand = GetStrand(Id);
		bool Strand2 = DB.GetStrand(Id2);

		string Seq;
		string Seq2;
		GetUngappedSeq(Id, Seq);
		DB.GetUngappedSeq(Id2, Seq2);

		if (!Strand)
			::RevComp(Seq);
		if (!Strand2)
			::RevComp(Seq2);

		unsigned Lo = GetLo(Id);
		unsigned Lo2 = DB.GetLo(Id2);

		unsigned L = SIZE(Seq);
		unsigned L2 = SIZE(Seq2);

		unsigned Hi = Lo + L - 1;
		unsigned Hi2 = Lo2 + L2 - 1;

		asserta(Lo >= Lo2);
		asserta(Hi <= Hi2);

		for (unsigned i = 0; i < L; ++i)
			{
			char c = Seq[i];
			unsigned Pos = Lo + i;
			unsigned i2 = Pos - Lo2;
			char c2 = Seq2[i2];
			if (toupper(c) != toupper(c2))
				{
				Log("\n");

				LogMe();
				LogX();

				Log("Label %s\n", Label.c_str());
				Log("Lo  %10u%c\n", Lo, pom(Strand));
				Log("Lo2 %10u%c\n", Lo2, pom(Strand2));
				Log("i   %10u\n", i);
				Log("i2  %10u\n", i2);
				Log("Seq  ");
				for (unsigned k = Lo2; k < Lo; ++k)
					Log(".");
				Log("%s\n", Seq.c_str());

				Log("Seq2 %s\n", Seq2.c_str());

				Log("Diff ");
				for (unsigned k = Lo2; k < Lo+i; ++k)
					Log(" ");
				Log("^\n", Seq.c_str());

				Log("Offset %u,%u = %c,%c\n", i, i2, c, c2);

				Die("SeqDB::Validate");
				}
			}
		}
	}

unsigned SeqDB::OffsetToPos(unsigned SeqIndex, unsigned Offset) const
	{
	asserta(SeqIndex < SIZE(m_Los));
	asserta(Offset < m_Lengths[SeqIndex]);
	if (m_Strands[SeqIndex])
		return m_Los[SeqIndex] + Offset;
	else
		return m_Los[SeqIndex] + GetUngappedSeqLength(SeqIndex) - Offset - 1;
	}

unsigned SeqDB::PosToOffset(unsigned SeqIndex, unsigned Pos) const
	{
	asserta(SeqIndex < SIZE(m_Los));
	unsigned Lo = m_Los[SeqIndex];
	unsigned Hi = Lo + GetUngappedSeqLength(SeqIndex) - 1;
	asserta(Pos >= Lo && Pos <= Hi);
	if (m_Strands[SeqIndex])
		return Pos - Lo;
	else
		return Hi - Pos;
	}

bool SeqDB::PosInSeq(unsigned SeqIndex, unsigned Pos) const
	{
	asserta(SeqIndex < SIZE(m_Los));
	unsigned Lo = m_Los[SeqIndex];
	unsigned Hi = Lo + GetUngappedSeqLength(SeqIndex) - 1;
	return Pos >= Lo && Pos <= Hi;
	}

void SeqDB::GetUngappedSeq(unsigned SeqIndex, string &Seq) const
	{
	Seq.clear();
	const byte *s = GetSeq(SeqIndex);
	const unsigned L = GetSeqLength(SeqIndex);
	for (unsigned i = 0; i < L; ++i)
		{
		char c = s[i];
		if (!isgap(c))
			Seq.push_back(c);
		}
	}

void SeqDB::LogX(bool WithSeqs) const
	{
	Log("\n");
	Log("SeqDB(%s), %u seqs, aligned=%c\n",
	  m_Name.c_str(), GetSeqCount(), tof(m_Aligned));
	Log("Index  Length  UngLen      FullLn          Lo          Hi  +  Label\n");
	Log("-----  ------  ------  ----------  ----------  ----------  -  -----\n");
	for (unsigned i = 0; i < GetSeqCount(); ++i)
		{
		if (WithSeqs)
			{
			Log("%5u  %6u  %6u  %10u  %10u  %10u  %c  %-8.8s",
			  i,
			  GetSeqLength(i),
			  GetUngappedSeqLength(i),
			  GetFullLength(i),
			  GetLo(i),
			  GetHi(i),
			  pom(GetStrand(i)),
			  GetLabel(i).c_str());
			unsigned L = GetSeqLength(i);
			Log("  %*.*s\n", L, L, GetSeq(i));
			}
		else
			Log("%5u  %6u  %6u  %10u  %10u  %10u  %c  %s\n",
			  i,
			  GetSeqLength(i),
			  GetUngappedSeqLength(i),
			  GetFullLength(i),
			  GetLo(i),
			  GetHi(i),
			  pom(GetStrand(i)),
			  GetLabel(i).c_str());
		}
	}

bool SeqDB::IsAllGaps(unsigned SeqIndex) const
	{
	const byte *Seq = GetSeq(SeqIndex);
	const unsigned L = GetSeqLength(SeqIndex);
	for (unsigned i = 0; i < L; ++i)
		if (!isgap(Seq[i]))
			return false;
	return true;
	}

void SeqDB::StripAllGapSeqs()
	{
	m_AccMxf.Clear();
	m_IdMxf.Clear();
	m_GuideTree.Clear();
	m_SPPs.clear();
	const unsigned SeqCount = GetSeqCount();

	vector<bool> Del(SeqCount, false);
	vector<string> NewLabels;
	vector<byte *> NewSeqs;
	vector<unsigned> NewLengths;
	vector<float> NewWeights;
	vector<unsigned> NewUsers;
	vector<unsigned> NewFullLengths;
	vector<unsigned> NewLos;
	vector<bool> NewStrands;

	for (unsigned i = 0; i < SeqCount; ++i)
		if (!IsAllGaps(i))
			{
#define x(y)	New##y.push_back(m_##y[i]);
			x(Labels);
			x(Seqs);
			x(Lengths);
			x(Weights);
			x(Users);
			x(FullLengths);
			x(Los);
			x(Strands);
#undef x
			}
#define x(y)	m_##y = New##y
		x(Labels);
		x(Seqs);
		x(Lengths);
		x(Weights);
		x(Users);
		x(FullLengths);
		x(Los);
		x(Strands);
#undef x
	}

float SeqDB::GetColScore(unsigned ColIndex) const
	{
	float **SubstMx = GetSubstMx();
	const unsigned SeqCount = GetSeqCount();
	if (SeqCount == 2)
		{
		byte c1 = Get(0, ColIndex);
		byte c2 = Get(1, ColIndex);
		bool gap1 = isgap(c1);
		bool gap2 = isgap(c2);
		if (gap1 && gap2)
			return 0.0f;
		else if (gap1 || gap2)
			return -1.0f;
		return SubstMx[c1][c2];
		}

	float Score = 0;
	for (unsigned SeqIndex1 = 0; SeqIndex1 < SeqCount; ++SeqIndex1)
		{
		byte c1 = Get(SeqIndex1, ColIndex);
		if (isgap(c1))
			continue;
		for (unsigned SeqIndex2 = SeqIndex1 + 1; SeqIndex2 < SeqCount; ++SeqIndex2)
			{
			byte c2 = Get(SeqIndex2, ColIndex);
			if (isgap(c2))
				continue;
			Score += SubstMx[c1][c2];
			}
		}
	unsigned PairCount = (SeqCount*(SeqCount-1))/2;
	return Score/PairCount;
	}

void SeqDB::LogCol(unsigned ColIndex) const
	{
	for (unsigned i = 0; i < GetSeqCount(); ++i)
		Log("%c", Get(i, ColIndex));
	}

bool SeqDB::HasGap(unsigned ColIndex) const
	{
	for (unsigned i = 0; i < GetSeqCount(); ++i)
		if (isgap(Get(i, ColIndex)))
			return true;
	return false;
	}
