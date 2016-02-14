#include "myutils.h"
#include "seqdb.h"
#include "mx.h"
#include "params.h"

float ComputeMean(const vector<float> &v, float &StdDev)
	{
	StdDev = 0.0f;
	const unsigned N = SIZE(v);
	if (N == 0)
		return 0.0f;

	float Sum = 0.0f;
	for (unsigned i = 0; i < N; ++i)
		Sum += v[i];
	float Mean = Sum/N;

	float Sum2 = 0.0f;
	for (unsigned i = 0; i < N; ++i)
		{
		float d = v[i] - Mean;
		Sum2 += d*d;
		}
	StdDev = sqrt(Sum2/N);
	return Mean;
	}

float GetMinAcc(const vector<float> &Accs)
	{
	float StdDev;
	float Mean = ComputeMean(Accs, StdDev);
	float MinAcc = (Mean - opt_prunedev*StdDev);
	if (MinAcc > opt_pruneacc)
		MinAcc = opt_pruneacc;
	return MinAcc;
	}

void Prune(SeqDB &Input)
	{
	const unsigned InputSeqCount = Input.GetSeqCount();

	vector<float> Accs;
	vector<float> FractIds;
	float Acc;
	float FractId;
	Input.ComputeAccsAndIds(Accs, FractIds, Acc, FractId);

	const vector<float> &AccVec = (opt_pruneid ? FractIds : Accs);
	float MinAcc = GetMinAcc(AccVec);

	SeqDB Pruned;
	SeqDB Outliers;
	for (unsigned SeqIndex = 0; SeqIndex < InputSeqCount; ++SeqIndex)
		{
		float Acc = AccVec[SeqIndex];
		bool Outlier = (Acc < MinAcc);
		if (Outlier)
			Outliers.AppendSeq(Input, SeqIndex);
		else
			Pruned.AppendSeq(Input, SeqIndex);
		}

	if (opt_outliers != "")
		Outliers.ToFasta(opt_outliers);

	const unsigned PrunedSeqCount = Pruned.GetSeqCount();
	if (PrunedSeqCount < InputSeqCount)
		{
		Input.Copy(Pruned);
		for (unsigned i = 0; i < PrunedSeqCount; ++i)
			Input.SetUser(i, i);
		}

	const unsigned OutlierSeqCount = Outliers.GetSeqCount();
	Log("\n");
	Log("Pruned outliers:\n");
	if (OutlierSeqCount == 0)
		Log("No outliers found.\n");
	else
		{
		Log("  Seq  Accuracy    Pct Id  Label\n");
		Log("-----  --------  --------  -----\n");
		float SumAcc = 0;
		float SumId = 0;
		for (unsigned i = 0; i < OutlierSeqCount; ++i)
			{
			unsigned InputSeqIndex = Outliers.GetUser(i);
			const string &Label = Outliers.GetLabel(i);
			float Acc = Accs[InputSeqIndex];
			float FractId = FractIds[InputSeqIndex];
			SumAcc += Acc;
			SumId += FractId;
			bool Outlier = (AccVec[InputSeqIndex] < MinAcc);
			Log("%5u  %7.1f%%%c %7.1f%%  %s\n",
			  InputSeqIndex+1,
			  Acc*100,
			  (Outlier ? '*' : ' '),
			  FractId*100,
			  Label.c_str());
			}
		Log("-----  --------  --------\n");
		Log("%5u  %7.1f%%  %7.1f%%\n",
		  OutlierSeqCount,
		  SumAcc*100.0/OutlierSeqCount,
		  SumId*100.0/OutlierSeqCount);
		}
	}
