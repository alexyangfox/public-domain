#include "myutils.h"
#include "mx.h"
#include "sparsemx.h"
#include "seqdb.h"
#include "info.h"
#include "params.h"

void IterateLocalFB(const string &Description, vector<string> &Paths,
  vector<unsigned> &Startis, vector<unsigned> &Startjs);
void LogLocalAln(const byte *A, const byte *B, unsigned LoA, unsigned LoB,
  const string &Path, bool Inverted, bool Reversed, bool Nucleo);
void GetLetterCounts(const string &Path, unsigned &i, unsigned &j);

void LogLowComplexity(SeqDB &DB, unsigned IdA, unsigned IdB,
  const string &Path, unsigned StartA, unsigned StartB)
	{
	}

void ComputeLowComplexity(SeqDB &DB)
	{
	string Model;
	GetLocalModel(DB, Model);
	SetModel(Model);

	const unsigned SeqCount = DB.GetSeqCount();
	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{
		}
	}
