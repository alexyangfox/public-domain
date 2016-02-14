#include <float.h>
#include "myutils.h"
#include "tree.h"

// http://evolution.genetics.washington.edu/phylip/newicktree.html

#if	0
void TestTree()
	{
	Tree T;
	T.FromFile("/p/tmp/test.phy");
	T.LogMe();
	}
#endif

static int GetNextNonSpaceChar(FILE *f)
	{
	for (;;)
		{
		int c = getc(f);
		if (c == EOF)
			Die("GetNextNonSpaceChar, end-of-file");
		if (!isspace(c))
			return c;
		}
	}

static void SkipWhiteSpace(FILE *f)
	{
	for (;;)
		{
		int c = getc(f);
		if (c == -1)
			return;
		if (!isspace(c))
			{
			ungetc(c, f);
			return;
			}
		}
	}

bool isfloatc(int c)
	{
	if (isdigit(c))
		return true;
	switch (c)
		{
	case '.':
	case 'e':
	case 'E':
	case 'g':
	case 'G':
	case '+':
	case '-':
		return true;
		}
	return false;
	}

static void GetOptionalBranchLength(FILE *f, double &BranchLength)
	{
	BranchLength = FLT_MAX;

	int c = GetNextNonSpaceChar(f);
	if (c != ':')
		{
		ungetc(c, f);
		return;
		}
	SkipWhiteSpace(f);
	string s;
	for (;;)
		{
		int c = getc(f);
		if (c == EOF)
			Die("GetOptionalBranchLength, end-of-file");
		if (!isfloatc(c))
			{
			ungetc(c, f);
			BranchLength = (double) atof(s.c_str());
			return;
			}
		s.push_back((char) c);
		}
	}

static void GetNewickName(FILE *f, string &Name)
	{
	Name.clear();

	SkipWhiteSpace(f);
	for (;;)
		{
		int c = getc(f);
		if (c == EOF)
			Die("GetNewickName, end-of-file");
		if (isspace(c))
			return;
		if (c == ':' || c == ',' || c == ')' || c == ';')
			{
			ungetc(c, f);
			return;
			}
		Name.push_back((char) c);
		}
	}

static void GetNameAndOptionalBranchlength(FILE *f, string &Name,
  double &BranchLength)
	{
	Name.clear();
	BranchLength = FLT_MAX;

	GetNewickName(f, Name);
	GetOptionalBranchLength(f, BranchLength);
	}

void Tree::Clear()
	{
	m_Rooted = true;
	m_RootNodeIndex = UINT_MAX;
	m_NextNodeIndex = 0;
	m_Lefts.clear();
	m_Rights.clear();
	m_BranchLengths.clear();
	m_Labels.clear();
	m_Users.clear();
	}

void Tree::ToFileRecurse(FILE *f, unsigned NodeIndex) const
	{
	if (IsLeaf(NodeIndex))
		fprintf(f, "%s:%g", GetLabel(NodeIndex).c_str(), GetBranchLength(NodeIndex));
	else
		{
		fprintf(f, "(");
		ToFileRecurse(f, GetLeft(NodeIndex));
		fprintf(f, ",\n");
		ToFileRecurse(f, GetRight(NodeIndex));
		fprintf(f, ")");
		}
	}

void Tree::ToFile(const char *FileName) const
	{
	FILE *f = CreateStdioFile(FileName);
	ToFileRecurse(f, GetRootNodeIndex());
	fprintf(f, ";\n");
	CloseStdioFile(f);
	}

static unsigned g_TreeDepth;

void Tree::LogNewick(unsigned NodeIndex) const
	{
	for (unsigned i = 0; i < g_TreeDepth; ++i)
		Log(" ");
	++g_TreeDepth;
	if (NodeIndex == UINT_MAX)
		NodeIndex = GetRootNodeIndex();
	if (IsLeaf(NodeIndex))
		Log("%s", GetLabel(NodeIndex).c_str());
	else
		{
		Log("(\n");
		LogNewick(GetLeft(NodeIndex));
		Log(",\n");
		LogNewick(GetRight(NodeIndex));
		Log(")\n");
		}
	if (IsRoot(NodeIndex))
		Log(";\n");
	--g_TreeDepth;
	}

