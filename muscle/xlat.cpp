#include <ctype.h>
#include "myutils.h"
#include "seqdb.h"

void SetSimMx(SeqDB &DB, unsigned IdA, unsigned IdB);
void FwdBwdLocal(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &PPMx);
byte CodonToAA(const byte *DNA);
void MultiSWX(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &PPMx,
  float t, float e, vector<unsigned> &Startis, vector<unsigned> &Startjs,
  vector<string> &Paths);
void LogLocalAln(SeqDB &DB, unsigned IdA, unsigned IdB,
  unsigned Starti, unsigned Startj, const string &Path);
void GetLetterCounts(const string &Path, unsigned &i, unsigned &j);
void LogLocalAlnX(SeqDB &DB, unsigned SeqIndex1, unsigned SeqIndex2,
  unsigned Starti, unsigned Startj, const string &Path);

const float ONE_THIRD = 1.0f/3.0f;

static unsigned XlatSeq(const byte *Seq, unsigned L, unsigned Frame,
  byte *SeqX)
	{
	unsigned i = 0;
	for (unsigned Pos = Frame; Pos + 2 < L; Pos += 3)
		SeqX[i++] = CodonToAA(Seq + Pos);
	return i;
	}

static void XlatDB(SeqDB &DB, SeqDB &DBX)
	{
	DBX.Clear();
	const unsigned SeqCount = DB.GetSeqCount();
	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{
		const byte *Seq = DB.GetSeq(SeqIndex);
		unsigned L = DB.GetSeqLength(SeqIndex);
		const string &Label = DB.GetLabel(SeqIndex);

		for (unsigned Frame = 0; Frame < 3; ++Frame)
			{
			byte *SeqX = myalloc<byte>(3*L);
			unsigned LX = XlatSeq(Seq, L, Frame, SeqX);

			string LabelX = Label + "." + char('1' + Frame);
			DBX.AppendSeq(LabelX, SeqX, LX);
			}
		}
	}

static void LogXlatSeq(const byte *Seq, const byte *SeqX, unsigned L,
  unsigned LX, const string &Label, unsigned Frame, unsigned nsl)
	{
	Log("%*.*s  %5u  ", nsl, nsl, Label.c_str(), Frame);

	for (unsigned i = 0; i < Frame; ++i)
		Log("%c", Seq[i]);

	for (unsigned i = Frame; i < L; )
		{
		if (i > Frame || Frame > 0)
			Log(" ");
		char c1 = Seq[i++];
		char c2 = i < L ? Seq[i++] : ' ';
		char c3 = i < L ? Seq[i++] : ' ';
		Log("%c%c%c", c1, c2, c3);
		}
	Log("\n");
	Log("%*.*s  %5.5s  ", nsl, nsl, "", "");

	for (unsigned i = 0; i < Frame; ++i)
		Log(" ");
	if (Frame > 0)
		Log(" ");
	for (unsigned i = 0; i < LX; ++i)
		Log("  %c ", SeqX[i]);
	Log("\n");
	Log("\n");
	}

void LogXlatDB(SeqDB &DB, SeqDB &DBX)
	{
	const unsigned SeqCount = DB.GetSeqCount();
	unsigned nsl = DB.GetMaxShortLabelLength();
	if (nsl < 5)
		nsl = 5;

	Log("\n");
	Log("%*.*s  Frame  Sequence\n", nsl, nsl, "Label");
	for (unsigned i = 0; i < nsl; ++i)
		Log("-");
	Log("  -----  --------\n");
	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{
		const byte *Seq = DB.GetSeq(SeqIndex);
		unsigned L = DB.GetSeqLength(SeqIndex);
		string Label;
		DB.GetShortLabel(SeqIndex, Label);

		for (unsigned Frame = 0; Frame < 3; ++Frame)
			{
			unsigned SeqIndexX = 3*SeqIndex + Frame;
			const byte *SeqX = DBX.GetSeq(SeqIndexX);
			unsigned LX = DBX.GetSeqLength(SeqIndexX);
			LogXlatSeq(Seq, SeqX, L, LX, Label, Frame, nsl);
			}
		}
	}

