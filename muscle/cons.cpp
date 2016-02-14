#include "myutils.h"
#include "seqdb.h"
#include "mx.h"
#include "sparsemx.h"

#define TRACE		0

static void UpdateTriplet(SparseMx &XZ, SparseMx &ZY, float w, float **NewMx)
	{
#if	TRACE
	Log("UpdateTriplet(XZ=%s-%s, ZY=%s-%s)\n",
	  XZ.GetRowLabel(), XZ.GetColLabel(),
	  ZY.GetRowLabel(), ZY.GetColLabel());
#endif
#if	DEBUG
	{
	assert(XZ.GetColLabel() == ZY.GetRowLabel());
	assert(XZ.GetRowLabel() != ZY.GetColLabel());
	assert(XZ.m_ColCount == ZY.m_RowCount);
	}
#endif
	const unsigned LX = XZ.m_RowCount - 1;

	for (unsigned PosX = 1; PosX <= LX; ++PosX)
		{
		float *ValuesXZ;
		unsigned *ColIndexesXZ;
		unsigned EntryCountXZ = XZ.GetRow(PosX, &ValuesXZ, &ColIndexesXZ);
		for (unsigned EntryIndexXZ = 0; EntryIndexXZ < EntryCountXZ; ++EntryIndexXZ)
			{
			float ProbXZ = ValuesXZ[EntryIndexXZ];
			unsigned PosZ = ColIndexesXZ[EntryIndexXZ];

			float *ValuesZY;
			unsigned *ColIndexesZY;
			unsigned EntryCountZY = ZY.GetRow2(PosZ, &ValuesZY, &ColIndexesZY);
			for (unsigned EntryIndexZY = 0; EntryIndexZY < EntryCountZY; ++EntryIndexZY)
				{
				float ProbZY = ValuesZY[EntryIndexZY];
				unsigned PosY = ColIndexesZY[EntryIndexZY];

				NewMx[PosX][PosY] += w*ProbXZ*ProbZY;
				}
			}
		}
	}

// First matrix is transposed
static void UpdateTripletT(SparseMx &ZX, SparseMx &ZY, float w, float **NewMx)
	{
#if	TRACE
	Log("UpdateTripletT(ZX=%s-%s, ZY=%s-%s)\n",
	  ZX.GetRowLabel(), ZX.GetColLabel(),
	  ZY.GetRowLabel(), ZY.GetColLabel());
	Log("ZX=\n");
	ZX.LogMe();
	Log("ZY=\n");
	ZY.LogMe();
	Log("\n");
#endif

	assert(ZX.GetRowLabel() == ZY.GetRowLabel());
	assert(ZX.GetColLabel() != ZY.GetColLabel());
	assert(ZY.m_RowCount == ZX.m_RowCount);

	const unsigned LZ = ZX.m_RowCount - 1;

	for (unsigned PosZ = 1; PosZ <= LZ; ++PosZ)
		{
		float *ValuesZX;
		unsigned *ColIndexesZX;
		unsigned EntryCountZX = ZX.GetRow(PosZ, &ValuesZX, &ColIndexesZX);
		for (unsigned EntryIndexZX = 0; EntryIndexZX < EntryCountZX; ++EntryIndexZX)
			{
			float ProbZX = ValuesZX[EntryIndexZX];
			unsigned PosX = ColIndexesZX[EntryIndexZX];

			float *ValuesZY;
			unsigned *ColIndexesZY;
			unsigned EntryCountZY = ZY.GetRow2(PosZ, &ValuesZY, &ColIndexesZY);
			for (unsigned EntryIndexZY = 0; EntryIndexZY < EntryCountZY; ++EntryIndexZY)
				{
				float ProbZY = ValuesZY[EntryIndexZY];
				unsigned PosY = ColIndexesZY[EntryIndexZY];

				NewMx[PosX][PosY] += w*ProbZX*ProbZY;
				}
			}
		}
	}

