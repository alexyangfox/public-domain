#include "myutils.h"
#include "mx.h"
#include "seqdb.h"

char ProbToChar(float p);

list<MxBase *> *MxBase::m_Matrices = 0;

void MxBase::OnCtor(MxBase *Mx)
	{
	if (m_Matrices == 0)
		m_Matrices = new list<MxBase *>;
	asserta(m_Matrices != 0);
	m_Matrices->push_front(Mx);
	}

void MxBase::OnDtor(MxBase *Mx)
	{
	if (m_Matrices == 0)
		{
		Warning("MxBase::OnDtor, m_Matrices = 0");
		return;
		}
	for (list<MxBase*>::iterator p = m_Matrices->begin();
	  p != m_Matrices->end(); ++p)
		{
		if (*p == Mx)
			{
			m_Matrices->erase(p);
			if (m_Matrices->empty())
				delete m_Matrices;
			return;
			}
		}
	Warning("MxBase::OnDtor, not found");
	}

MxBase *MxBase::Get(const string &Name)
	{
	if (m_Matrices == 0)
		Die("MxBase::Get, m_Matrices=0");
	for (list<MxBase*>::iterator p = m_Matrices->begin();
	  p != m_Matrices->end(); ++p)
		{
		MxBase *m = *p;
		if (m != 0 && m->m_Name == Name)
			return m;
		}
	Die("MxBase::Get(%s), not found", Name.c_str());
	ureturn(0);
	}

float **MxBase::Getf(const string &Name)
	{
	Mx<float> *m = (Mx<float> *) Get(Name);
	asserta(m->GetTypeSize() == sizeof(float));
	return m->GetData();
	}

char **MxBase::Getc(const string &Name)
	{
	Mx<char> *m = (Mx<char> *) Get(Name);
	asserta(m->GetTypeSize() == sizeof(char));
	return m->GetData();
	}

void MxBase::Alloc(const string &Name, unsigned RowCount, unsigned ColCount,
  SeqDB *DB, unsigned IdA, unsigned IdB)
	{
	if (DB != 0)
		{
		asserta(IdA != UINT_MAX);
		asserta(IdB != UINT_MAX);
		asserta(RowCount == DB->GetSeqLength(IdA) + 1);
		asserta(ColCount == DB->GetSeqLength(IdB) + 1);
		}
	if (RowCount > m_AllocatedRowCount || ColCount > m_AllocatedColCount)
		{
		FreeData();
		AllocData(RowCount, ColCount);
		}
	
	m_Name = Name;
	m_RowCount = RowCount;
	m_ColCount = ColCount;
	m_SeqDB = DB;
	m_IdA = IdA;
	m_IdB = IdB;
	}

void MxBase::LogMe(bool WithData) const
	{
	Log("\n");
	Log("%s(%p) Rows %u/%u, Cols %u/%u",
	  m_Name.c_str(), this,
	  m_RowCount, m_AllocatedRowCount,
	  m_ColCount, m_AllocatedColCount);
	if (m_SeqDB != 0 && m_IdA != UINT_MAX)
		Log(", A=%s", m_SeqDB->GetLabel(m_IdA).c_str());
	if (m_SeqDB != 0 && m_IdB != UINT_MAX)
		Log(", B=%s", m_SeqDB->GetLabel(m_IdB).c_str());
	Log("\n");
	if (!WithData || m_RowCount == 0 || m_ColCount == 0)
		return;

	const char *z = GetAsStr(0, 0);
	unsigned Width = strlen(z);
	unsigned Mod = 1;
	for (unsigned i = 0; i < Width; ++i)
		Mod *= 10;

	if (!m_Alpha.empty())
		{
		Log("// Alphabet=%s\n", m_Alpha.c_str());
		Log("//      ");
		for (unsigned j = 0; j < SIZE(m_Alpha); ++j)
			Log(" %*c", Width, m_Alpha[j]);
		Log("\n");
		for (unsigned i = 0; i < SIZE(m_Alpha); ++i)
			{
			Log("/* %c */ {", m_Alpha[i]);
			unsigned ci = m_Alpha[i];
			for (unsigned j = 0; j < SIZE(m_Alpha); ++j)
				{
				unsigned cj = m_Alpha[j];
				Log("%s,", GetAsStr(ci, cj));
				}
			Log("},  // %c\n", m_Alpha[i]);
			}
		return;
		}

	const byte *A = 0;
	const byte *B = 0;
	if (m_SeqDB != 0 && m_IdA != UINT_MAX)
		A = m_SeqDB->GetSeq(m_IdA);
	if (m_SeqDB != 0 && m_IdB != UINT_MAX)
		B = m_SeqDB->GetSeq(m_IdB);

	if (B != 0)
		{
		if (A != 0)
			Log("  ");
		Log("%5.5s", "");
		for (unsigned j = 0; j < m_ColCount; ++j)
			Log("%*c", Width, j == 0 ? ' ' : B[j-1]);
		Log("\n");
		}

	if (A != 0)
		Log("  ");
	Log("%5.5s", "");
	for (unsigned j = 0; j < m_ColCount; ++j)
		Log("%*u", Width, j%Mod);
	Log("\n");

	for (unsigned i = 0; i < m_RowCount; ++i)
		{
		if (A != 0)
			Log("%c ", i == 0 ? ' ' : A[i-1]);
		Log("%4u ", i);
		
		for (unsigned j = 0; j < m_ColCount; ++j)
			Log("%s", GetAsStr(i, j));
		Log("\n");
		}
	}