void FwdBwdXlat(SeqDB &DB, unsigned IdA, unsigned IdB, Mx<float> &PPMx)
	{
	SeqDB DBPair;

	byte *A = DB.GetSeq(IdA);
	byte *B = DB.GetSeq(IdB);

	const string &LabelA = DB.GetLabel(IdA);
	const string &LabelB = DB.GetLabel(IdB);

	const unsigned LA = DB.GetSeqLength(IdA);
	const unsigned LB = DB.GetSeqLength(IdB);

	float WeightA = DB.GetSeqWeight(IdA);
	float WeightB = DB.GetSeqWeight(IdB);

	unsigned UserA = DB.GetUser(IdA);
	unsigned UserB = DB.GetUser(IdB);

	DBPair.AddSeq(LabelA, A, LA, WeightA, UserA);
	DBPair.AddSeq(LabelB, B, LB, WeightB, UserB);

	SeqDB DBX;
	XlatDB(DBPair, DBX);

	PPMx.Alloc("PP", LA+1, LB+1, &DB, IdA, IdB);
	PPMx.Init(0);
	float **PP = PPMx.GetData();
	for (unsigned Frame1 = 0; Frame1 < 3; ++Frame1)
		{
		unsigned IdAX = Frame1;
		unsigned LAX = DBX.GetSeqLength(IdAX);
		for (unsigned Frame2 = 0; Frame2 < 3; ++Frame2)
			{
			unsigned IdBX = 3 + Frame2;
			unsigned LBX = DBX.GetSeqLength(IdBX);

			Mx<float> PPMxX;
			FwdBwdLocal(DBX, IdAX, IdBX, PPMxX);
			float **PPX = PPMxX.GetData();

			for (unsigned iX = 0; iX < LAX; ++iX)
				{
				unsigned i = 3*iX + Frame1 + 3;
				assert(i <= LA);
				for (unsigned jX = 0; jX < LBX; ++jX)
					{
					unsigned j = 3*jX + Frame2 + 3;
					assert(j <= LB);
					PP[i][j] = PPX[iX+1][jX+1];
					}
				}
			}
		}
	//SparseMx SPPMx;
	//SPPMx.FromMx(PPMx);
	//SPPMx.LogSmallDotPlot();
	}