// Second matrix is transposed
static void UpdateTriplet_T(SparseMx &XZ, SparseMx &YZ, float w, float **NewMx)
	{
#if	TRACE
	Log("UpdateTriplet_T(XZ=%s-%s, YZ=%s-%s)\n",
	  XZ.GetRowLabel(), XZ.GetColLabel(),
	  YZ.GetRowLabel(), YZ.GetColLabel());
	Log("XZ=\n");
	XZ.LogMe();
	Log("YZ=\n");
	YZ.LogMe();
	Log("\n");
#endif

	assert(XZ.GetColLabel() == YZ.GetColLabel());
	assert(XZ.GetRowLabel() != YZ.GetRowLabel());

	YZ.ComputeCols();

	const unsigned LX = XZ.m_RowCount - 1;

	for (unsigned PosX = 1; PosX <= LX; ++PosX)
		{
		float *ValuesXZ;
		unsigned *ColIndexesXZ;
		unsigned EntryCountXZ = XZ.GetRow(PosX, &ValuesXZ, &ColIndexesXZ);
		for (unsigned EntryIndexXZ = 0; EntryIndexXZ < EntryCountXZ; ++EntryIndexXZ)
			{
			float ProbXZ = ValuesXZ[EntryIndexXZ];
			unsigned PosZ = ColIndexesXZ[EntryIndexXZ];

			assert(PosZ < SIZE(YZ.m_Cols));
			const vector<pair<uint16, float> > &ColYZ = YZ.m_Cols[PosZ];
			const unsigned EntryCountYZ = SIZE(ColYZ);
#if	TRACE
			{
			Log("ColYZ[PosZ=%u]: ", PosZ);
			for (unsigned EntryIndexYZ = 0; EntryIndexYZ < EntryCountYZ; ++EntryIndexYZ)
				{
				const pair<uint16, float> &e2 = ColYZ[EntryIndexYZ];
				unsigned PosY = e2.first;
				float ProbYZ = e2.second;
				Log(" PosY=%u ProbYZ=%.4f", PosY, ProbYZ);
				}
			Log("\n");
			}
#endif
			for (unsigned EntryIndexYZ = 0; EntryIndexYZ < EntryCountYZ; ++EntryIndexYZ)
				{
				const pair<uint16, float> &e2 = ColYZ[EntryIndexYZ];
				unsigned PosY = e2.first;
				float ProbYZ = e2.second;

				NewMx[PosX][PosY] += w*ProbXZ*ProbYZ;
				}
			}
		}

	YZ.FreeCols();
	}

