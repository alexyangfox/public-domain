#include "myutils.h"
#include "seqdb.h"

void FwdBwd(SeqDB &DB, unsigned IdA, unsigned IdB, SparseMx &SPP);
void FwdBwdLocal(SeqDB &DB, unsigned IdA, unsigned IdB, SparseMx &SPP);

void SeqDB::ComputeSPPs(bool Local)
	{
	if (!m_SPPs.empty())
		return;

	const unsigned SeqCount = GetSeqCount();
	const unsigned PairCount = GetPairCount();
	m_SPPs.resize(PairCount);
	for (unsigned i = 0; i < PairCount; ++i)
		{
		m_SPPs[i] = new SparseMx;
		if (m_SPPs[i] == 0)
			Die("Out of memory");
		}

	unsigned Counter = 0;
	for (unsigned SeqIndex1 = 0; SeqIndex1 < SeqCount; ++SeqIndex1)
		{
		const string &Label1 = GetLabel(SeqIndex1);

		for (unsigned SeqIndex2 = SeqIndex1 + 1; SeqIndex2 < SeqCount; ++SeqIndex2)
			{
			const string &Label2 = GetLabel(SeqIndex2);

			unsigned PairIndex = GetPairIndex(SeqIndex1, SeqIndex2);
			ProgressStep(Counter++, PairCount, "Align pairs %.16s,%.16s",
			  Label1.c_str(), Label2.c_str());

			SparseMx &SPP = *m_SPPs[PairIndex];
			if (Local)
				FwdBwdLocal(*this, SeqIndex1, SeqIndex2, SPP);
			else
				FwdBwd(*this, SeqIndex1, SeqIndex2, SPP);

			if (opt_trace)
				SPP.LogMe();
			}
		}
	}

void ComputeSelfSPPs(vector<SparseMx *> &SPPs)
	{
	asserta(SPPs.empty());
	}
