#include <stdlib.h>
#include <list>
#include "myutils.h"
#include "tree.h"
#include "sparsemx.h"
#include "seqdb.h"

float AlignTwoMSAs(SeqDB &Input, const SeqDB &msa1,const SeqDB &msa2, SeqDB &OutMSA);

static void GetRandom2e(unsigned N,
  vector<unsigned> &Indexes1, vector<unsigned> &Indexes2)
	{
	asserta(N >= 2);
	Indexes1.clear();
	Indexes2.clear();

	list<unsigned> Order;
	for (unsigned i = 0; i < N; ++i)
		{
		if (rand()%2 == 0)
			Order.push_front(i);
		else
			Order.push_back(i);
		}

	list<unsigned>::const_iterator p = Order.begin();
	for (unsigned i = 0; i < N/2; ++i)
		Indexes1.push_back(*p++);
	for (unsigned i = N/2; i < N; ++i)
		Indexes2.push_back(*p++);
	asserta(p == Order.end());
	}

static void GetRandom2(unsigned N,
  vector<unsigned> &Indexes1, vector<unsigned> &Indexes2)
	{
	asserta(N >= 2);
	for (;;)
		{
		Indexes1.clear();
		Indexes2.clear();
		for (unsigned i = 0; i < N; ++i)
			{
			if (rand()%2 == 0)
				Indexes1.push_back(i);
			else
				Indexes2.push_back(i);
			}
		if (!Indexes1.empty() && !Indexes2.empty())
			return;
		}
	}

static void GetRandom3(unsigned N,
  vector<unsigned> &Indexes1, vector<unsigned> &Indexes2,
  vector<unsigned> &Indexes3)
	{
	asserta(N >= 3);
	for (;;)
		{
		Indexes1.clear();
		Indexes2.clear();
		Indexes3.clear();
		for (unsigned i = 0; i < N; ++i)
			{
			switch (rand()%3)
				{
			case 0: Indexes1.push_back(i); continue;
			case 1: Indexes2.push_back(i); continue;
			case 2: Indexes3.push_back(i); continue;
				}
			}
		if (!Indexes1.empty() && !Indexes2.empty() && !Indexes3.empty())
			return;
		}
	}

static void RefineRandom2(SeqDB &Input, SeqDB &msa, bool Equal)
	{
	const unsigned SeqCount = msa.GetSeqCount();
	if (SeqCount <= 2)
		return;

	vector<unsigned> Indexes1;
	vector<unsigned> Indexes2;
	if (Equal)
		GetRandom2e(SeqCount, Indexes1, Indexes2);
	else
		GetRandom2(SeqCount, Indexes1, Indexes2);

	SeqDB msa1;
	SeqDB msa2;

	msa1.CopySubset(msa, Indexes1);
	msa2.CopySubset(msa, Indexes2);

	msa1.StripGapCols();
	msa2.StripGapCols();

	AlignTwoMSAs(Input, msa1, msa2, msa);
	}

static void RefineRandom3(SeqDB &Input, SeqDB &msa)
	{
	const unsigned SeqCount = msa.GetSeqCount();
	if (SeqCount <= 3)
		return;

	vector<unsigned> Indexes1;
	vector<unsigned> Indexes2;
	vector<unsigned> Indexes3;
	GetRandom3(SeqCount, Indexes1, Indexes2, Indexes3);

	SeqDB msa1;
	SeqDB msa2;
	SeqDB msa3;

	msa1.CopySubset(msa, Indexes1);
	msa2.CopySubset(msa, Indexes2);
	msa3.CopySubset(msa, Indexes3);

	msa1.StripGapCols();
	msa2.StripGapCols();

	SeqDB msa12;
	AlignTwoMSAs(Input, msa1, msa2, msa12);
	AlignTwoMSAs(Input, msa12, msa3, msa);
	}

