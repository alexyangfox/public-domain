#ifndef sparsemx_h
#define sparsemx_h

#include "mx.h"

struct SparseMx
	{
private:
	SparseMx(const SparseMx &rhs);
	SparseMx &operator=(const SparseMx &rhs);

public:
// Object data
	uint16 m_RowCount;
	uint16 m_ColCount;

// Matrix entry, aka a value, is a probability in the
// range 0 .. 1. Values greater than the threshold are
// stored as a (ColumnIndex, FractValue) pair. 
// Each row conceptually has a vector of these pairs.
	uint16 *m_ColIndexes;

// Values are stored as-is, i.e. floats
	float *m_Values;

// m_RowStartPos[i], i=0..(RowCount-1) is the offset into the
// m_Values and m_ColIndexes vector of the first value
// for row i.
	uint16 *m_RowStartPos;

// Columns are stored differently.
// They are computed on demand, so it is a priority that they
// be computed efficiently as well as retrieved efficiently.
	vector<vector<pair<uint16, float> > > m_Cols;

	SeqDB *m_SeqDB;
	unsigned m_IdA;
	unsigned m_IdB;

	void Validate();

// Interface uses unsigned rather than uint16 (a) to allow future
// extensions to larger arrays, and (b) to allow the class to check
// for overflows rather than requiring the calling code to do it.
	SparseMx();
	~SparseMx();
	void Clear();
	void FromMx(const float * const *Mx, unsigned RowCount, unsigned ColCount,
	  float MinValue, SeqDB *DB = 0, unsigned IdA = UINT_MAX, unsigned IdB = UINT_MAX);
	void FromMx(const Mx<float> &m);
	void ToMx(float **M);
	void ToFile(const string &FileName);
	void ToMxf(Mx<float> &M);
	void LogMe(const string &Name = "", bool Internals = false);
	void LogDotPlot(const string &Name = "");
	void LogSmallDotPlot(const string &Name = "", unsigned MaxL = 64);
	void LogInternals();
	void Copy(SparseMx &rhs);
	const char *GetRowLabel() const;
	const char *GetColLabel() const;
	const byte *GetRowSeq() const;
	const byte *GetColSeq() const;

// Return value is number of entries in *ptrValues and *ptrColIndexes.
	unsigned GetRow(unsigned RowIndex, float **ptrValues,
	  unsigned **ptrColIndexes);
	unsigned GetRow2(unsigned RowIndex, float **ptrValues,
	  unsigned **ptrColIndexes);

	void ComputeCols();
	void FreeCols();

// SLOW method just for testing
	float Get(unsigned RowIndex, unsigned ColIndex);

// Shared data
public:
// Pre-allocated vectors with size=max row.
// This to avoid thrashing and to simplify memory management.
	static unsigned m_MaxNonZeroValuesPerRow;
	static float *m_RowValueBuffer;
	static unsigned *m_ColIndexBuffer;
	static float *m_RowValueBuffer2;
	static unsigned *m_ColIndexBuffer2;
	static unsigned m_TotalFractValuesBytes;
	static unsigned m_TotalColIndexesBytes;
	static unsigned m_TotalStartPosBytes;
	static unsigned m_TotalCellCount;
	static unsigned m_TotalRowCount;
	};

void WriteSMx(const string &Name, SparseMx &SM);

#endif // sparsemx_h
