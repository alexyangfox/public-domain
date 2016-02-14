#ifndef tree_h
#define tree_h

#include "myutils.h"
#include <map>
#include <list>

using namespace std;

struct Tree;
typedef bool (*ptrTravFn)(const Tree &t, unsigned NodeIndex, void *UserData);

// Binary tree class
struct Tree
	{
private:
// An unrooted tree is represented internally as a rooted binary
// tree. The two edges L, R descending from the (fictitious) root
// should be considered as a single edge with a length that is
// length(L) + length(R).
	bool m_Rooted;
	unsigned m_RootNodeIndex;
	unsigned m_NextNodeIndex;

// For internal node i, m_Lefts[i] and m_Rights[i] contain node
// indexes of its children. For leaf node i, m_Lefts[i] is UINT_MAX.
	vector<unsigned> m_Lefts;
	vector<unsigned> m_Rights;
	vector<double> m_BranchLengths;
	vector<unsigned> m_Users;
	vector<string> m_Labels;
	map<string, unsigned> m_NameToNodeIndex;
	mutable list<unsigned> m_TraverseStack;

public:
	void Clear();
	void Init(const vector<string> &LeafNames);
	unsigned Join(unsigned Node1, double BranchLength1, unsigned Node2,
	  double BranchLength2, const string &Name = "");
	void Attach(const Tree &t, const string &Label);
	bool IsRooted() const { return m_Rooted; }
	bool Empty() const { return GetNodeCount() == 0; }

	unsigned GetNodeCount() const
		{
		return SIZE(m_Lefts);
		}

	bool IsRoot(unsigned NodeIndex) const
		{
		return m_Rooted && NodeIndex == m_RootNodeIndex;
		}

	unsigned GetLeafCount() const
		{
		unsigned N = GetNodeCount();
		return (N + 1)/2;
		}

	unsigned GetInternalNodeCount() const
		{
		return GetNodeCount() - GetLeafCount();
		}

	unsigned GetRootNodeIndex() const
		{
		if (!m_Rooted)
			Die("GetRootNodeIndex: not rooted");
		return m_RootNodeIndex;
		}

	unsigned GetLeft(unsigned NodeIndex) const
		{
		assert(IsInternal(NodeIndex));
		return m_Lefts[NodeIndex];
		}

	unsigned GetRight(unsigned NodeIndex) const
		{
		assert(IsInternal(NodeIndex));
		return m_Rights[NodeIndex];
		}

	void SetUser(unsigned NodeIndex, unsigned User)
		{
		assert(NodeIndex < GetNodeCount());
		m_Users[NodeIndex] = User;
		}

	unsigned GetUser(unsigned NodeIndex) const
		{
		assert(NodeIndex < GetNodeCount());
		return m_Users[NodeIndex];
		}

	double GetBranchLength(unsigned NodeIndex) const
		{
		assert(NodeIndex < GetNodeCount());
		return m_BranchLengths[NodeIndex];
		}

	void GetLeafIndexes(unsigned NodeIndex, vector<unsigned> &LeafIndexes) const;
	void GetSubtreeNodes(unsigned Node, vector<unsigned> &Nodes) const;

	unsigned GetNodeIndex(const string &Name, bool NotFoundOK = false) const
		{
		map<string, unsigned>::const_iterator p = m_NameToNodeIndex.find(Name);
		if (p == m_NameToNodeIndex.end())
			{
			if (NotFoundOK)
				return UINT_MAX;
			Die("GetNodeIndex(%.32s), not found", Name.c_str());
			}
		return p->second;
		}

	void GetDepths(vector<double> &Depths) const;
	void GetNodeDepths(vector<unsigned> &Depths) const;
	void GetInfixOrder(vector<unsigned> &Order) const;
	void GetPrefixOrder(vector<unsigned> &Order) const;
	void GetPathToRoot(unsigned NodeIndex, vector<unsigned> &Path) const;

	const string &GetLabel(unsigned NodeIndex) const
		{
		assert(NodeIndex < GetNodeCount());
		return m_Labels[NodeIndex];
		}

	bool IsLeaf(unsigned NodeIndex) const
		{
		assert(NodeIndex < GetNodeCount());
		return m_Lefts[NodeIndex] == UINT_MAX;
		}

	bool IsInternal(unsigned NodeIndex) const
		{
		return !IsLeaf(NodeIndex);
		}

	void Traverse(ptrTravFn OnNode, void *UserData) const;

	void ToFile(const char *FileName) const;
	void ToFileRecurse(FILE *f, unsigned NodeIndex) const;
	void FromFile(const string &FileName);
	unsigned NodeFromNewickFile(FILE *f);
	void LogMe() const;
	void LogMePretty(bool WithPrefixOrder = false) const;
	void LogPrefixOrder() const;
	void LogNewick(unsigned NodeIndex = UINT_MAX) const;
	void Validate() const;
	unsigned GetFirstDepthFirstNodeIndex() const;
	unsigned GetNextDepthFirstNodeIndex() const;
	unsigned GetParent(unsigned NodeIndex, bool DieOnErr = true) const;
	void DeleteLeaf(unsigned NodeIndex);

private:
	void GetDepthsRecurse(unsigned NodeIndex, vector<double> &Depths) const;
	void GetNodeDepthsRecurse(unsigned NodeIndex, vector<unsigned> &Depths) const;
	void GetInfixOrderRecurse(unsigned NodeIndex, vector<unsigned> &Order) const;
	void GetPrefixOrderRecurse(unsigned NodeIndex, vector<unsigned> &Order) const;
	};

#endif // tree_h