static void RefineTreeNode(SeqDB &Input, SeqDB &msa, const Tree &tree, unsigned NodeIndex)
	{
	const unsigned NodeCount = tree.GetNodeCount();
	const unsigned LeafCount = tree.GetLeafCount();

	vector<unsigned> LeafIndexes1;
	tree.GetLeafIndexes(NodeIndex, LeafIndexes1);
	const unsigned LeafCount1 = SIZE(LeafIndexes1);
	asserta(LeafCount1 != 0);

	vector<bool> In1(NodeCount, false);
	for (unsigned i = 0; i < LeafCount1; ++i)
		In1[LeafIndexes1[i]] = true;

	vector<unsigned> LeafIndexes2;
	for (unsigned i = 0; i < NodeCount; ++i)
		if (tree.IsLeaf(i) && !In1[i])
			LeafIndexes2.push_back(i);
	const unsigned LeafCount2 = SIZE(LeafIndexes2);
	asserta(LeafCount1 + LeafCount2 == LeafCount);

	vector<unsigned> Indexes1;
	vector<unsigned> Indexes2;

	for (unsigned i = 0; i < LeafCount1; ++i)
		{
		unsigned NodeIndex = LeafIndexes1[i];
		unsigned SeqIndex = tree.GetUser(NodeIndex);
		Indexes1.push_back(SeqIndex);
		}

	for (unsigned i = 0; i < LeafCount2; ++i)
		{
		unsigned NodeIndex = LeafIndexes2[i];
		unsigned SeqIndex = tree.GetUser(NodeIndex);
		Indexes2.push_back(SeqIndex);
		}

	SeqDB msa1;
	SeqDB msa2;

	msa1.CopySubset(msa, Indexes1);
	msa2.CopySubset(msa, Indexes2);

	msa1.StripGapCols();
	msa2.StripGapCols();

	AlignTwoMSAs(Input, msa1, msa2, msa);
	}

void SeqDB::Refine(SeqDB &msa, unsigned /*Iter*/, unsigned Iters)
	{
	const unsigned SeqCount = GetSeqCount();
	const unsigned NodeCount = GetSeqCount();
	unsigned TotalCount = 0;
	if (opt_refinerand2)
		TotalCount += SeqCount;
	if (opt_refinerand3)
		TotalCount += SeqCount;
	if (opt_refinetree)
		TotalCount += NodeCount - 1;

	bool RefineRand2 = opt_refinerand2;
	bool RefineRand3 = opt_refinerand3;
	bool RefineTree = opt_refinetree;
	if (opt_refinemix > 0)
		{
		if (SeqCount <= opt_refinemix)
			{
			RefineRand2 = false;
			RefineRand3 = false;
			RefineTree = true;
			}
		else
			{
			RefineRand2 = true;
			RefineRand3 = false;
			RefineTree = false;
			}
		}

	TotalCount *= Iters;
	if (RefineRand2)
		for (unsigned i = 0; i < SeqCount; ++i)
			{
//			ProgressStep(Counter++, TotalCount, "Refine %u/%u", Iter, Iters);
			RefineRandom2(*this, msa, opt_refinerand2e);
			}

	if (RefineRand3)
		for (unsigned i = 0; i < SeqCount; ++i)
			{
//			ProgressStep(Counter++, TotalCount, "Refine %u/%u", Iter, Iters);
			RefineRandom3(*this, msa);
			}
//TODO does this work better now?@@	
	if (RefineTree)
		for (unsigned i = 0; i < NodeCount; ++i)
			{
			unsigned NodeIndex = (opt_refinerandtree ? rand()%NodeCount : i);
			if (m_GuideTree.IsRoot(NodeIndex))
				continue;
			//if (Counter >= TotalCount)
			//	TotalCount += 100;
			//ProgressStep(Counter++, TotalCount, "Refine %u/%u", Iter, Iters);
			RefineTreeNode(*this, msa, m_GuideTree, NodeIndex);
			}
	}
