#include "myutils.h"
#include "sparsemx.h"
#include "mx.h"
#include "seqdb.h"

#define TRACE		0

unsigned SparseMx::m_MaxNonZeroValuesPerRow;
float *SparseMx::m_RowValueBuffer;
unsigned *SparseMx::m_ColIndexBuffer;
float *SparseMx::m_RowValueBuffer2;
unsigned *SparseMx::m_ColIndexBuffer2;

unsigned SparseMx::m_TotalFractValuesBytes;
unsigned SparseMx::m_TotalColIndexesBytes;
unsigned SparseMx::m_TotalStartPosBytes;
unsigned SparseMx::m_TotalCellCount;
unsigned SparseMx::m_TotalRowCount;

static unsigned g_Buffer1Size;
static unsigned g_Buffer2Size;
static uint16 *g_Buffer2;

static float *g_Buffer1;
static float *GetBuffer1(unsigned Size)
	{
	if (Size > g_Buffer1Size)
		{
		if (g_Buffer1Size > 0)
			free(g_Buffer1);
		g_Buffer1 = myalloc<float>(Size);
		g_Buffer1Size = Size;
		}
	return g_Buffer1;
	}

// For col indexes; uint16 enough for seq lengths up to 2^16-1.
static uint16 *GetBuffer2(unsigned Size)
	{
	if (Size > g_Buffer2Size)
		{
		if (g_Buffer2Size > 0)
			free(g_Buffer2);
		g_Buffer2 = myalloc<uint16>(Size);
		g_Buffer2Size = Size;
		}
	return g_Buffer2;
	}

SparseMx::SparseMx()
	{
	m_Values = 0;
	m_ColIndexes = 0;
	m_RowStartPos = 0;
	m_RowCount = 0;
	m_ColCount = 0;
	}

SparseMx::~SparseMx()
	{
	//@@Clear();
	}

void SparseMx::Clear()
	{
	myfree(m_Values);
	myfree(m_RowStartPos);
	myfree(m_ColIndexes);

	m_RowCount = 0;
	m_ColCount = 0;
	m_Values = 0;
	m_ColIndexes = 0;
	m_RowStartPos = 0;

	m_SeqDB = 0;
	m_IdA = UINT_MAX;
	m_IdB = UINT_MAX;
	}

// @@ Can do this much more efficiently, this is
// @@ to be robust against internals
void SparseMx::Copy(SparseMx &rhs)
	{
	Mx<float> m;
	m.Alloc("tmp", rhs.m_RowCount, rhs.m_ColCount);
	rhs.ToMx(m.GetData());
	FromMx(m.GetData(), rhs.m_RowCount, rhs.m_ColCount, opt_minsparseprob,
	  rhs.m_SeqDB, rhs.m_IdA, rhs.m_IdB);
	}

void SparseMx::ToMxf(Mx<float> &Mxf)
	{
	Mxf.Alloc("FromSparse", m_RowCount, m_ColCount, m_SeqDB, m_IdA, m_IdB);
	float **Data = Mxf.GetData();
	ToMx(Data);
	}

void SparseMx::ToMx(float **Mx)
	{
	for (unsigned i = 0; i < m_RowCount; ++i)
		{
		float *Row = Mx[i];
		for (unsigned j = 0; j < m_ColCount; ++j)
			Row[j] = 0;
		float *Values;
		unsigned *ColIndexes;
		unsigned EntryCount = GetRow(i, &Values, &ColIndexes);
		for (unsigned k = 0; k < EntryCount; ++k)
			Row[ColIndexes[k]] = Values[k];
		}
	}

void SparseMx::FromMx(const Mx<float> &m)
	{
	FromMx(m.GetData(), m.m_RowCount, m.m_ColCount, opt_minsparseprob,
	  m.m_SeqDB, m.m_IdA, m.m_IdB);
	}

