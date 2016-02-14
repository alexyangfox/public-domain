#ifndef seqdb_h
#define seqdb_h

#include "mx.h"
#include "sparsemx.h"
#include "tree.h"
#include <vector>
using namespace std;
struct SparseMx;

enum SEQ_TYPE
	{
	ST_Unknown,
	ST_Amino,
	ST_DNA,
	ST_RNA
	};

struct SeqDB
	{
private:
	SeqDB(const SeqDB &rhs);
	SeqDB &operator=(const SeqDB &rhs);

public:
	string m_Name;
	vector<string> m_Labels;
	vector<byte *> m_Seqs;
	vector<unsigned> m_Lengths;
	vector<float> m_Weights;
	vector<unsigned> m_Users;
	vector<SparseMx *> m_SPPs;
	Mx<float> m_AccMxf;
	Mx<float> m_IdMxf;
	Tree m_GuideTree;

// Relationship of stored sequence i to an original sequence
// is defined by m_FullLengths[i], m_Los[i] and m_Strands[i].
// m_Strands[i] is false iff the sequence is revcomp'd vs
// the original full-length sequence.
	vector<unsigned> m_FullLengths;
	vector<unsigned> m_Los;
	vector<bool> m_Strands;

	byte *m_Buffer;
	bool m_Aligned;
	SEQ_TYPE m_SeqType;
	unsigned m_BufferPos;
	unsigned m_BufferSize;
	float m_Accuracy;

	SeqDB();
	~SeqDB();
	void Clear();
	void ClearSPPs();

	byte *GetSeq(unsigned SeqIndex) const
		{
		asserta(SeqIndex < SIZE(m_Seqs));
		return m_Seqs[SeqIndex];
		}

	const string &GetLabel(unsigned SeqIndex) const
		{
		asserta(SeqIndex < SIZE(m_Labels));
		return m_Labels[SeqIndex];
		}

	void SetLabel(unsigned SeqIndex, const string &Label)
		{
		asserta(SeqIndex < SIZE(m_Labels));
		m_Labels[SeqIndex] = Label;
		}

	unsigned GetSeqLength(unsigned SeqIndex) const
		{
		asserta(SeqIndex < SIZE(m_Lengths));
		return m_Lengths[SeqIndex];
		}

	unsigned GetSeqCount() const
		{
		return SIZE(m_Seqs);
		}

	unsigned GetPairCount() const
		{
		unsigned SeqCount = GetSeqCount();
		return (SeqCount*(SeqCount - 1))/2;
		}

	unsigned GetLo(unsigned SeqIndex) const
		{
		assert(SeqIndex < SIZE(m_Los));
		return m_Los[SeqIndex];
		}
	unsigned GetHi(unsigned SeqIndex) const;
	unsigned GetMAFPos(unsigned SeqIndex) const;

	bool GetStrand(unsigned SeqIndex) const
		{
		assert(SeqIndex < SIZE(m_Strands));
		return m_Strands[SeqIndex];
		}

	unsigned GetFullLength(unsigned SeqIndex) const
		{
		assert(SeqIndex < SIZE(m_FullLengths));
		return m_FullLengths[SeqIndex];
		}

	unsigned GetPairIndex(unsigned SeqIndex1, unsigned SeqIndex2) const
		{
		if (SeqIndex1 > SeqIndex2)
			return (SeqIndex1*(SeqIndex1 - 1))/2 + SeqIndex2;
		return (SeqIndex2*(SeqIndex2 - 1))/2 + SeqIndex1;
		}

	unsigned GetPairIndex(unsigned SeqIndex1, unsigned SeqIndex2,
	  bool &Transpose) const
		{
		if (SeqIndex1 > SeqIndex2)
			{
			Transpose = true;
			return (SeqIndex1*(SeqIndex1 - 1))/2 + SeqIndex2;
			}
		Transpose = false;
		return (SeqIndex2*(SeqIndex2 - 1))/2 + SeqIndex1;
		}

	unsigned GetColCount() const
		{
		if (!m_Aligned)
			Die("SeqDB::GetColCount, not aligned");
		if (m_Lengths.empty())
			Die("SeqDB::GetColCount, empty");
		return m_Lengths[0];
		}

	SEQ_TYPE GuessSeqType() const;
	SEQ_TYPE GetSeqType() const;
	unsigned GetSeqIndex(const string &Label) const;

	bool IsNucleo() const
		{
		SEQ_TYPE t = GetSeqType();
		return t == ST_DNA || t == ST_RNA;
		}

	void LogX(bool WithSeqs = false) const;
	void LogMe(const SeqDB *Input = 0) const;
	void ToMAF(FILE *f) const;

	void SetUser(unsigned SeqIndex, unsigned User);
	unsigned GetUser(unsigned SeqIndex) const
		{
		assert(SeqIndex < SIZE(m_Users));
		return m_Users[SeqIndex];
		}
	void AddSeq(const string &Label, byte *Seq, unsigned L,
	  float Weight = 1.0f, unsigned User = UINT_MAX, unsigned Lo = 0,
	  unsigned FullLength = UINT_MAX, bool Strand = true);

	void ReadSeqs(const string &FileName);
	unsigned GetMaxHi() const;
	unsigned GetMaxMAFPos() const;
	void GetShortLabel(unsigned SeqIndex, string &Label) const;

	void Reserve(unsigned Bytes);
	unsigned AppendSeq(const string &Label, const byte *Seq, unsigned L,
	  float Weight = 1.0f, unsigned User = UINT_MAX, unsigned Lo = 0,
	  unsigned FullLength = UINT_MAX, bool Strand = true);
	unsigned AppendSeq(const SeqDB &DB, unsigned SeqIndex);
	unsigned GetSeqBytes() const;
	void StripGapCols();
	void StripGaps(unsigned SeqIndex);
	void StripGaps();
	void Copy(const SeqDB &rhs);
	void CopySubset(const SeqDB &rhs, const vector<unsigned> &SeqIndexes);
	void ToFasta(const string &FileName) const;
	void ToFasta(FILE *f, unsigned SeqIndex) const;
	void SetColCase(const vector<bool> &Uppers);
	bool IsAllGaps(unsigned SeqIndex) const;
	void StripAllGapSeqs();
	unsigned GetUngappedSeqLength(unsigned SeqIndex) const;
	float GetSeqWeight(unsigned SeqIndex) const
		{
		asserta(SeqIndex < SIZE(m_Weights));
		return m_Weights[SeqIndex];
		};
	unsigned GetMaxShortLabelLength() const;
	unsigned GetMaxUngappedSeqLength() const;
	unsigned GetMaxFullLength() const;
	SparseMx &GetSPP(unsigned SeqIndex1, unsigned SeqIndex2, bool &Transpose);
	void GetPP(unsigned SeqIndex1, unsigned SeqIndex2, Mx<float> &PP);
	void GetPP(unsigned SeqIndex1, unsigned SeqIndex2, Mx<float> &PP, bool &Transpose);
	byte Get(unsigned SeqIndex, unsigned ColIndex) const;
	void GetCol(unsigned ColIndex, string &Col) const;
	Mx<float> &GetAccMxf() { if (m_AccMxf.Empty()) ComputeAccAndIdMxs(); return m_AccMxf; }
	Mx<float> &GetIdMxf() { if (m_IdMxf.Empty()) ComputeAccAndIdMxs(); return m_IdMxf; }
	float **GetAccMx() { ComputeAccAndIdMxs(); return m_AccMxf.GetData(); }
	float **GetIdMx() { ComputeAccAndIdMxs(); return m_IdMxf.GetData(); }
	void ComputeFastIdMx();
	void ComputeAccAndIdMxs();
	void ComputeSPPs(bool Local = false);
	void ComputeSelfSPPs(vector<SparseMx *> &SPPs);
	void Cons(unsigned Iter, unsigned Iters);
	void ComputePP(unsigned SeqIndex1, unsigned SeqIndex2, SparseMx &PP);
	void ComputeGuideTree();
	void ComputeSeqWeights(const Tree &tree);
	SeqDB &ProgressiveAlign(const string &SubFamFilenamePrefix = "");
	void Refine(SeqDB &msa, unsigned Iter, unsigned Iters);
	SeqDB &Align(unsigned ConsIters, unsigned RefineIters,
	  bool DoSequenceWeighting, const string &SubFamFilenamePrefix);
	float GetPctId(unsigned SeqIndex1, unsigned SeqIndex2) const;
	float GetAvgPctId() const;
	void GetPctIdMx(Mx<float> &Dist) const;
	void RevComp(unsigned SeqIndex);
	void RevComp();
	void ComputeAccsAndIds(vector<float> &AvgAccs, vector<float> &FractIds,
	  float &Acc, float &FractId);
	unsigned ColToPos(unsigned SeqIndex, unsigned Col) const;
	unsigned PosToCol(unsigned SeqIndex, unsigned Pos) const;
	void Sort(const vector<unsigned> &SortOrder);
	void SortByUser();
	void SortByTree(Tree &t);
	void SetPosFromLabels(const string &RegEx);
	void BindTree(Tree &t) const;
	void FromColRange(const SeqDB &DB, unsigned FromCol, unsigned ToCol);
	void FromSeq(const SeqDB &DB, unsigned Id);
	void GetSumLine(string &Line, unsigned FromCol = 0, unsigned ToCol = UINT_MAX) const;
	void Validate(const SeqDB &DB) const;
	unsigned OffsetToPos(unsigned SeqIndex, unsigned Offset) const;
	unsigned PosToOffset(unsigned SeqIndex, unsigned Pos) const;
	bool PosInSeq(unsigned SeqIndex, unsigned Pos) const;
	void GetUngappedSeq(unsigned SeqIndex, string &Seq) const; 
	float GetColScore(unsigned ColIndex) const;
	void LogCol(unsigned ColIndex) const;
	bool HasGap(unsigned ColIndex) const;
	};

#endif