static unsigned g_MatrixFileCount;

void WriteSMx(const string &Name, SparseMx &SM)
	{
	Log("\n");
	Log("Posterior:\n");
	Log("Type=%s\n", Name.c_str());
	unsigned LA = SM.m_RowCount - 1;
	unsigned LB = SM.m_ColCount - 1;
	Log("nrows=%u\n", LA);
	Log("ncols=%u\n", LB);
	Log("rows=%s\n", SM.GetRowLabel());
	Log("cols=%s\n", SM.GetColLabel());
	Log("probs=\n");
	for (unsigned i = 0; i < LA; ++i)
		{
		float *Values;
		unsigned *ColIndexes;
		unsigned EntryCount = SM.GetRow(i+1, &Values, &ColIndexes);
		if (EntryCount == 0)
			continue;
		Log("%u:", i+1);
		for (unsigned k = 0; k < EntryCount; ++k)
			Log(" %u=%.5f", ColIndexes[k], Values[k]);
		Log("\n");
		}
	Log("//\n");
	Log("\n");
	}

void WriteMx(const string &Name, Mx<float> &Mxf)
	{
	Log("\n");
	Log("Posterior:\n");
	Log("Type=%s\n", Name.c_str());
	Log("Subtype=%s\n", Mxf.m_Name.c_str());
	unsigned LA = Mxf.m_RowCount - 1;
	unsigned LB = Mxf.m_ColCount - 1;
	Log("nrows=%u\n", LA);
	Log("ncols=%u\n", LB);

	if (Mxf.m_SeqDB != 0)
		{
		const SeqDB &DB = *Mxf.m_SeqDB;
		unsigned IdA = Mxf.m_IdA;
		unsigned IdB = Mxf.m_IdB;
		Log("rows=%s\n", DB.GetLabel(IdA).c_str());
		Log("cols=%s\n", DB.GetLabel(IdB).c_str());
		}

	Log("probs=\n");
	for (unsigned i = 0; i < LA; ++i)
		{
		bool Any = false;
		for (unsigned j = 0; j < LB; ++j)
			{
			float p = Mxf.Get(i+1, j+1);
			if (p >= opt_minsparseprob)
				{
				if (!Any)
					{
					Any = true;
					Log("%u:", i+1);
					}
				Log(" %u=%.5f", j+1, p);
				}
			}
		if (Any)
			Log("\n");
		}
	Log("//\n");
	Log("\n");
	}

void LogSmallDotPlot(const Mx<float> &Mx, const string &Name, unsigned MaxL)
	{
	Log("\n");
	if (Name != "")
		Log("%s:\n", Name.c_str());

	const unsigned RowCount = Mx.m_RowCount;
	const unsigned ColCount = Mx.m_ColCount;
	unsigned RowN = RowCount;
	unsigned ColN = ColCount;
	unsigned RowInc = 1;
	unsigned ColInc = 1;

	if (RowN > MaxL)
		{
		RowInc = RowN/MaxL + 1;
		RowN = RowCount/RowInc;
		}
	if (ColN > MaxL)
		{
		ColInc = ColN/MaxL + 1;
		ColN = ColCount/ColInc;
		}

	for (unsigned i = 1; i < RowCount; i += RowInc)
		{
		for (unsigned j = 1; j < ColCount; j += ColInc)
			{
			float maxp = 0;
			for (unsigned ki = 0; ki < RowInc; ++ki)
				{
				for (unsigned kj = 0; kj < ColInc; ++kj)
					{
					unsigned ii = i - 1 + ki;
					if (ii >= RowCount)
						continue;
					unsigned jj = j - 1 +kj;
					if (jj >= ColCount)
						continue;
					float p = Mx.Get(ii, jj);
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
