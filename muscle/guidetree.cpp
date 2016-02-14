#include "myutils.h"
#include "mx.h"
#include "seqdb.h"
#include "tree.h"

void SeqDB::ComputeGuideTree()
	{
	if (!m_GuideTree.Empty())
		return;

	if (opt_trace)
		Log("ComputeGuideTree()\n");

	m_GuideTree.Clear();
	m_GuideTree.Init(m_Labels);

	const unsigned NodeCount = m_GuideTree.GetNodeCount();
	const unsigned LeafCount = m_GuideTree.GetLeafCount();
	const unsigned InternalNodeCount = m_GuideTree.GetInternalNodeCount();

	if (opt_fasttree)
		ComputeFastIdMx();
	Mx<float> &DistMxf = (opt_fasttree ? GetIdMxf() : GetAccMxf());

// Full distance matrix including internal nodes
	Mx<float> DistMx2f;
	DistMx2f.Alloc("DistMx2", NodeCount, NodeCount);

	const float * const *DistMx = DistMxf.GetData();
	float **DistMx2 = DistMx2f.GetData();

// Set leaf distances to expected accuracy
	for (unsigned i = 0; i < LeafCount; ++i)
		for (unsigned j = 0; j < LeafCount; ++j)
				DistMx2[i][j] = DistMx[i][j];

// Initialize internal distances as defensive check
	for (unsigned i = LeafCount; i < NodeCount; ++i)
		for (unsigned j = 0; j < NodeCount; ++j)
			DistMx2[i][j] = DistMx2[j][i] = -1.0f;

	vector<bool> Pending(NodeCount);
	vector<unsigned> ClusterSize(NodeCount);

// Leaf nodes
	for (unsigned i = 0; i < LeafCount; ++i)
		{
		Pending[i] = true;
		ClusterSize[i] = 1;
		m_GuideTree.SetUser(i, i);
		}

// Internal nodes
	for (unsigned i = LeafCount; i < NodeCount; ++i)
		Pending[i] = false;

	for (unsigned i = 0; i < InternalNodeCount; ++i)
		{
	// Find closest pair of clusters
		float BestProb = -1;
		unsigned Bestj = UINT_MAX;
		unsigned Bestk = UINT_MAX;
		for (unsigned j = 0; j < NodeCount; ++j)
			{
			if (!Pending[j])
				continue;
			for (unsigned k = 0; k < j; ++k)
				{
				if (!Pending[k])
					continue;
				float Prob = DistMx2[j][k];
				if (Prob < -0.1f)
					Warning("PCCluster P=%g", Prob);
				if (Prob > BestProb)
					{
					BestProb = Prob;
					Bestj = j;
					Bestk = k;
					}
				}
			}
		asserta(Bestj != UINT_MAX && Bestk != UINT_MAX);

	// Join j and k
		if (opt_trace)
			Log("Bestj=%u(%s) Bestk=%u(%s) BestProb=%g\n",
			  Bestj,
			  m_GuideTree.GetLabel(Bestj).c_str(),
			  Bestk,
			  m_GuideTree.GetLabel(Bestk).c_str(),
			  BestProb);

		double BranchLength = (1.0f - BestProb)/2.0f;
		unsigned NewNodeIndex = m_GuideTree.Join(Bestj, BranchLength, Bestk, BranchLength);
		asserta(NewNodeIndex >= LeafCount && NewNodeIndex < NodeCount);
		ClusterSize[NewNodeIndex] = ClusterSize[Bestj] + ClusterSize[Bestk];
	
		Pending[Bestj] = false;
		Pending[Bestk] = false;
		Pending[NewNodeIndex] = true;

		for (unsigned NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
			{
			if (!Pending[NodeIndex])
				continue;

			float Accj = DistMx2[NodeIndex][Bestj];
			float Acck = DistMx2[NodeIndex][Bestk];

			float Acc;
			if (opt_clustersize)
				{
				unsigned Nj = ClusterSize[Bestj];
				unsigned Nk = ClusterSize[Bestk];
				Acc = (Nj*Accj + Nk*Acck)*BestProb/(Nj + Nk);
				}
			else
				Acc = (Accj + Acck)*BestProb/2.0f;

			DistMx2[NodeIndex][NewNodeIndex] = Acc;
			DistMx2[NewNodeIndex][NodeIndex] = Acc;
			}
		}

	if (opt_trace)
		m_GuideTree.LogNewick(m_GuideTree.GetRootNodeIndex());
	return;
	}
