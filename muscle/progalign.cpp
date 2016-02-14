#include "myutils.h"
#include "tree.h"
#include "sparsemx.h"
#include "seqdb.h"

#define TRACE	0

float AlignTwoMSAs(SeqDB &Input, const SeqDB &msa1, const SeqDB &msa2,
  SeqDB &OutMSA);
float ComputeColProbs(SeqDB &Input, const SeqDB &msa,
  vector<unsigned> &LetterPairCounts, vector<float> &PairProbs,
  vector<float> &ColProbs, float &SumPairProbs, float &SumColProbs);

static vector<SeqDB *> g_InternalNodeMSAs;
static vector<SparseMx *> *g_SPPs;
static vector<float> g_NodeAccs;
static unsigned g_SubfamCounter = 0;
static string g_SubFamFilenamePrefix;
static SeqDB *g_Input;
static unsigned g_NodeCounter = 0;

static bool LogNodeAccsOnNode(const Tree &t, unsigned NodeIndex, void *ptrCounter)
	{
	if (g_SubFamFilenamePrefix == "")
		return false;
	if (t.IsLeaf(NodeIndex))
		return true;
	unsigned &Counter = *(unsigned *) ptrCounter;
	++Counter;
	asserta(NodeIndex < SIZE(g_NodeAccs));
	float Acc = g_NodeAccs[NodeIndex];
	Log("%6u  %7.1f%% ", Counter, 100.0f*Acc);

	vector<unsigned> LeafIndexes;
	t.GetLeafIndexes(NodeIndex, LeafIndexes);
	for (unsigned i = 0; i < SIZE(LeafIndexes); ++i)
		{
		unsigned LeafIndex = LeafIndexes[i];
		unsigned Id = t.GetUser(LeafIndex);
		Log(" %u", Id);
		}
	Log("\n");
	return true;
	}

void LogNodeAccs(const Tree &GuideTree)
	{
	if (g_SubFamFilenamePrefix == "")
		return;
	if (g_NodeAccs.empty())
		return;

	asserta(SIZE(g_NodeAccs) == GuideTree.GetNodeCount());

	Log("\n");
	Log("Subfamily accuracies\n");
	Log("Subfam  Accuracy  Sequences\n");
	Log("------  --------  ---------\n");
	unsigned Counter = 0;
	GuideTree.Traverse(LogNodeAccsOnNode, &Counter);
	}

bool OnGuideTreeNode(const Tree &GuideTree, unsigned NodeIndex, void *vDB)
	{
	const unsigned NodeCount = GuideTree.GetNodeCount();
	ProgressStep(g_NodeCounter++, NodeCount, "Progressive");
#if	TRACE
	Log("\n");
	Log("OnGuideTreeNode(%u)\n", NodeIndex);
#endif
	if (g_InternalNodeMSAs.empty())
		g_InternalNodeMSAs.resize(NodeCount);

	SeqDB &DB = *((SeqDB *) vDB);
	if (GuideTree.IsLeaf(NodeIndex))
		{
		unsigned SeqIndex = GuideTree.GetUser(NodeIndex);
		byte *Seq = DB.GetSeq(SeqIndex);
		unsigned Length = DB.GetSeqLength(SeqIndex);

		asserta(NodeIndex < SIZE(g_InternalNodeMSAs));
		SeqDB *msa = new SeqDB;
		if (msa == 0)
			Die("Out of memory");

		g_InternalNodeMSAs[NodeIndex] = msa;
		const string &Label = DB.GetLabel(SeqIndex);
		unsigned InputSeqIndex = DB.GetUser(SeqIndex);
		unsigned Lo = DB.GetLo(SeqIndex);
		bool Strand = DB.GetStrand(SeqIndex);
		msa->AddSeq(Label, Seq, Length, 1.0f, InputSeqIndex, Lo, Strand);
		if (NodeIndex >= SIZE(g_NodeAccs))
			g_NodeAccs.resize(NodeIndex+1);
		g_NodeAccs[NodeIndex] = 1.0f;

#if	TRACE
		Log(" Seq Id=%u Label=%s\n", Id, DB.GetLabel(Id).c_str());
#endif
		}
	else
		{
		unsigned Left = GuideTree.GetLeft(NodeIndex);
		unsigned Right = GuideTree.GetRight(NodeIndex);
#if	TRACE
		Log(" L=%u R=%u\n", Left, Right);
#endif

		SeqDB &msa1 = *g_InternalNodeMSAs[Left];
		SeqDB &msa2 = *g_InternalNodeMSAs[Right];
		SeqDB &msa = *new SeqDB;
		if (&msa == 0)
			Die("Out of memory");
		g_InternalNodeMSAs[NodeIndex] = &msa;
		AlignTwoMSAs(DB, msa1, msa2, msa);
		for (unsigned Iter = 0; Iter < opt_subrefine; ++Iter)
			{
			bool opt_refinetree_save = opt_refinetree;
			opt_refinetree = false;
			(*g_Input).Refine(msa, Iter, opt_subrefine);
			opt_refinetree = opt_refinetree_save;
			}

		if (g_SubFamFilenamePrefix != "")
			{
			vector<unsigned> LetterPairCounts;
			vector<float> PairProbs;
			vector<float> ColProbs;
			float SumPairProbs;
			float SumColProbs;
			float Acc = ComputeColProbs(DB, msa, LetterPairCounts,
			  PairProbs, ColProbs, SumPairProbs, SumColProbs);
			msa.m_Accuracy = Acc;
			if (NodeIndex >= SIZE(g_NodeAccs))
				g_NodeAccs.resize(NodeIndex+1);
			g_NodeAccs[NodeIndex] = Acc;

			if (msa.GetSeqCount() > 1)
				{
				++g_SubfamCounter;
				char s[16];
				sprintf(s, "Node%u.Acc%.0f", g_SubfamCounter, Acc*100.0);
				string FileName = g_SubFamFilenamePrefix + s;
				msa.ToFasta(FileName);
				}
			}

		if (!GuideTree.IsLeaf(Left))
			delete &msa1;
		if (!GuideTree.IsLeaf(Right))
			delete &msa2;
		}
	return true;
	}

SeqDB &SeqDB::ProgressiveAlign(const string &SubFamFilenamePrefix)
	{
	g_Input = this;
	const unsigned SeqCount = GetSeqCount();
	if (SeqCount == 1)
		return *this;

	g_SubFamFilenamePrefix = SubFamFilenamePrefix;

	ComputeGuideTree();
	
	g_SPPs = &m_SPPs;
	g_InternalNodeMSAs.clear();
	g_NodeAccs.clear();
	g_NodeCounter = 0;
	m_GuideTree.Traverse(OnGuideTreeNode, this);
	LogNodeAccs(m_GuideTree);

	unsigned RootNodeIndex = m_GuideTree.GetRootNodeIndex();
	asserta(RootNodeIndex < SIZE(g_InternalNodeMSAs));
	return *g_InternalNodeMSAs[RootNodeIndex];
	}