void SparseMx::FromMx(const float * const*Mx, unsigned RowCount, unsigned ColCount,
  float MinValue, SeqDB *DB, unsigned IdA, unsigned IdB)
	{
	Clear();

	if (RowCount > USHRT_MAX || ColCount > USHRT_MAX)
		Die("SparseMx::FromMx(%u,%u), size overflow", RowCount, ColCount);

	m_RowCount = uint16(RowCount);
	m_ColCount = uint16(ColCount);

	m_SeqDB = DB;
	m_IdA = IdA;
	m_IdB = IdB;

	unsigned MaxEntryCount = m_RowCount*m_ColCount;

// Allocate max possible, resize memory block later.
// First entry in m_Values is zero for special case
// where no non-zero entries in row.
	m_Values = GetBuffer1(MaxEntryCount+RowCount+1);

// Max possible is where base column index changes every
// time, which is 3 bytes per entry.
	m_ColIndexes = GetBuffer2(MaxEntryCount);

// Per-row vector
	m_RowStartPos = myalloc<uint16>(RowCount);

	m_Values[0] = 0;
	unsigned Pos = 1;
	unsigned MaxNonZeroValuesPerRow = 0;
#if	TRACE
	Log("\n");
	Log("  Row    Col    Pos     Value\n");
	Log("-----  -----  -----  --------\n");
#endif // TRACE
	for (unsigned RowIndex = 0; RowIndex < RowCount; ++RowIndex)
		{
		unsigned NonZeroValuesThisRow = 0;
		unsigned StartPosRow = 0;
		for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
			{
			float Value = Mx[RowIndex][ColIndex];
			if (Value <= MinValue)
				continue;

			++NonZeroValuesThisRow;
			if (StartPosRow == 0)
				StartPosRow = Pos;

			m_Values[Pos] = Value;
			m_ColIndexes[Pos] = uint16(ColIndex);
#if	TRACE
			Log("%5u  %5u  %5u  %8.4f\n",
			  RowIndex,
			  ColIndex,
			  Pos,
			  Value);
#endif // TRACE
			++Pos;
			}

		if (NonZeroValuesThisRow > 0)
			m_TotalCellCount += NonZeroValuesThisRow;

		m_Values[Pos++] = 0;

		if (NonZeroValuesThisRow > MaxNonZeroValuesPerRow)
			MaxNonZeroValuesPerRow = NonZeroValuesThisRow;

		if (Pos > USHRT_MAX)
			Die("SparseMx::FromMx(), cell overflow");

		m_RowStartPos[RowIndex] = uint16(StartPosRow);
		}

	if (MaxNonZeroValuesPerRow > m_MaxNonZeroValuesPerRow)
		{
		if (m_MaxNonZeroValuesPerRow > 0)
			{
			free(m_RowValueBuffer);
			free(m_ColIndexBuffer);
			free(m_RowValueBuffer2);
			free(m_ColIndexBuffer2);
			}
		m_MaxNonZeroValuesPerRow = MaxNonZeroValuesPerRow;
		m_RowValueBuffer = myalloc<float>(MaxNonZeroValuesPerRow);
		m_ColIndexBuffer = myalloc<unsigned>(MaxNonZeroValuesPerRow);
		m_RowValueBuffer2 = myalloc<float>(MaxNonZeroValuesPerRow);
		m_ColIndexBuffer2 = myalloc<unsigned>(MaxNonZeroValuesPerRow);
		}

	m_TotalFractValuesBytes += Pos*sizeof(float);
	float *Values = myalloc<float>(Pos);
	memcpy(Values, m_Values, sizeof(float)*Pos);

	uint16 *ColIndexes = myalloc<uint16>(Pos);

	memcpy(ColIndexes, m_ColIndexes, Pos*sizeof(uint16));

	m_Values = Values;
	m_ColIndexes = ColIndexes;

	m_TotalRowCount += RowCount;
	m_TotalColIndexesBytes += Pos*sizeof(m_ColIndexes[0]);
	m_TotalStartPosBytes  += RowCount*sizeof(m_RowStartPos[0]);

#if TRACE
	LogMe("", true);
#endif
#if	DEBUG
	Validate();
#endif
	}

