#include "myutils.h"
#include "mxmgr.h"

void MxMgr::Clear()
	{
	const unsigned MxCount = GetCount();
	for (unsigned MxIndex = 0; MxIndex < MxCount; ++MxIndex)
		Clear(MxIndex);
	}

void MxMgr::Clear(unsigned MxIndex)
	{
	asserta(MxIndex < SIZE(m_MxPtrs));
	unsigned RowCount = m_RowCounts[MxIndex];
	float **Mx = m_MxPtrs[MxIndex];
	for (unsigned i = 0; i < RowCount; ++i)
		free(Mx[i]);
	m_MxPtrs[MxIndex] = 0;
	}

float **MxMgr::Get(unsigned MxIndex, unsigned RowCount, unsigned ColCount)
	{
	if (MxIndex >= SIZE(m_MxPtrs))
		{
		m_MxPtrs.resize(MxIndex+1, 0);
		m_RowCounts.resize(MxIndex+1, 0);
		m_ColCounts.resize(MxIndex+1, 0);
		}

	if (m_RowCounts[MxIndex] <= RowCount && m_ColCounts[MxIndex] <= ColCount)
		return m_MxPtrs[MxIndex];

	Clear(MxIndex);

	float **Mx = myalloc<float *>(RowCount);
	for (unsigned i = 0; i < RowCount; ++i)
		Mx[i] = myalloc<float>(ColCount);

	m_MxPtrs[MxIndex] = Mx;
	return Mx;
	}
