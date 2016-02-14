#include "myutils.h"
#include "seqdb.h"
#include "mx.h"
#include "params.h"
#include "hit.h"

void AlignSeqPairLocal(SeqDB &DB, unsigned IdA, unsigned IdB, vector<HitData> &Hits);
void LogLocalAlnHit(SeqDB &DB, unsigned IdA, unsigned IdB, const HitData &Hit);
void AlignMSAPairLocal(SeqDB &msa1, SeqDB &msa2, vector<HitData> &Hits);
void LogLocalAlnAlnHit(SeqDB &DBA, SeqDB &DBB, const HitData &Hit);
void AlignMSAsGivenSubPath(const SeqDB &msa1, const SeqDB &msa2, 
  unsigned StartCol1, unsigned StartCol2, const string &Path, bool Strand,
  SeqDB &OutMSA);
void GetGoodSegments(const SeqDB &msa, float SwitchPenalty,
  vector<SeqDB *> &msas);

static FILE *g_fMAF;
static Tree *g_Tree;

extern SeqDB *g_Input;

// g_MSAs[i] is vector of MSAs at tree node i.
static vector<vector<SeqDB *> > g_MSAs;

// g_BlockIndexes[i] is vector of block indexes at tree node i.
static vector<vector<unsigned> > g_BlockIndexes;

// g_BlockIndexes1/2[i] are the blocks that were aligned
// to make block i.
static vector<unsigned> g_BlockIndexes1;
static vector<unsigned> g_BlockIndexes2;

static unsigned g_BlockIndex = 0;
static unsigned g_ProgressCounter = 0;

static void AddBlock(const Tree &t, unsigned NodeIndex, unsigned LeftSub,
  unsigned RightSub, SeqDB &msa)
	{
	msa.StripAllGapSeqs();
	if (msa.GetSeqCount() == 0)
		return;

	g_BlockIndexes[NodeIndex].push_back(g_BlockIndex);

	unsigned BlockIndex1 = UINT_MAX;
	unsigned BlockIndex2 = UINT_MAX;
	if (t.IsInternal(NodeIndex))
		{
		if (LeftSub != UINT_MAX)
			{
			unsigned LeftNodeIndex = t.GetLeft(NodeIndex);
			asserta(LeftSub < SIZE(g_BlockIndexes[LeftNodeIndex]));
			BlockIndex1 = g_BlockIndexes[LeftNodeIndex][LeftSub];
			}

		if (RightSub != UINT_MAX)
			{
			unsigned RightNodeIndex = t.GetRight(NodeIndex);
			asserta(RightSub < SIZE(g_BlockIndexes[RightNodeIndex]));
			BlockIndex2 = g_BlockIndexes[RightNodeIndex][RightSub];
			}
		}

	asserta(SIZE(g_BlockIndexes1) == g_BlockIndex);
	asserta(SIZE(g_BlockIndexes2) == g_BlockIndex);
	g_BlockIndexes1.push_back(BlockIndex1);
	g_BlockIndexes2.push_back(BlockIndex2);

	char Suffix[64];

	sprintf(Suffix, "%u.%u.%u",
	  BlockIndex1 == UINT_MAX ? 0 : BlockIndex1+1,
	  BlockIndex2 == UINT_MAX ? 0 : BlockIndex2+1,
	  g_BlockIndex+1);

	bool Root = t.IsRoot(NodeIndex);
	char Tmp[64];
	sprintf(Tmp, "Block:%s", Suffix);
	msa.m_Name = Tmp;

	g_MSAs[NodeIndex].push_back(&msa);

	if (msa.GetSeqCount() > 1)
		{
		if (Root || opt_allblocks)
			{
			msa.LogMe();
			msa.ToMAF(g_fMAF);
			fflush(g_fMAF);
			}
		}

	if (opt_blockprefix != "")
		{
		string FileName = opt_blockprefix + Suffix;
		msa.ToFasta(FileName);
		}

	++g_BlockIndex;
	}