void Tree::LogMe() const
	{
	const unsigned NodeCount = GetNodeCount();
	Log("\n");
	Log("%s, %u nodes, %u leaves",
	  (m_Rooted ? "Rooted" : "Unrooted"),
	  NodeCount,
	  GetLeafCount());
	if (m_Rooted)
		Log(" root=%u", m_RootNodeIndex);
	Log("\n");
	Log("Index   Left  Right    Branch        User  Name\n");
	Log("=====  =====  =====  ========  ==========  ====\n");
	for (unsigned NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
		{
		unsigned L = m_Lefts[NodeIndex];
		unsigned R = m_Rights[NodeIndex];
		double B = m_BranchLengths[NodeIndex];

		Log("%5u", NodeIndex);
		if (L == UINT_MAX)
			Log("       ");
		else
			Log("  %5u", L);

		if (R == UINT_MAX)
			Log("       ");
		else
			Log("  %5u", R);

		if (B == FLT_MAX)
			Log("         *");
		else
			Log("  %8.4f", m_BranchLengths[NodeIndex]);

		if (m_Users[NodeIndex] == UINT_MAX)
			Log("            ");
		else
			Log("  %10u", m_Users[NodeIndex]);

		Log("  %.32s", m_Labels[NodeIndex].c_str());
		Log("\n");
		}
	}

void Tree::Validate() const
	{
	const unsigned NodeCount = GetNodeCount();

	asserta(SIZE(m_Lefts) == NodeCount);
	asserta(SIZE(m_Rights) == NodeCount);
	asserta(SIZE(m_Users) == NodeCount);
	asserta(SIZE(m_Labels) == NodeCount);

	asserta(m_RootNodeIndex < NodeCount);

	vector<bool> NodeReferenced(NodeCount, false);
	NodeReferenced[m_RootNodeIndex] = true;
	unsigned InternalCount = 0;
	unsigned LeafCount = 0;
	for (unsigned NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
		{
		if (IsInternal(NodeIndex))
			{
			++InternalCount;
			unsigned Left = GetLeft(NodeIndex);
			unsigned Right = GetRight(NodeIndex);
			asserta(Left < NodeCount);
			asserta(Right < NodeCount);
			if (NodeReferenced[Left])
				{
				LogMe();
				Die("Node %u referenced twice", Left);
				}
			if (NodeReferenced[Right])
				{
				LogMe();
				Die("Node %u referenced twice", Right);
				}
			NodeReferenced[Left] = true;
			NodeReferenced[Right] = true;
			}
		else
			++LeafCount;
		}

	if (InternalCount != GetInternalNodeCount())
		{
		LogMe();
		Die("InternalCount != GetInternalNodeCount()");
		}

	if (LeafCount != GetLeafCount())
		{
		LogMe();
		Die("LeafCount != GetLeafNodeCount()");
		}

	for (unsigned NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
		{
		if (!NodeReferenced[NodeIndex])
			{
			LogMe();
			Die("Node %u not referenced", NodeIndex);
			}
		}
	}

// Leaf is <n>:<L> where n is value and L is branch length.
// Internal node is (<node>,<node>)[Label]
unsigned Tree::NodeFromNewickFile(FILE *f)
	{
	unsigned NodeIndex = SIZE(m_Lefts);
	m_Lefts.push_back(UINT_MAX);
	m_Rights.push_back(UINT_MAX);
	m_Labels.push_back("");
	m_BranchLengths.push_back(FLT_MAX);

	unsigned ThirdNodeIndex = UINT_MAX;
	int c = GetNextNonSpaceChar(f);
	if (c == '(')
		{
		unsigned LeftIndex = NodeFromNewickFile(f);
		c = GetNextNonSpaceChar(f);
		if (c != ',')
			Die("TreeNodeFromFile, expected ',' but got '%c'", c);

		unsigned RightIndex = NodeFromNewickFile(f);

		c = GetNextNonSpaceChar(f);
		if (NodeIndex == 0 && c == ',')
			{
			ThirdNodeIndex = NodeFromNewickFile(f);
			c = GetNextNonSpaceChar(f);
			}

		if (c != ')')
			Die("Expected ')', got '%c'", c);

		string Label;
		GetNewickName(f, Label);

		double BranchLength = FLT_MAX;
		if (NodeIndex > 0)
			GetOptionalBranchLength(f, BranchLength);
		m_Lefts[NodeIndex] = LeftIndex;
		m_Rights[NodeIndex] = RightIndex;
		m_Labels[NodeIndex] = Label;
		m_BranchLengths[NodeIndex] = BranchLength;
		}
	else
		{
		ungetc(c, f);
		double BranchLength = FLT_MAX;
		string Name;
		GetNameAndOptionalBranchlength(f, Name, BranchLength);
		m_Lefts[NodeIndex] = UINT_MAX;
		m_Rights[NodeIndex] = UINT_MAX;
		m_Labels[NodeIndex] = Name;
		m_BranchLengths[NodeIndex] = BranchLength;
		}

	if (ThirdNodeIndex != UINT_MAX)
		{
		m_RootNodeIndex = SIZE(m_Lefts);
		m_Lefts.push_back(NodeIndex);
		m_Rights.push_back(ThirdNodeIndex);
		m_BranchLengths.push_back(FLT_MAX);
		m_Labels.push_back("[PseudoRoot]");
		NodeIndex = m_RootNodeIndex;
		}

	if (NodeIndex == m_RootNodeIndex)
		{
		c = GetNextNonSpaceChar(f);
		if (c != ';')
			Die("NodeFromNewickFile, root node, expected ',' or ';', got '%c'", c);
		}
	return NodeIndex;
	}

unsigned Tree::GetFirstDepthFirstNodeIndex() const
	{
	if (GetNodeCount() < 2)
		Die("Cannot traverse tree with < 2 nodes"); 

	m_TraverseStack.clear();

	unsigned NodeIndex = GetRootNodeIndex();
	m_TraverseStack.push_back(NodeIndex);
	for (;;)
		{
		NodeIndex = GetLeft(NodeIndex);
		m_TraverseStack.push_back(NodeIndex);
		if (IsLeaf(NodeIndex))
			break;
		}
	return NodeIndex;
	}

void Tree::Traverse(ptrTravFn OnNode, void *UserData) const
	{
	unsigned NodeIndex = GetFirstDepthFirstNodeIndex();
	while (NodeIndex != UINT_MAX)
		{
		bool OK = OnNode(*this, NodeIndex, UserData);
		if (!OK)
			return;
		NodeIndex = GetNextDepthFirstNodeIndex();
		}
	}

unsigned Tree::GetNextDepthFirstNodeIndex() const
	{
	if (m_TraverseStack.empty())
		return UINT_MAX;

	unsigned NodeIndex = m_TraverseStack.back();
	m_TraverseStack.pop_back();
	if (NodeIndex == m_RootNodeIndex)
		return UINT_MAX;
	unsigned Parent = m_TraverseStack.back();
	if (GetRight(Parent) == NodeIndex)
		return Parent;

	NodeIndex = GetRight(Parent);
	m_TraverseStack.push_back(NodeIndex);
	for (;;)
		{
		if (IsLeaf(NodeIndex))
			break;
		NodeIndex = GetLeft(NodeIndex);
		m_TraverseStack.push_back(NodeIndex);
		}
	return NodeIndex;
	}

void Tree::FromFile(const string &FileName)
	{
	FILE *f = OpenStdioFile(FileName.c_str());
	Clear();
	m_RootNodeIndex = NodeFromNewickFile(f);
	CloseStdioFile(f);
	m_Users.clear();
	const unsigned NodeCount = SIZE(m_Lefts);
	m_Users.resize(NodeCount, UINT_MAX);
	for (unsigned i = 0; i < NodeCount; ++i)
		m_NameToNodeIndex[m_Labels[i]] = i;
	Validate();
	}

static void GetLeafIndexesRecurse(const Tree &t, unsigned NodeIndex,
  vector<unsigned> &Indexes)
	{
	if (t.IsLeaf(NodeIndex))
		{
		Indexes.push_back(NodeIndex);
		return;
		}
	else
		{
		GetLeafIndexesRecurse(t, t.GetLeft(NodeIndex), Indexes);
		GetLeafIndexesRecurse(t, t.GetRight(NodeIndex), Indexes);
		}
	}

void Tree::GetLeafIndexes(unsigned NodeIndex, vector<unsigned> &LeafIndexes) const
	{
	LeafIndexes.clear();
	GetLeafIndexesRecurse(*this, NodeIndex, LeafIndexes);
	}

void Tree::GetDepthsRecurse(unsigned NodeIndex, vector<double> &Depths) const
	{
	if (IsLeaf(NodeIndex))
		return;

	unsigned Left = GetLeft(NodeIndex);
	unsigned Right = GetRight(NodeIndex);
	double L = m_BranchLengths[Left];
	double R = m_BranchLengths[Right];
	if (L == FLT_MAX)
		Depths[Left] = FLT_MAX;
	else
		Depths[Left] = Depths[NodeIndex] + L;

	if (R == FLT_MAX)
		Depths[Right] = FLT_MAX;
	else
		Depths[Right] = Depths[NodeIndex] + L;

	GetDepthsRecurse(Left, Depths);
	GetDepthsRecurse(Right, Depths);
	}

void Tree::GetNodeDepthsRecurse(unsigned NodeIndex, vector<unsigned> &Depths) const
	{
	if (IsLeaf(NodeIndex))
		return;

	unsigned Left = GetLeft(NodeIndex);
	unsigned Right = GetRight(NodeIndex);
	Depths[Left] = Depths[NodeIndex] + 1;
	Depths[Right] = Depths[NodeIndex] + 1;

	GetNodeDepthsRecurse(Left, Depths);
	GetNodeDepthsRecurse(Right, Depths);
	}

void Tree::GetDepths(vector<double> &Depths) const
	{
	Depths.resize(GetNodeCount(), 0);
	unsigned Root = GetRootNodeIndex();
	Depths[Root] = 0;
	GetDepthsRecurse(Root, Depths);
	}

void Tree::GetNodeDepths(vector<unsigned> &Depths) const
	{
	Depths.resize(GetNodeCount(), 0);
	unsigned Root = GetRootNodeIndex();
	Depths[Root] = 0;
	GetNodeDepthsRecurse(Root, Depths);
	}

void Tree::GetInfixOrderRecurse(unsigned NodeIndex, vector<unsigned> &Order) const
	{
	if (IsLeaf(NodeIndex))
		{
		Order.push_back(NodeIndex);
		return;
		}
	unsigned Left = GetLeft(NodeIndex);
	unsigned Right = GetRight(NodeIndex);
	GetInfixOrderRecurse(Left, Order);
	Order.push_back(NodeIndex);
	GetInfixOrderRecurse(Right, Order);
	}

void Tree::GetPrefixOrderRecurse(unsigned NodeIndex, vector<unsigned> &Order) const
	{
	if (IsLeaf(NodeIndex))
		{
		Order.push_back(NodeIndex);
		return;
		}
	unsigned Left = GetLeft(NodeIndex);
	unsigned Right = GetRight(NodeIndex);
	GetPrefixOrderRecurse(Left, Order);
	GetPrefixOrderRecurse(Right, Order);
	Order.push_back(NodeIndex);
	}

void Tree::GetInfixOrder(vector<unsigned> &Order) const
	{
	Order.clear();
	GetInfixOrderRecurse(GetRootNodeIndex(), Order);
	}

void Tree::GetPrefixOrder(vector<unsigned> &Order) const
	{
	Order.clear();
	GetPrefixOrderRecurse(GetRootNodeIndex(), Order);
	}

void Tree::Init(const vector<string> &LeafNames)
	{
	asserta(!LeafNames.empty());
	Clear();
	m_Labels = LeafNames;
	const unsigned N = SIZE(LeafNames);
	for (unsigned i = 0; i < N; ++i)
		m_NameToNodeIndex[LeafNames[i]] = i;
	m_NextNodeIndex = N;

	const unsigned NodeCount = 2*N - 1;
	m_Labels.resize(NodeCount);
	m_Users.resize(NodeCount, UINT_MAX);
	m_Lefts.resize(NodeCount, UINT_MAX);
	m_Rights.resize(NodeCount, UINT_MAX);
	m_BranchLengths.resize(NodeCount, 0);
	}

unsigned Tree::Join(unsigned Node1, double BranchLength1, unsigned Node2,
  double BranchLength2, const string &Name)
	{
	const unsigned N = GetNodeCount();
	asserta(m_NextNodeIndex < N);
	unsigned NodeIndex = m_NextNodeIndex++;
	if (NodeIndex == N - 1)
		m_RootNodeIndex = NodeIndex;
	
	m_Lefts[NodeIndex] = Node1;
	m_Rights[NodeIndex] = Node2;
	m_BranchLengths[Node1] = BranchLength1;
	m_BranchLengths[Node2] = BranchLength2;
	m_Labels[NodeIndex] = Name;

	return NodeIndex;
	}

void Tree::Attach(const Tree &t, const string &Label)
	{
	unsigned AttachNodeIndex = GetNodeIndex(Label);
	unsigned OldNodeCount = GetNodeCount();
	unsigned tNodeCount = t.GetNodeCount();
	unsigned AddedNodeCount = tNodeCount - 1;
	unsigned NewNodeCount = OldNodeCount + AddedNodeCount;
	
	m_Lefts.resize(NewNodeCount);
	m_Rights.resize(NewNodeCount);
	m_BranchLengths.resize(NewNodeCount);
	m_Users.resize(NewNodeCount);
	m_Labels.resize(NewNodeCount);

	m_NextNodeIndex = OldNodeCount;

	unsigned tRootNodeIndex = t.GetRootNodeIndex();
	for (unsigned tNodeIndex = 0; tNodeIndex < tNodeCount; ++tNodeIndex)
		{
		if (tNodeIndex == tRootNodeIndex)
			{
			m_Lefts[AttachNodeIndex] = t.GetLeft(tNodeIndex) + OldNodeCount;
			m_Rights[AttachNodeIndex] = t.GetRight(tNodeIndex) + OldNodeCount;
			m_Labels[AttachNodeIndex] = "(Attach)";
			continue;
			}

		unsigned NewNodeIndex = tNodeIndex + OldNodeCount;
		if (tNodeIndex > tRootNodeIndex)
			--NewNodeIndex;

		const string &Name = t.GetLabel(tNodeIndex);
		m_Users[NewNodeIndex] = t.GetUser(tNodeIndex);
		m_BranchLengths[NewNodeIndex] = t.GetBranchLength(tNodeIndex);
		m_Labels[NewNodeIndex] = Name;
		m_NameToNodeIndex[Name] = NewNodeIndex;
		if (t.IsLeaf(tNodeIndex))
			{
			m_Lefts[NewNodeIndex] = UINT_MAX;
			m_Rights[NewNodeIndex] = UINT_MAX;
			}
		else
			{
			m_Lefts[NewNodeIndex] = t.GetLeft(tNodeIndex) + OldNodeCount;
			m_Rights[NewNodeIndex] = t.GetRight(tNodeIndex) + OldNodeCount;
			}
		}
	Validate();
	}

// @@ SLOOOWWWW!
unsigned Tree::GetParent(unsigned Node, bool DieOnErr) const
	{
	if (Node == m_RootNodeIndex)
		{
		if (DieOnErr)
			{
			LogMe();
			Die("Tree::GetParent(%u), root node", Node);
			}
		return UINT_MAX;
		}
	for (unsigned i = 0; i < GetNodeCount(); ++i)
		{
		if (m_Lefts[i] == UINT_MAX)
			continue;
		if (m_Lefts[i] == Node || m_Rights[i] == Node)
			return i;
		}
	if (DieOnErr)
		{
		LogMe();
		Die("Tree::GetParent(%u), not found", Node);
		}
	return UINT_MAX;
	}

static void GetLabel2(const Tree &t, unsigned NodeIndex, string &Label)
	{
	Label = t.GetLabel(NodeIndex).c_str();
	if (Label == "")
		{
		if (t.IsRoot(NodeIndex))
			Label = "root";
		else
			{
			char Tmp[32];
			if (t.IsInternal(NodeIndex))
				sprintf(Tmp, "internal%u", NodeIndex);
			else
				sprintf(Tmp, "leaf%u", NodeIndex);
			Label = Tmp;
			}
		}
	}

void Tree::LogPrefixOrder() const
	{
	vector<double> Depths;
	GetDepths(Depths);

	Log("\n");
	Log("   Node     Left    Right   Parent   Branch    Depth  Label\n");
	Log("-------  -------  -------  -------  -------  -------  -----\n");
	vector<unsigned> Order;
	GetPrefixOrder(Order);
	const unsigned NodeCount = GetNodeCount();
	asserta(SIZE(Order) == NodeCount);
	for (unsigned i = 0; i < NodeCount; ++i)
		{
		unsigned NodeIndex = Order[i];
		Log("%7u", NodeIndex);
		if (IsLeaf(NodeIndex))
			Log("  %7.7s  %7.7s", "*", "*");
		else
			{
			unsigned Left = GetLeft(NodeIndex);
			unsigned Right = GetRight(NodeIndex);
			Log("  %7u  %7u", Left, Right);
			}

		if (IsRoot(NodeIndex))
			Log("  %7.7s", "*");
		else
			{
			unsigned Parent = GetParent(NodeIndex);
			Log("  %7u", Parent);
			}

		if (IsRoot(NodeIndex))
			Log("  %7.4f", 0.0);
		else
			{
			double Branch = GetBranchLength(NodeIndex);
			if (Branch == FLT_MAX)
				Log("  %7.7s", "");
			else
				Log("  %7.4f", Branch);
			}
		double Depth = Depths[NodeIndex];
		if (Depth == FLT_MAX)
			Log("  %7.7s", "");
		else
			Log("  %7.4f", Depth);
		string Label;
		::GetLabel2(*this, NodeIndex, Label);
		Log("  %s\n", Label.c_str());
		}

	Log("\n");
	for (unsigned NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
		{
		if (!IsLeaf(NodeIndex))
			continue;
	
		const string &LeafLabel = GetLabel(NodeIndex);
		unsigned Node = NodeIndex;
		unsigned Parent = GetParent(Node);
		for (;;)
			{
			if (IsRoot(Parent))
				break;
			unsigned Grandparent = GetParent(Parent);

			string ParentLabel;
			string GrandparentLabel;
			::GetLabel2(*this, Parent, ParentLabel);
			::GetLabel2(*this, Grandparent, GrandparentLabel);

			Log("./ta1 %s.%s %s.%s %s.%s\n",
			  ParentLabel.c_str(), LeafLabel.c_str(),
			  GrandparentLabel.c_str(),
			  ParentLabel.c_str(),
			  GrandparentLabel.c_str(),
			  LeafLabel.c_str());

			if (IsRoot(Parent))
				break;
			Parent = Grandparent;
			}
		}
	Log("\n");
	for (unsigned NodeIndex1 = 0; NodeIndex1 < NodeCount; ++NodeIndex1)
		{
		if (!IsLeaf(NodeIndex1))
			continue;
		vector<unsigned> Path1;
		GetPathToRoot(NodeIndex1, Path1);
		const unsigned L1 = SIZE(Path1);
		const string &Label1 = GetLabel(NodeIndex1);
		for (unsigned NodeIndex2 = NodeIndex1+1; NodeIndex2 < NodeCount; ++NodeIndex2)
			{
			if (!IsLeaf(NodeIndex2))
				continue;
			vector<unsigned> Path2;
			GetPathToRoot(NodeIndex2, Path2);
			const unsigned L2 = SIZE(Path2);
			const string &Label2 = GetLabel(NodeIndex2);
			unsigned MRCA = UINT_MAX;
			for (unsigned i = 0; i < L1; ++i)
				for (unsigned j = 0; j < L2; ++j)
					if (Path1[i] == Path2[j])
						{
						MRCA = Path1[i];
						goto Done;
						}
			asserta(false);
		Done:
			string LabelMRCA;
			::GetLabel2(*this, MRCA, LabelMRCA);
			Log("./ta2 %s.%s %s.%s %s.%s\n",
			  LabelMRCA.c_str(), Label1.c_str(), 
			  LabelMRCA.c_str(), Label2.c_str(),
			  Label1.c_str(), Label2.c_str());
			}
		}
	}

/***
              +-- Leaf1
         +----|
         |    +-- Leaf2
      +--|
      |  +------- Leaf3
------|
      |       +-- Leaf4
      +-------|
              +-- Leaf5

One line per node.
***/
void Tree::LogMePretty(bool WithPrefixOrder) const
	{
	if (WithPrefixOrder)
		LogPrefixOrder();

	const unsigned W = 3;
	const unsigned NodeCount = GetNodeCount();
	vector<string> Lines(NodeCount);
	vector<unsigned> Depths;
	GetNodeDepths(Depths);

	unsigned MaxDepth = 0;
	for (unsigned i = 0; i < NodeCount; ++i)
		if (Depths[i] > MaxDepth)
			MaxDepth = Depths[i];

	vector<unsigned> InfixOrder;
	GetInfixOrder(InfixOrder);

#if	TRACE
	LogMe();
	Log("Infix   Node  Depth  Label\n");
	Log("-----  -----  -----  -----\n");
	for (unsigned i = 0; i < NodeCount; ++i)
		{
		unsigned NodeIndex = InfixOrder[i];
		Log("%5u  %5u  %5u", i, NodeIndex, Depths[NodeIndex]);
		if (IsLeaf(NodeIndex))
			Log("  %s", GetLabel(NodeIndex).c_str());
		Log("\n");
		}
	Log("\n");
#endif

	vector<unsigned> LineIndex(NodeCount);
	for (unsigned i = 0; i < NodeCount; ++i)
		{
		unsigned NodeIndex = InfixOrder[i];
		string &Line = Lines[i];
		LineIndex[NodeIndex] = i;
		unsigned Depth = Depths[NodeIndex];
		if (IsLeaf(NodeIndex))
			Depth = MaxDepth;
		unsigned L = (Depth + 1)*W;
		Line.resize(L, ' ');
		if (IsRoot(NodeIndex))
			{
			for (unsigned k = 0; k < W; ++k)
				Line[k] = '-';
			}
		else
			{
			unsigned Parent = GetParent(NodeIndex);
			unsigned ParentDepth = Depths[Parent];
			asserta(ParentDepth < Depth);
			Line[(ParentDepth + 1)*W - 1] = '+';
			for (unsigned k = (ParentDepth+1)*W; k < L-1; ++k)
				Line[k] = '-';
			}
		}

	for (unsigned NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
		{
		if (IsLeaf(NodeIndex))
			continue;
		unsigned Left = GetLeft(NodeIndex);
		unsigned Right = GetRight(NodeIndex);
		unsigned LeftY = LineIndex[Left];
		unsigned RightY = LineIndex[Right];
		unsigned MinY = min(LeftY, RightY);
		unsigned MaxY = max(LeftY, RightY);
		unsigned Depth = Depths[NodeIndex] + 1;
		unsigned L = Depth*W;
		for (unsigned Y = MinY + 1; Y < MaxY; ++Y)
			Lines[Y][L-1] = '|';
		}

	Log("\n");
	for (unsigned i = 0; i < NodeCount; ++i)
		{
		unsigned NodeIndex = InfixOrder[i];
		Log("%s", Lines[i].c_str());
		const string &Label = GetLabel(NodeIndex);
		if (Label == "")
			Log("%u", NodeIndex);
		else
			Log("%s", Label.c_str());
		//Log("%s", GetLabel(NodeIndex).c_str());
		Log("\n");
		}
	}

void Tree::DeleteLeaf(unsigned NodeIndex)
	{
	asserta(IsLeaf(NodeIndex));

	vector<unsigned> Lefts;
	vector<unsigned> Rights;
	vector<double> BranchLengths;
	vector<unsigned> Users;
	vector<string> Labels;

	m_NameToNodeIndex.clear();
	unsigned Parent = GetParent(NodeIndex);
	const unsigned NodeCount = GetNodeCount();
	for (unsigned i = 0; i < NodeCount; ++i)
		{
		if (i == NodeIndex || i == Parent)
			continue;

		unsigned Left = m_Lefts[i];
		unsigned Right = m_Rights[i];
		float BranchLength = m_BranchLengths[i];
		unsigned User = m_Users[i];
		const string &Label = m_Labels[i];

		if (Left == Parent)
			{
			if (m_Lefts[Parent] == NodeIndex)
				Left = m_Rights[Parent];
			else
				{
				asserta(m_Rights[Parent] == NodeIndex);
				Left = m_Lefts[Parent];
				}
			}
		else if (Right == Parent)
			{
			if (m_Lefts[Parent] == NodeIndex)
				Right = m_Rights[Parent];
			else
				{
				asserta(m_Rights[Parent] == NodeIndex);
				Right = m_Lefts[Parent];
				}
			}

		if (Left != UINT_MAX && Left > NodeIndex)
			--Left;
		if (Right != UINT_MAX && Right > NodeIndex)
			--Right;

		if (Left != UINT_MAX && Left > Parent)
			--Left;
		if (Right != UINT_MAX && Right > Parent)
			--Right;

		m_NameToNodeIndex[Label] = SIZE(Lefts);
		Lefts.push_back(Left);
		Rights.push_back(Right);
		BranchLengths.push_back(BranchLength);
		Users.push_back(User);
		Labels.push_back(Label);
		}

	m_Lefts = Lefts;
	m_Rights = Rights;
	m_BranchLengths = BranchLengths;
	m_Users = Users;
	m_Labels = Labels;
#if	DEBUG
	Validate();
#endif
	}

void Tree::GetPathToRoot(unsigned NodeIndex, vector<unsigned> &Path) const
	{
	unsigned n = NodeIndex;
	Path.clear();
	for (;;)
		{
		if (IsRoot(n))
			return;
		n = GetParent(n);
		Path.push_back(n);
		}
	}