void SeqDB::Cons(unsigned Iter, unsigned Iters)
	{
	const unsigned SeqCount = GetSeqCount();
	const unsigned PairCount = GetPairCount();

	if (m_Weights.empty())
		m_Weights.resize(SeqCount, 1.0f);

#if	TRACE
	Log("\n");
	Log("Posteriors before:\n");
	for (unsigned i = 0; i < PairCount; ++i)
		m_SPPs[i]->LogMe();
#endif

	vector<SparseMx *> NewSPPs(PairCount);
	for (unsigned i = 0; i < PairCount; ++i)
		{
		NewSPPs[i] = new SparseMx;
		if (NewSPPs[i] == 0)
			Die("Out of memory");
		}

	unsigned Counter = 0;
	Mx<float> NewMxf;
	for (unsigned SeqIndexX = 0; SeqIndexX < SeqCount; ++SeqIndexX)
		{
		const unsigned LX = GetSeqLength(SeqIndexX);
		const float wX = m_Weights[SeqIndexX];
		for (unsigned SeqIndexY = SeqIndexX+1; SeqIndexY < SeqCount; ++SeqIndexY)
			{
			const unsigned LY = GetSeqLength(SeqIndexY);
			float wY = m_Weights[SeqIndexY];

			unsigned PairIndex = GetPairIndex(SeqIndexX, SeqIndexY);
			if (opt_cons > 1)
				ProgressStep(Counter++, PairCount, "Consistency %u / %u",
				  Iter+1, Iters);
			else
				ProgressStep(Counter++, PairCount, "Consistency");

			bool Transpose;
			SparseMx &XY = GetSPP(SeqIndexX, SeqIndexY, Transpose);
			assert(XY.GetRowLabel() == GetLabel(SeqIndexX));
			assert(XY.GetColLabel() == GetLabel(SeqIndexY));
			asserta(!Transpose);

			NewMxf.Alloc("NewMxf", LX+1, LY+1, this, SeqIndexX, SeqIndexY);
			NewMxf.Init(0);
			float **NewMx = NewMxf.GetData();

		// Add contributions from special case triplets XXY and XYY
			const float w = (wX*wX*wY + wX*wY*wY);
			float Summ_Weights = w;
			for (unsigned i = 0; i <= LX; ++i)
				{
				float *Values;
				unsigned *ColIndexes;
				unsigned EntryCount = XY.GetRow(i, &Values, &ColIndexes);
				for (unsigned e = 0; e < EntryCount; ++e)
					{
					unsigned ColIndex = ColIndexes[e];
					assert(i < NewMxf.m_RowCount);
					if (ColIndex >= NewMxf.m_ColCount)
						Die("ColIndex=%u ColCount=%u", ColIndex, NewMxf.m_ColCount);
					float Value = Values[e];
					NewMx[i][ColIndex] = w*Value;
					}
				}

			for (unsigned SeqIndexZ = 0; SeqIndexZ < SeqCount; ++SeqIndexZ)
				{
				if (SeqIndexZ == SeqIndexX || SeqIndexZ == SeqIndexY)
					continue;

				const float wZ = m_Weights[SeqIndexZ];
				bool TransposeXZ;
				bool TransposeZY;
				SparseMx &XZ = GetSPP(SeqIndexX, SeqIndexZ, TransposeXZ);
				SparseMx &ZY = GetSPP(SeqIndexZ, SeqIndexY, TransposeZY);
#if	DEBUG
				{
				unsigned LZ = GetSeqLength(SeqIndexZ);
				if (TransposeXZ)
					{
					asserta(LZ+1 == XZ.m_RowCount);
					asserta(LX+1 == XZ.m_ColCount);
					}
				else
					{
					asserta(LX+1 == XZ.m_RowCount);
					asserta(LZ+1 == XZ.m_ColCount);
					}
				if (TransposeZY)
					{
					asserta(LY+1 == ZY.m_RowCount);
					asserta(LZ+1 == ZY.m_ColCount);
					}
				else
					{
					asserta(LZ+1 == ZY.m_RowCount);
					asserta(LY+1 == ZY.m_ColCount);
					}
				}
#endif
				const float w = wX*wY*wZ;
				Summ_Weights += w;
				if (!TransposeXZ && !TransposeZY)
					UpdateTriplet(XZ, ZY, w, NewMx);
				else if (TransposeXZ && !TransposeZY)
					UpdateTripletT(XZ, ZY, w, NewMx);
				else if (!TransposeXZ && TransposeZY)
					UpdateTriplet_T(XZ, ZY, w, NewMx);
				else
					asserta(false);
				}

		// Normalize -- do this even if not in-place to avoid
		// exploding size of sparse matrix.
			for (unsigned PosX = 1; PosX <= LX; ++PosX)
				for (unsigned PosY = 1; PosY <= LY; ++PosY)
					{
					float NewP = NewMx[PosX][PosY] / Summ_Weights;
#if	TRACE
					Log("\n");
					Log("NewMx[%u][%u]=%g / Summ_Weights=%g = %g\n",
					  PosX, PosY, NewMx[PosX][PosY], Summ_Weights, NewP);
#endif
					if (NewP < 0.0 || NewP > 1.1)
						{
						static bool Done = false;
						if (!Done)
							{
							Warning("NewP=%g", NewP);
							Done = true;
							}
						NewP = 0.0;
						}
					NewMx[PosX][PosY] = NewP;
					}
#if	TRACE
			Log("\n");
			Log("NewSPPs[%s-%s].FromMx\n",
			  XY.GetRowLabel(), XY.GetColLabel());
#endif
			NewSPPs[PairIndex]->FromMx(NewMx, LX+1, LY+1,
			  float(opt_minsparseprob), this, SeqIndexX, SeqIndexY);
			}
		}
	NewMxf.Clear();

	for (unsigned i = 0; i < PairCount; ++i)
		{
		delete m_SPPs[i];
		m_SPPs[i] = NewSPPs[i];
		}

#if	TRACE
	Log("\n");
	Log("Posteriors after:\n");
	for (unsigned i = 0; i < PairCount; ++i)
		m_SPPs[i]->LogMe();
#endif
	}