void SparseMx::Validate()
	{
	for (unsigned RowIndex = 0; RowIndex < m_RowCount; ++RowIndex)
		{
		float *Values;
		unsigned *ColIndexes;
		unsigned EntryCount = GetRow(RowIndex, &Values, &ColIndexes);
		for (unsigned EntryIndex = 0; EntryIndex < EntryCount; ++EntryIndex)
			{
			unsigned ColIndex = ColIndexes[EntryIndex];
			asserta(ColIndex < m_ColCount);
			float Value = Values[EntryIndex];
			if (Value < float(0) && Value > float(1.1))
				Die("SparseMx: Row=%u Entry=%u Value=%g", RowIndex, EntryIndex, Value);
			}
		}
	
	ComputeCols();
	for (unsigned ColIndex = 0; ColIndex < m_ColCount; ++ColIndex)
		{
		const vector<pair<uint16, float> > &Col = m_Cols[ColIndex];
		const unsigned EntryCount = SIZE(Col);
		for (unsigned EntryIndex = 0; EntryIndex < EntryCount; ++EntryIndex)
			{
			const pair<uint16, float> &e2 = Col[EntryIndex];
			unsigned RowIndex = e2.first;
			asserta(RowIndex < m_RowCount);
			float Value = e2.second;
			if (Value < float(0) && Value > float(1.1))
				Die("SparseMx: Col=%u Entry=%u Value=%g", ColIndex, EntryIndex, Value);
			}
		}
	FreeCols();
	}

unsigned SparseMx::GetRow(unsigned RowIndex, float **ptrValues, unsigned **ptrColIndexes)
	{
	asserta(RowIndex < m_RowCount);

	float *Values = m_RowValueBuffer;
	unsigned *ColIndexes = m_ColIndexBuffer;

	*ptrValues = Values;
	*ptrColIndexes = ColIndexes;

	unsigned Pos = m_RowStartPos[RowIndex];
	unsigned EntryCount = 0;
	for (;;)
		{
		float Value = m_Values[Pos];
		if (Value == 0)
			return EntryCount;
		Values[EntryCount] = Value;
		ColIndexes[EntryCount] = m_ColIndexes[Pos];
		++Pos;
		++EntryCount;
		}
	}

unsigned SparseMx::GetRow2(unsigned RowIndex, float **ptrValues, unsigned **ptrColIndexes)
	{
	asserta(RowIndex < m_RowCount);

	float *Values = m_RowValueBuffer2;
	unsigned *ColIndexes = m_ColIndexBuffer2;

	*ptrValues = Values;
	*ptrColIndexes = ColIndexes;

	unsigned Pos = m_RowStartPos[RowIndex];
	unsigned EntryCount = 0;
	for (;;)
		{
		float Value = m_Values[Pos];
		if (Value == 0)
			return EntryCount;
		Values[EntryCount] = Value;
		ColIndexes[EntryCount] = m_ColIndexes[Pos];
		++Pos;
		++EntryCount;
		}
	}

float SparseMx::Get(unsigned RowIndex, unsigned ColIndex)
	{
	float *Values;
	unsigned *ColIndexes;
	unsigned EntryCount = GetRow(RowIndex, &Values, &ColIndexes);
	for (unsigned EntryIndex = 0; EntryIndex < EntryCount; ++EntryIndex)
		{
		if (ColIndexes[EntryIndex] == ColIndex)
			return Values[EntryIndex];
		}
	return float(0);
	}

void SparseMx::ComputeCols()
	{
	m_Cols.clear();
	m_Cols.resize(m_ColCount);
	for (unsigned RowIndex = 0; RowIndex < m_RowCount; ++RowIndex)
		{
		float *Values;
		unsigned *ColIndexes;
		unsigned EntryCount = GetRow(RowIndex, &Values, &ColIndexes);
		for (unsigned EntryIndex = 0; EntryIndex < EntryCount; ++EntryIndex)
			{
			unsigned ColIndex = ColIndexes[EntryIndex];
			assert(ColIndex < m_ColCount);
			vector<pair<uint16, float> > &Col = m_Cols[ColIndex];
			float Value = Values[EntryIndex];
			Col.push_back(pair<uint16, float>(uint16(RowIndex), Value));
			}
		}
	}

void SparseMx::FreeCols()
	{
	m_Cols.clear();
	}

