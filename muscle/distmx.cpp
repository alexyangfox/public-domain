#include "myutils.h"
#include "seqdb.h"

float PPViterbi(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &PPMx, string &Path);

void SeqDB::ComputeDistMx()
	{
	if (!m_DistMx.Empty())
		return;

	ComputeSPPs();

	const unsigned SeqCount = GetSeqCount();

	m_DistMx.Alloc("DistMx", SeqCount, SeqCount);
	float **Dist = m_DistMx.GetData();

	for (unsigned SeqIndex1 = 0; SeqIndex1 < SeqCount; ++SeqIndex1)
		{
		Dist[SeqIndex1][SeqIndex1] = 0.0f;
		for (unsigned SeqIndex2 = SeqIndex1 + 1; SeqIndex2 < SeqCount; ++SeqIndex2)
			{
			Mx<float> PPMx;
			GetPP(SeqIndex1, SeqIndex2, PPMx);

			string Path;
			float ExpectedAccuracy =
			  PPViterbi(*this, SeqIndex1, SeqIndex2, PPMx, Path);
			Dist[SeqIndex1][SeqIndex2] = ExpectedAccuracy;
			Dist[SeqIndex2][SeqIndex1] = ExpectedAccuracy;
			}
		}
	if (opt_trace)
		m_DistMx.LogMe();
	}