void FrameShift(SeqDB &DB)
	{
	opt_xlat = true;
	DB.ComputeSPPs();

	for (unsigned i = 0; i < opt_cons; ++i)
		DB.Cons(i, opt_cons);

	const unsigned SeqCount = DB.GetSeqCount();
	unsigned nsl = DB.GetMaxShortLabelLength();
	if (nsl < 4)
		nsl = 4;
	
	vector<unsigned> UnshiftedCount(SeqCount);
	vector<unsigned> ShiftedCount(SeqCount);
	vector<unsigned> Frame1Count(SeqCount);
	vector<unsigned> Frame2Count(SeqCount);
	unsigned HitCount = 0;

	unsigned PairCount = DB.GetPairCount();
	unsigned PairIndex = 0;

	vector<unsigned> SSeqIndexes1;
	vector<unsigned> SSeqIndexes2;
	vector<unsigned> SStartis;
	vector<unsigned> SStartjs;
	vector<string> SPaths;

	for (unsigned SeqIndex1 = 0; SeqIndex1 < SeqCount; ++SeqIndex1)
		{
		string Label1;
		DB.GetShortLabel(SeqIndex1, Label1);

		for (unsigned SeqIndex2 = SeqIndex1 + 1; SeqIndex2 < SeqCount; ++SeqIndex2)
			{
			string Label2;
			DB.GetShortLabel(SeqIndex2, Label2);

			ProgressStep(PairIndex++, PairCount, "Find frameshifts (%.16s,%.16s)",
			  Label1.c_str(), Label2.c_str());

			bool Transpose;
			SparseMx &SPP = DB.GetSPP(SeqIndex1, SeqIndex2, Transpose);
			Mx<float> PPMx;
			SPP.ToMxf(PPMx);

			const float t = opt_minlocalprob;
			const float e = -opt_gaplocal;
			vector<unsigned> Startis;
			vector<unsigned> Startjs;
			vector<string> Paths;
			MultiSWX(DB, SeqIndex1, SeqIndex2, PPMx, t, e, Startis, Startjs, Paths);

			const unsigned PathCount = SIZE(Paths);
			for (unsigned PathIndex = 0; PathIndex < PathCount; ++PathIndex)
				{
				unsigned Starti = Startis[PathIndex];
				unsigned Startj = Startjs[PathIndex];
				unsigned Frame1 = Starti%3;
				unsigned Frame2 = Startj%3;
				if (Frame1 != 0 && Frame2 != 0 && !opt_allframes)
					continue;
				if (HitCount == 0)
					{
					Log("%*.*s", nsl, nsl, "Seq1");  
					Log("  %*.*s", nsl, nsl, "Seq2");
					Log("  Start1  Start2    AAs1    AAs2  Frame1  Frame2");
					Log("\n");

					for (unsigned i = 0; i < nsl; ++i)
						Log("-");
					Log("  ");
					for (unsigned i = 0; i < nsl; ++i)
						Log("-");
					for (unsigned i = 0; i < 6; ++i)
						Log("  ------");
					Log("\n");
					}
				const string &Path = Paths[PathIndex];
				unsigned Ni;
				unsigned Nj;
				GetLetterCounts(Path, Ni, Nj);
				bool Shift = (Frame1 != 0 || Frame2 != 0);
				if (Frame1 != 0)
					{
					ShiftedCount[SeqIndex1] += Ni;
					if (Frame1 == 1)
						Frame1Count[SeqIndex1] += Ni;
					else if (Frame1 == 2)
						Frame2Count[SeqIndex1] += Ni;
					else
						asserta(false);
					}
				if (Frame2 != 0)
					{
					ShiftedCount[SeqIndex2] += Nj;
					if (Frame2 == 1)
						Frame1Count[SeqIndex2] += Nj;
					else if (Frame2 == 2)
						Frame2Count[SeqIndex2] += Nj;
					else
						asserta(false);
					}
				if (Frame1 == 0 && Frame2 == 0)
					{
					UnshiftedCount[SeqIndex1] += Ni;
					UnshiftedCount[SeqIndex2] += Nj;
					}
				if (Frame1 != 0)
					{
					Log("%*.*s  %*.*s", nsl, nsl, Label1.c_str(), nsl, nsl, Label2.c_str());
					Log("  %6u  %6u  %6u  %6u  %6u  %6u",
					  Starti, Startj, Ni, Nj, Starti%3, Startj%3);
					}
				else
					{
					Log("%*.*s  %*.*s", nsl, nsl, Label2.c_str(), nsl, nsl, Label1.c_str());
					Log("  %6u  %6u  %6u  %6u  %6u  %6u",
					  Startj, Starti, Ni, Nj, Startj%3, Starti%3);
					}
				if (Shift)
					Log(" <<");
				Log("\n");
				++HitCount;
				if (Shift || opt_alllocals)
					{
					SSeqIndexes1.push_back(SeqIndex1);
					SSeqIndexes2.push_back(SeqIndex2);
					SStartis.push_back(Starti);
					SStartjs.push_back(Startj);
					SPaths.push_back(Path);
					}
				}
			}
		}

	if (HitCount == 0)
		{
		Log("No translated local alignments found, no frameshifts detected.\n");
		return;
		}

	vector<unsigned> ShiftedSeqIndexes;
	vector<unsigned> Shifts;
	Log("\n");
	Log("Total AAs in translated local alignments:\n");
	Log("  Seq  Shifted0  Shifted1  Shifted2  Y  Label\n");
	Log("-----  --------  --------  --------  -  -----\n");
	for (unsigned SeqIndex = 0; SeqIndex < SeqCount; ++SeqIndex)
		{
		unsigned U = UnshiftedCount[SeqIndex];
		unsigned S = ShiftedCount[SeqIndex];
		bool Shifted = (U > 0 && S > 0 && float(S)/float(U) >= opt_frameskew
		  && S + U >= opt_framemin);
		Log("%5u  %8u  %8u  %8u  %c  %s\n",
		  SeqIndex, U, Frame1Count[SeqIndex], Frame2Count[SeqIndex],
		  yon(Shifted), DB.GetLabel(SeqIndex).c_str());

		if (Shifted)
			{
			ShiftedSeqIndexes.push_back(SeqIndex);
			if (Frame1Count[SeqIndex] > Frame2Count[SeqIndex])
				Shifts.push_back(1);
			else
				Shifts.push_back(2);
			}
		}

	if (opt_alllocals)
		{
		bool Any = false;
		for (unsigned SPathIndex = 0; SPathIndex < SIZE(SPaths); ++SPathIndex)
			{
			unsigned SeqIndex1 = SSeqIndexes1[SPathIndex];
			unsigned SeqIndex2 = SSeqIndexes2[SPathIndex];
			if (!Any)
				{
				Any = true;
				Log("\n");
				Log("Local alignments:\n");
				}

			unsigned Starti = SStartis[SPathIndex];
			unsigned Startj = SStartjs[SPathIndex];
			const string &Path = SPaths[SPathIndex];
			LogLocalAlnX(DB, SeqIndex1, SeqIndex2, Starti, Startj, Path);
			}
		}

	if (!ShiftedSeqIndexes.empty() && !opt_alllocals)
		{
		Log("\n");
		Log("Shifted alignments:\n");
		for (unsigned i = 0; i < SIZE(ShiftedSeqIndexes); ++i)
			{
			unsigned SeqIndex = ShiftedSeqIndexes[i];
			for (unsigned SPathIndex = 0; SPathIndex < SIZE(SPaths); ++SPathIndex)
				{
				unsigned SeqIndex1 = SSeqIndexes1[SPathIndex];
				unsigned SeqIndex2 = SSeqIndexes2[SPathIndex];
				unsigned Starti = SStartis[SPathIndex];
				unsigned Startj = SStartjs[SPathIndex];
				const string &Path = SPaths[SPathIndex];

				if (SeqIndex1 == SeqIndex || SeqIndex2 == SeqIndex)
					LogLocalAlnX(DB, SeqIndex1, SeqIndex2, Starti, Startj, Path);
				}
			}
		}

	if (!ShiftedSeqIndexes.empty())
		{
		Log("\n");
		for (unsigned i = 0; i < SIZE(ShiftedSeqIndexes); ++i)
			{
			unsigned SeqIndex = ShiftedSeqIndexes[i];
			Log("Frameshift(%u) %s\n",
			  Shifts[i],
			  DB.GetLabel(SeqIndex).c_str());
			}
		}
	}