void SparseMx::LogMe(const string &Name, bool Internals)
	{
	Log("\n");
	if (Name != "")
		Log("%s:\n", Name.c_str());

	if (Internals)
		LogInternals();

	if (m_SeqDB != 0)
		{
		Log("Rows=%s\n", GetRowLabel());
		Log("Cols=%s\n", GetColLabel());
		const byte *RowSeq = GetRowSeq();
		const byte *ColSeq = GetColSeq();
		if (RowSeq != 0)
			Log("  ");
		Log("     ");
		for (unsigned ColIndex = 0; ColIndex < m_ColCount; ++ColIndex)
			Log("%8.8c", ColIndex == 0 ? ' ' : ColSeq[ColIndex-1]);
		Log("\n");
		}

	if (m_SeqDB != 0 && GetRowSeq() != 0)
		Log("  ");
	Log("     ");
	for (unsigned ColIndex = 0; ColIndex < m_ColCount; ++ColIndex)
		Log("%8u", ColIndex);
	Log("\n");

	for (unsigned RowIndex = 0; RowIndex < m_RowCount; ++RowIndex)
		{
		const byte *RowSeq = GetRowSeq();
		if (RowSeq != 0)
			Log("%c ", RowIndex == 0 ? ' ' : RowSeq[RowIndex-1]);
		Log("%5u", RowIndex);
		for (unsigned ColIndex = 0; ColIndex < m_ColCount; ++ColIndex)
			{
			float Value = Get(RowIndex, ColIndex);
			if (Value == 0.0f)
				Log("%8.8s", ".");
			else
				Log("%8.4f", Value);
			}
		Log("\n");
		}
	}

void SparseMx::LogInternals()
	{
	Log("RowCount=%u ColCount=%u\n", m_RowCount, m_ColCount);
	Log("  Row  StartPos\n");
	Log("-----  --------\n");
	for (unsigned RowIndex = 0; RowIndex < m_RowCount; ++RowIndex)
		Log("%5u  %8u\n",
		  RowIndex, m_RowStartPos[RowIndex]);

	Log("\n");
	for (unsigned RowIndex = 0; RowIndex < m_RowCount; ++RowIndex)
		{
		unsigned Pos = m_RowStartPos[RowIndex];
		Log("Row %5u: ", RowIndex);
		for (;;)
			{
			float Value = m_Values[Pos];
			Log(" FVs[%u]=%.4f", Pos, Value);
			if (Value == 0)
				break;

			unsigned ColIndex = m_ColIndexes[Pos];
			Log(" CI=%u", ColIndex);
			++Pos;
			}
		Log("\n");
		}
	}

void LogSparseMxStats()
	{
	unsigned Total = SparseMx::m_TotalFractValuesBytes + SparseMx::m_TotalStartPosBytes + SparseMx::m_TotalColIndexesBytes;

	Log("%8.1f M   Cells\n", SparseMx::m_TotalCellCount/1.0e6);
	Log("%8.1f     Avg cells per row\n", double(SparseMx::m_TotalCellCount)/double(SparseMx::m_TotalRowCount));
	Log(  "%8u     Max per row\n", SparseMx::m_MaxNonZeroValuesPerRow);
	Log("%8.1f Mb  Fract values\n", SparseMx::m_TotalFractValuesBytes/1.0e6);
	Log("%8.1f Mb  Start pos \n", SparseMx::m_TotalStartPosBytes/1.0e6);
	Log("%8.1f Mb  Col indexes \n", SparseMx::m_TotalColIndexesBytes/1.0e6);
	Log("%8.1f Mb  Total\n", Total/1.0e6);
	}

#if	0
void TestSparseMx()
	{
	unsigned LA = 8;
	unsigned LB = 5;
	float **Mx = new float*[LA];
	for (unsigned i = 0; i < LA; ++i)
		{
		Mx[i] = new float[LB];
		for (unsigned j = 0; j < LB; ++j)
			Mx[i][j] = 0;
		}

	Mx[0][0] = 0.1f;
	Mx[1][1] = 0.2f;
	Mx[1][2] = 0.22f;
	Mx[2][2] = 0.3f;
	Mx[3][4] = 0.4f;

	SparseMx M;
	M.FromMx(Mx, LA, LB, 0.01f, "", "");
	M.LogInternals();
	}
#endif //0