static bool OnNode(const Tree &t, unsigned NodeIndex, void *UserData)
	{
	ProgressStep(g_ProgressCounter++, t.GetNodeCount(),
	  "Align %.16s", t.GetLabel(NodeIndex).c_str());
	SeqDB &Input = *(SeqDB *) UserData;

	if (t.IsLeaf(NodeIndex))
		{
		SeqDB &seq = *new SeqDB;
		unsigned SeqIndex = t.GetUser(NodeIndex);
		seq.FromSeq(Input, SeqIndex);
		AddBlock(t, NodeIndex, UINT_MAX, UINT_MAX, seq);
		return true;
		}

	unsigned Left = t.GetLeft(NodeIndex);
	unsigned Right = t.GetRight(NodeIndex);

	vector<SeqDB *> &LeftMSAs = g_MSAs[Left];
	vector<SeqDB *> &RightMSAs = g_MSAs[Right];

	const unsigned LeftMSACount = SIZE(LeftMSAs);
	const unsigned RightMSACount = SIZE(RightMSAs);

	vector<vector<BPData> > RightBPsVec(RightMSACount);

	for (unsigned i = 0; i < LeftMSACount; ++i)
		{
		vector<BPData> LeftBPs;
		SeqDB &LeftMSA = *(LeftMSAs[i]);
		const unsigned LeftColCount = LeftMSA.GetColCount();

		for (unsigned j = 0; j < RightMSACount; ++j)
			{
			SeqDB &RightMSA = *(RightMSAs[j]);

			vector<HitData> Hits;
			AlignMSAPairLocal(LeftMSA, RightMSA, Hits);
			AppendBPs(Hits, LeftBPs, true);
			AppendBPs(Hits, RightBPsVec[j], false);

			for (unsigned k = 0; k < SIZE(Hits); ++k)
				{
				const HitData &Hit = Hits[k];

				SeqDB &ParentMSA = *new SeqDB;
				AlignMSAsGivenSubPath(LeftMSA, RightMSA, Hit.LoA, Hit.LoB, 
				  Hit.Path, Hit.Strand, ParentMSA);

				if (ParentMSA.GetSeqCount() == 2)
					{
					vector<SeqDB *> msas;//@@
					GetGoodSegments(ParentMSA, opt_gbpen, msas);//@@
					for (unsigned MSAIndex = 0; MSAIndex < SIZE(msas); ++MSAIndex)
						AddBlock(t, NodeIndex, i, j, *msas[MSAIndex]);
					}
				else
					AddBlock(t, NodeIndex, i, j, ParentMSA);
				}
			}

		vector<SegData> UncoveredSegs;
		GetUncoveredSegs(LeftBPs, LeftColCount, UncoveredSegs);

		for (unsigned k = 0; k < SIZE(UncoveredSegs); ++k)
			{
			const SegData &Seg = UncoveredSegs[k];
			if (Seg.GetLength() < opt_minlocallen)
				continue;
			SeqDB &ParentMSA = *new SeqDB;
			ParentMSA.FromColRange(LeftMSA, Seg.Lo, Seg.Hi);

			AddBlock(t, NodeIndex, i, UINT_MAX, ParentMSA);
			}
		}

	for (unsigned i = 0; i < RightMSACount; ++i)
		{
		vector<SegData> UncoveredSegs;
		const SeqDB &RightMSA = (*RightMSAs[i]);
		const unsigned RightColCount = RightMSA.GetColCount();
		GetUncoveredSegs(RightBPsVec[i], RightColCount, UncoveredSegs);
		for (unsigned j = 0; j < SIZE(UncoveredSegs); ++j)
			{
			const SegData &Seg = UncoveredSegs[j];
			if (Seg.GetLength() < opt_minlocallen)
				continue;
			SeqDB &ParentMSA = *new SeqDB;
			ParentMSA.FromColRange(RightMSA, Seg.Lo, Seg.Hi);

			AddBlock(t, NodeIndex, UINT_MAX, i, ParentMSA);
			}
		}

	LeftMSAs.clear();
	RightMSAs.clear();
	void ValidateCov(const vector<vector<SeqDB *> > &MSAs);//@@
	ValidateCov(g_MSAs);//@@
	return true;
	}

