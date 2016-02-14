#include "myutils.h"
#include "tree.h"
#include "seqdb.h"

#define TRACE 0

/***
Compute weights by the CLUSTALW method.
Thompson, Higgins and Gibson (1994), CABIOS (10) 19-29;
see also CLUSTALW paper.

m_Weights are computed from the edge lengths of a rooted tree.

Define the strength of an edge to be its length divided by the number
of leaves under that edge. The weight of a sequence is then the sum
of edge strengths on the path from the root to the leaf.

Example.

        0.2
       -----A     0.1
	 -x         ------- B     0.7
	   --------y           ----------- C
	    0.3     ----------z
                    0.4    -------------- D
                                 0.8

Edge	Length	Leaves	Strength
----	-----	------	--------
xy		0.3		3		0.1
xA		0.2		1		0.2
yz		0.4		2		0.2
yB		0.1		1		0.1
zC		0.7		1		0.7
zD		0.8		1		0.8

Leaf	Path		Strengths			Weight
----	----		---------			------
A		xA			0.2					0.2
B		xy-yB		0.1 + 0.1			0.2
C		xy-yz-zC	0.1 + 0.2 + 0.7		1.0
D		xy-yz-zD	0.1 + 0.2 + 0.8		1.1

***/

static unsigned CountLeaves(const Tree &tree, unsigned NodeIndex,
  vector<unsigned> &LeavesUnderNode)
	{
	if (tree.IsLeaf(NodeIndex))
		{
		LeavesUnderNode[NodeIndex] = 1;
		return 1;
		}

	const unsigned Left = tree.GetLeft(NodeIndex);
	const unsigned Right = tree.GetRight(NodeIndex);
	const unsigned RightCount = CountLeaves(tree, Right, LeavesUnderNode);
	const unsigned LeftCount = CountLeaves(tree, Left, LeavesUnderNode);
	const unsigned Count = RightCount + LeftCount;
	LeavesUnderNode[NodeIndex] = Count;
	return Count;
	}

void SeqDB::ComputeSeqWeights(const Tree &tree)
	{
#if	TRACE
	Log("SeqDB::ComputeSeqWeights\n");
	tree.LogMe();
#endif

	m_Weights.clear();
	const unsigned LeafCount = tree.GetLeafCount();
	asserta(LeafCount == GetSeqCount());

	m_Weights.resize(LeafCount, 1.0f);
	if (LeafCount <= 2)
		return;

	if (!tree.IsRooted())
		Die("ComputeSeqWeights requires rooted tree");

	const unsigned NodeCount = tree.GetNodeCount();
	vector<unsigned> LeavesUnderNode(NodeCount, 0);

	const unsigned RootNodeIndex = tree.GetRootNodeIndex();
	unsigned LeavesUnderRoot = CountLeaves(tree, RootNodeIndex, LeavesUnderNode);
	if (LeavesUnderRoot != LeafCount)
		Die("WeightsFromTreee: Internal error, root count %u %u",
		  LeavesUnderRoot, LeafCount);

#if	TRACE
	Log("Node  Leaves    Length  Strength\n");
	Log("----  ------  --------  --------\n");
	//    1234  123456  12345678  12345678
#endif

	vector<float> Strengths(NodeCount);
	for (unsigned NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
		{
		if (tree.IsRoot(NodeIndex))
			{
			Strengths[NodeIndex] = 0.0f;
			continue;
			}
		const float Length = tree.GetBranchLength(NodeIndex);
		const unsigned LeafCount = LeavesUnderNode[NodeIndex];
		const float Strength = Length / (float) LeafCount;
		Strengths[NodeIndex] = Strength;
#if	TRACE
		Log("%4u  %6u  %8g  %8g\n", NodeIndex, LeafCount, Length, Strength);
#endif
		}

#if	TRACE
	Log("\n");
	Log("                 Seq  Path..Weight\n");
	Log("--------------------  ------------\n");
#endif
	float SumWeights = 0.0f;
	for (unsigned Node = 0; Node < NodeCount; ++Node)
		{
		if (!tree.IsLeaf(Node))
			continue;
#if	TRACE
		Log("%20.20s  %4u ", tree.GetName(Node).c_str(), Node);
#endif
		float Weight = 0.0f;
		unsigned Node2 = Node;
		while (!tree.IsRoot(Node2))
			{
			Weight += Strengths[Node2];
			Node2 = tree.GetParent(Node2);
#if	TRACE
			Log("->%u(%g)", Node2, Strengths[Node2]);
#endif
			}
		if (Weight < 0.0001f)
			Weight = 0.0001f;

		unsigned SeqIndex = tree.GetUser(Node);
		asserta(SeqIndex < LeafCount);
		asserta(tree.GetLabel(Node) == GetLabel(SeqIndex));
		m_Weights[SeqIndex] = Weight;
		SumWeights += Weight;
#if	TRACE
		Log(" = %g\n", Weight);
#endif
		}

	float f = LeafCount/SumWeights;
	for (unsigned i = 0; i < LeafCount; ++i)
		m_Weights[i] *= f;

#if	TRACE
	{
	Log("\n");
	Log("           Label    Weight\n");
	Log("----------------  --------\n");
	for (unsigned i = 0; i < LeafCount; ++i)
		Log("%16.16s  %8.4f\n", GetLabel(i), m_Weights[i]);
	}
#endif
	}
