#include "myutils.h"
#include "seqdb.h"
#include "mx.h"
#include <algorithm>

float GetMinAcc(const vector<float> &Accs);
float ComputeMean(const vector<float> &v, float &StdDev);
void ComputeRowProbs(SeqDB &DB, const SeqDB &msa,
  vector<float> &RowProbs);

static vector<float> *g_Accs;
static bool LTAcc(unsigned i, unsigned j)
	{
// sort descending
	return (*g_Accs)[i] > (*g_Accs)[j];
	}

void LogAccs(SeqDB &DB)
	{
	const unsigned SeqCount = DB.GetSeqCount();
	if (SeqCount < 2)
		return;

	vector<float> Accs;
	vector<float> FractIds;
	float Acc;
	float FractId;
	DB.ComputeAccsAndIds(Accs, FractIds, Acc, FractId);

	vector<unsigned> SortOrder;
	for (unsigned i = 0; i < SeqCount; ++i)
		SortOrder.push_back(i);
	g_Accs = &Accs;
	sort(SortOrder.begin(), SortOrder.end(), LTAcc);

	float MinAcc = GetMinAcc(Accs);

	float StdDev;
	float AvgAcc = ComputeMean(Accs, StdDev);

	Log("\n");
	Log("Average accuracies:\n");
	Log("  Seq  Accuracy    Pct Id  Label\n");
	Log("-----  --------  --------  -----\n");
	for (unsigned i = 0; i < SeqCount; ++i)
		{
		unsigned SeqIndex = SortOrder[i];
		const string &Label = DB.GetLabel(SeqIndex);
		unsigned InputSeqIndex = DB.GetUser(SeqIndex);
		float Acc = Accs[SeqIndex];
		bool Outlier = (Acc < MinAcc);
		Log("%5u  %7.1f%%%c %7.1f%%  %s\n",
		  InputSeqIndex+1,
		  Acc*100,
		  (Outlier ? '*' : ' '),
		  FractIds[SeqIndex]*100.0,
		  Label.c_str());
		}
	Log("-----  --------  --------\n");
	Log("%5u  %7.1f%%  %7.1f%%  Acc std dev=%.1f%%, outlier acc < %.1f%%",
	  SeqCount, AvgAcc*100.0, FractId*100.0, StdDev*100.0, MinAcc*100.0);
	Log("\n");
	}