char ProbToChar(float p)
	{
	if (p > 1.1 || p < -0.1)
		return '!';
	if (p > 0.75)
		return '*';
	if (p > 0.5)
		return '+';
	if (p > 0.25)
		return '~';
	if (p >= 0.01)
		return '.';
	return ' ';
	}

void SparseMx::LogDotPlot(const string &Name)
	{
	Log("\n");
	if (Name != "")
		Log("%s:\n", Name.c_str());

	Log("  ");
	const byte *ColSeq = GetColSeq();
	const byte *RowSeq = GetRowSeq();
	for (unsigned j = 1; j < m_ColCount; ++j)
		Log("%c", ColSeq[j-1]);
	Log("\n");
	for (unsigned i = 1; i < m_RowCount; ++i)
		{
		Log("%c ", RowSeq[i-1]);
		for (unsigned j = 1; j < m_ColCount; ++j)
			{
			float p = Get(i-1, j-1);
			char c = ProbToChar(p);
			Log("%c", c);
			}
		Log("\n");
		}
	}

void SparseMx::LogSmallDotPlot(const string &Name, unsigned MaxL)
	{
	Log("\n");
	if (Name != "")
		Log("%s:\n", Name.c_str());

	unsigned RowN = m_RowCount;
	unsigned ColN = m_ColCount;
	unsigned RowInc = 1;
	unsigned ColInc = 1;

	if (RowN > MaxL)
		{
		RowInc = RowN/MaxL + 1;
		RowN = m_RowCount/RowInc;
		}
	if (ColN > MaxL)
		{
		ColInc = ColN/MaxL + 1;
		ColN = m_ColCount/ColInc;
		}

	const byte *ColSeq = GetColSeq();
	if (ColSeq != 0)
	for (unsigned k = 0; k < ColInc; ++k)
		{
		for (unsigned ki = 0; ki < RowInc; ++ki)
			Log(" ");
		Log("  ");
		for (unsigned j = 1; j < m_ColCount; j += ColInc)
			{
			unsigned jj = j - 1 + k;
			if (jj + 1 < m_ColCount)
				Log("%c", ColSeq[jj]);
			else
				Log(" ");
			}
		Log("\n");
		}

	for (unsigned k = 0; k < RowInc; ++k)
		Log(" ");
	Log("  ");
	for (unsigned j = 1; j < m_ColCount; j += ColInc)
		Log("_");
	Log("\n");

	const byte *RowSeq = GetRowSeq();
	for (unsigned i = 1; i < m_RowCount; i += RowInc)
		{
		if (RowSeq != 0)
		for (unsigned k = 0; k < RowInc; ++k)
			{
			unsigned ii = i-1+k;
			if (ii + 1 < m_RowCount)
				Log("%c", RowSeq[ii]);
			else
				Log(" ");
			}
		Log(" |");

		for (unsigned j = 1; j < m_ColCount; j += ColInc)
			{
			float maxp = 0;
			for (unsigned ki = 0; ki < RowInc; ++ki)
				{
				for (unsigned kj = 0; kj < ColInc; ++kj)
					{
					unsigned ii = i - 1 + ki;
					if (ii >= m_RowCount)
						continue;
					unsigned jj = j - 1 +kj;
					if (jj >= m_ColCount)
						continue;
					float p = Get(ii, jj);
					if (p > maxp)
						maxp = p;
					}
				}
			char c = ProbToChar(maxp);
			Log("%c", c);
			}
		Log("\n");
		}
	}

const char *SparseMx::GetRowLabel() const
	{
	if (m_SeqDB == 0)
		return "";
	return m_SeqDB->GetLabel(m_IdA).c_str();
	}

const char *SparseMx::GetColLabel() const
	{
	if (m_SeqDB == 0)
		return "";
	return m_SeqDB->GetLabel(m_IdB).c_str();
	}

const byte *SparseMx::GetRowSeq() const
	{
	if (m_SeqDB == 0)
		return 0;
	return m_SeqDB->GetSeq(m_IdA);
	}

const byte *SparseMx::GetColSeq() const
	{
	if (m_SeqDB == 0)
		return 0;
	return m_SeqDB->GetSeq(m_IdB);
	}
