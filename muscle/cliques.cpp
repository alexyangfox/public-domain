#include "myutils.h"
#include "seqdb.h"
#include "mx.h"
#include "params.h"
#include "sparsemx.h"
#include "msa.h"
#include "info.h"
#include <algorithm>

static vector<vector<unsigned> > g_Cliques;

static void ComputeCliques(Mx<bool> &GAf, vector<vector<unsigned> > &Cliques)
	{
	bool **GA = GAf.GetData();

	const unsigned N = GAf.m_RowCount;
	asserta(GAf.m_ColCount == N);
	vector<bool> Done(N, false);
	for (;;)
		{
		vector<unsigned> Clique;
		for (unsigned i = 0; i < N; ++i)
			{
			if (Done[i])
				continue;
			Clique.push_back(i);
			Done[i] = true;
			break;
			}

		for (unsigned k = 0; k < N; ++k)
			{
			if (Done[k])
				continue;

			bool Add = true;
			for (unsigned j = 0; j < SIZE(Clique); ++j)
				{
				unsigned c = Clique[j];
				if (!GA[c][k])
					{
					Add = false;
					break;
					}
				}
			if (Add)
				{
				Clique.push_back(k);
				Done[k] = true;
				}
			}
		if (Clique.empty())
			return;
		Cliques.push_back(Clique);
		}
	}

void LogCliques(SeqDB &DB, const vector<vector<unsigned> > &Cliques)
	{
	Log("\n");
	Log("Globally alignable subsets:\n");
	Log("Subset  Label\n");
	Log("------  -----\n");
	const unsigned CliqueCount = SIZE(Cliques);
	for (unsigned CliqueIndex = 0; CliqueIndex < CliqueCount; ++CliqueIndex)
		{
		if (CliqueIndex > 0)
			Log("\n");
		const vector<unsigned> &Clique = Cliques[CliqueIndex];
		const unsigned N = SIZE(Clique);
		for (unsigned i = 0; i < N; ++i)
			{
			unsigned SeqIndex = Clique[i];
			const char *Label = DB.GetLabel(SeqIndex);
			Log("%6u  %s\n", CliqueIndex + 1, Label);
			}
		}
	}

void ComputeGloballyAlignableSubsets()
	{
#if	0
	Mx<bool> GloballyAlignablef;
	GloballyAlignablef.Alloc("GloballyAlignable", SeqCount, SeqCount);
	GloballyAlignablef.Init(true);
	bool **GloballyAlignable = GloballyAlignablef.GetData();

	const vector<TranslocInfo> &Translocs = GetTranslocInfos();
	const vector<InvertInfo> &Inverts = GetInvertInfos();
	bool GloballyAlignablePair = (Count == 0);
		GloballyAlignable[SeqIndex1][SeqIndex2] = GloballyAlignablePair;
		GloballyAlignable[SeqIndex2][SeqIndex1] = GloballyAlignablePair;

	ComputeCliques(GloballyAlignablef, g_Cliques);
#endif
	}