void LogBlockParents(bool WithLabels)
	{
	const Tree &t = *g_Tree;
	Log("\n");
	Log("Block   Node  Parent1  Parent2\n");
	Log("-----  -----  -------  -------\n");
	vector<unsigned> NodeIndexes;
	t.GetPrefixOrder(NodeIndexes);
	for (unsigned n = 0; n < SIZE(NodeIndexes); ++n)
		{
		unsigned NodeIndex = NodeIndexes[n];
		if (t.IsLeaf(NodeIndex))
			continue;
		unsigned BlockCount = SIZE(g_BlockIndexes[NodeIndex]);
		for (unsigned i = 0; i < BlockCount; ++i)
			{
			unsigned BlockIndex = g_BlockIndexes[NodeIndex][i];
			unsigned Parent1 = g_BlockIndexes1[BlockIndex];
			unsigned Parent2 = g_BlockIndexes2[BlockIndex];

			vector<unsigned> Ls;
			vector<unsigned> Rs;
			unsigned Left = t.GetLeft(NodeIndex);
			unsigned Right = t.GetRight(NodeIndex);
			t.GetLeafIndexes(Left, Ls);
			t.GetLeafIndexes(Right, Rs);

			Log("%5u  %5u", BlockIndex+1, NodeIndex);
			if (Parent1 == UINT_MAX)
				Log("  %7.7s", "*");
			else
				Log("  %7u", Parent1+1);

			if (Parent2 == UINT_MAX)
				Log("  %7.7s", "*");
			else
				Log("  %7u", Parent2+1);

			if (WithLabels)
				{
				Log("  (");
				for (unsigned k = 0; k < SIZE(Ls); ++k)
					{
					if (k > 0)
						Log(" ");
					Log("%s", t.GetLabel(Ls[k]).c_str());
					}
				Log(", ");
				for (unsigned k = 0; k < SIZE(Rs); ++k)
					Log(" %s", t.GetLabel(Rs[k]).c_str());
				Log(")");
				}
			Log("\n");
			}
		}
	}

void AlignMAF(SeqDB &DB)
	{
	g_fMAF = CreateStdioFile(opt_maf);
	fprintf(g_fMAF, "## maf version=1\n");

	unsigned SeqCount = DB.GetSeqCount();

	Tree &t = DB.m_GuideTree;

	if (SeqCount == 2)
		{
		vector<string> LeafNames;
		LeafNames.push_back(DB.GetLabel(0));
		LeafNames.push_back(DB.GetLabel(1));
		t.Init(LeafNames);
		t.Join(0, 1.0, 1, 1.0, "root");
		t.SetUser(0, 0);
		t.SetUser(1, 0);
		asserta(t.GetNodeCount() == 3);
		asserta(t.IsLeaf(0));
		asserta(t.IsLeaf(1));
		asserta(t.GetRootNodeIndex() == 2);
		}
	else
		{
		if (opt_tree == "")
			{
			DB.ComputeGuideTree();
			t.LogMePretty();
			}
		else
			t.FromFile(opt_tree);
		}
	DB.BindTree(t);
	g_Tree = &t;

	const unsigned NodeCount = t.GetNodeCount();	
	g_MSAs.resize(NodeCount);
	g_BlockIndexes.resize(NodeCount);

	g_ProgressCounter = 0;
	t.Traverse(OnNode, &DB);
	LogBlockParents(false); //@@

	CloseStdioFile(g_fMAF);
	}
