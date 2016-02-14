#include "myutils.h"
#include "seqdb.h"

float Viterbi(Mx<float> &PPMx, string &Path);

float GetFractId(const byte *Seq1, const byte *Seq2, const string &Path,
  unsigned Lo1, unsigned Lo2)
	{
	const unsigned L = SIZE(Path);
	unsigned i = Lo1;
	unsigned j = Lo2;
	unsigned PairCount = 0;
	unsigned IdCount = 0;
	for (unsigned k = 0; k < L; ++k)
		{
		char c = Path[k];
		if (c == 'M')
			{
			char a = Seq1[i];
			char b = Seq2[j];
			++PairCount;
			if (toupper(a) == toupper(b))
				++IdCount;
			}
		if (c == 'M' || c == 'D')
			++i;
		if (c == 'M' || c == 'I')
			++j;
		}
	return PairCount == 0 ? 0.0f : float(IdCount)/float(PairCount);
	}

void SeqDB::ComputeAccAndIdMxs()
	{
	if (!m_AccMxf.Empty())
		return;

	ComputeSPPs();

	const unsigned SeqCount = GetSeqCount();

	m_AccMxf.Alloc("AccMx", SeqCount, SeqCount);
	m_IdMxf.Alloc("IdMx", SeqCount, SeqCount);
	float **AccMx = m_AccMxf.GetData();
	float **IdMx = m_IdMxf.GetData();

	for (unsigned SeqIndex1 = 0; SeqIndex1 < SeqCount; ++SeqIndex1)
		{
		const byte *Seq1 = GetSeq(SeqIndex1);
		AccMx[SeqIndex1][SeqIndex1] = 1.0f;
		IdMx[SeqIndex1][SeqIndex1] = 1.0f;

		for (unsigned SeqIndex2 = SeqIndex1 + 1; SeqIndex2 < SeqCount; ++SeqIndex2)
			{
			const byte *Seq2 = GetSeq(SeqIndex2);

			Mx<float> PPMx;
			GetPP(SeqIndex1, SeqIndex2, PPMx);

			string Path;
			float Acc = Viterbi(PPMx, Path);

			float Id = GetFractId(Seq1, Seq2, Path, 0, 0);

			AccMx[SeqIndex1][SeqIndex2] = Acc;
			AccMx[SeqIndex2][SeqIndex1] = Acc;

			IdMx[SeqIndex1][SeqIndex2] = Id;
			IdMx[SeqIndex2][SeqIndex1] = Id;
			}
		}
	if (opt_trace)
		m_AccMxf.LogMe();
	}
