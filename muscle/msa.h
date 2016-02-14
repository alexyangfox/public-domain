#ifndef msa_h
#define msa_h

#include "isgap.h"
#include "seqdb.h"

struct Tree;

struct MSA
	{
	unsigned m_ColCount;
	vector<byte *> m_Seqs;
	vector<unsigned> m_Ids;
	vector<unsigned> m_IdToSeqIndex;
	vector<unsigned> m_StartPos;
	vector<const char *> m_Labels;
	vector<string> m_ShortLabels;
	SeqDB *m_SeqDB;
	string m_Name;
	float m_Accuracy;

	MSA()
		{
		m_ColCount = 0;
		m_SeqDB = 0;
		m_Accuracy = -1.0f;
		}

	~MSA()
		{
		Clear();
		}

	void FromAlignedDB(SeqDB &DB);
	void SetDB(SeqDB *DB = 0)
		{
		m_SeqDB = DB;
		}

	void Clear();

	unsigned GetSeqCount() const
		{
		return SIZE(m_Seqs);
		}

	unsigned GetColCount() const
		{
		return m_ColCount;
		}

	byte *GetSeqById(unsigned Id) const
		{
		unsigned SeqIndex = GetIndex(Id);
		return m_Seqs[SeqIndex];
		}

	byte *GetSeqByIndex(unsigned Index) const
		{
		asserta(Index < SIZE(m_Seqs));
		return m_Seqs[Index];
		}

	unsigned GetId(unsigned SeqIndex) const
		{
		asserta(SeqIndex < SIZE(m_Ids));
		return m_Ids[SeqIndex];
		}

	unsigned GetIndex(unsigned Id) const
		{
		asserta(Id < SIZE(m_IdToSeqIndex));
		return m_IdToSeqIndex[Id];
		}

	const char *GetLabelByIndex(unsigned Index) const
		{
		asserta(Index < SIZE(m_Labels));
		return m_Labels[Index];
		}
	const char *GetShortLabelByIndex(unsigned Index);

	char Get(unsigned SeqIndex, unsigned ColIndex) const
		{
		assert(SeqIndex < GetSeqCount());
		assert(ColIndex < m_ColCount);
		return m_Seqs[SeqIndex][ColIndex];
		}

	SeqDB *GetDB(bool FailIfNull = true) const
		{
		if (FailIfNull && m_SeqDB == 0)
			Die("MSA::GetDB, m_SeqDB=0");
		return m_SeqDB;
		}

	unsigned GetSeqLength(unsigned Index) const;
	unsigned GetMaxSeqLength() const;

	float GetPctIdPair(unsigned Index1, unsigned Index2) const;
	float GetAvgPctId() const;

	void AddSeq(const char *Label, byte *Seq, unsigned ColCount, unsigned Id);
	unsigned GetMaxLabelLength() const;
	unsigned GetMaxShortLabelLength();
	unsigned GetLetterCount(unsigned SeqIndex) const;
	void StripGapCols();
	unsigned GetMaxId() const;
	void Copy(const MSA &rhs);
	void CopySubset(const MSA &rhs, vector<unsigned> &Ids);
	void Validate() const;
	void ToFasta(const string &FileName);
	void SortIdOrder();
	void SortByIds(const vector<unsigned> &Ids);
	void SortByTree(Tree &t);
	void GetCol(unsigned ColIndex, string &Col) const;
	void SetColCase(const vector<bool> &Upper);
	void LogMe(const vector<float> *ptrColProbs = 0);
	};

#endif // msa_h
