#include "myutils.h"
#include "params.h"
#include "mx.h"

const char Alphabet[] = "ACEDGFIHKMLNQPSRTWVY";

// BLOSUM70 with built-in gap extension penalty of 3.221,
//  = center parameter 6.442.
static float BLOSUM70C[20][20] =
	{
//	          A           C           E           D           G           F           I           H           K           M           L           N           Q           P           S           R           T           W           V           Y   
	{   8.56000f,   6.14100f,   5.95800f,   5.44000f,   6.45800f,   5.19800f,   5.66900f,   5.53000f,   6.01200f,   5.86000f,   5.55800f,   5.57200f,   5.97200f,   6.09400f,   7.05700f,   5.64100f,   6.41600f,   4.99200f,   6.29400f,   5.39000f,  },  // A
	{   6.14100f,  10.78400f,   4.26200f,   4.56000f,   4.86800f,   5.28400f,   5.77800f,   4.54400f,   4.63600f,   5.56100f,   5.59900f,   4.82800f,   4.89600f,   4.79700f,   5.90900f,   4.54100f,   5.83600f,   5.15600f,   5.87900f,   5.06400f,  },  // C
	{   5.95800f,   4.26200f,   9.09200f,   7.18900f,   5.26100f,   4.59400f,   4.64200f,   6.38500f,   6.79100f,   5.28700f,   4.75000f,   6.24000f,   7.36400f,   5.75500f,   6.33100f,   6.30800f,   6.00500f,   4.66000f,   5.09000f,   5.13400f,  },  // E
	{   5.44000f,   4.56000f,   7.18900f,   9.48900f,   5.68100f,   4.55100f,   4.51400f,   5.77500f,   5.99400f,   4.69800f,   4.31100f,   7.12100f,   6.15500f,   5.45500f,   6.22200f,   5.50100f,   5.86000f,   4.05200f,   4.62500f,   4.61100f,  },  // D
	{   6.45800f,   4.86800f,   5.26100f,   5.68100f,   9.33900f,   4.69000f,   4.28600f,   5.19800f,   5.54700f,   4.86200f,   4.38800f,   6.15800f,   5.36700f,   5.13400f,   6.18200f,   5.17000f,   5.54300f,   4.91000f,   4.65800f,   4.59500f,  },  // G
	{   5.19800f,   5.28400f,   4.59400f,   4.55100f,   4.69000f,   9.56000f,   6.28800f,   5.74000f,   4.75300f,   6.35700f,   6.63300f,   4.71200f,   4.76500f,   4.48100f,   5.13500f,   4.84000f,   5.35800f,   6.84100f,   5.95500f,   7.88400f,  },  // F
	{   5.66900f,   5.77800f,   4.64200f,   4.51400f,   4.28600f,   6.28800f,   8.58000f,   4.60000f,   4.94500f,   7.04300f,   7.21000f,   4.66200f,   4.86200f,   4.81900f,   5.14400f,   4.81800f,   6.01800f,   5.11900f,   7.72200f,   5.73700f,  },  // I
	{   5.53000f,   4.54400f,   6.38500f,   5.77500f,   5.19800f,   5.74000f,   4.60000f,  10.31300f,   6.07400f,   5.40800f,   4.84200f,   6.67500f,   6.75700f,   5.28700f,   5.95700f,   6.32400f,   5.60400f,   5.29100f,   4.76000f,   7.36600f,  },  // H
	{   6.01200f,   4.63600f,   6.79100f,   5.99400f,   5.54700f,   4.75300f,   4.94500f,   6.07400f,   8.89700f,   5.67200f,   5.11000f,   6.35500f,   7.09800f,   5.84000f,   6.24100f,   7.55100f,   6.08100f,   4.69600f,   5.17900f,   5.33100f,  },  // K
	{   5.86000f,   5.56100f,   5.28700f,   4.69800f,   4.86200f,   6.35700f,   7.04300f,   5.40800f,   5.67200f,   9.36700f,   7.49900f,   5.21000f,   6.24600f,   5.09300f,   5.58700f,   5.56900f,   6.07300f,   5.50200f,   6.75200f,   5.78400f,  },  // M
	{   5.55800f,   5.59900f,   4.75000f,   4.31100f,   4.38800f,   6.63300f,   7.21000f,   4.84200f,   5.11000f,   7.49900f,   8.46700f,   4.55500f,   5.24100f,   4.84200f,   5.13700f,   5.10500f,   5.68800f,   5.44400f,   6.80400f,   5.82300f,  },  // L
	{   5.57200f,   4.82800f,   6.24000f,   7.12100f,   6.15800f,   4.71200f,   4.66200f,   6.67500f,   6.35500f,   5.21000f,   4.55500f,   9.45200f,   6.40900f,   5.20600f,   6.69000f,   6.13900f,   6.37500f,   4.50900f,   4.87200f,   5.21100f,  },  // N
	{   5.97200f,   4.89600f,   7.36400f,   6.15500f,   5.36700f,   4.76500f,   4.86200f,   6.75700f,   7.09800f,   6.24600f,   5.24100f,   6.40900f,   9.31800f,   5.66000f,   6.31200f,   6.96600f,   6.07200f,   5.34900f,   5.24900f,   5.49200f,  },  // Q
	{   6.09400f,   4.79700f,   5.75500f,   5.45500f,   5.13400f,   4.48100f,   4.81900f,   5.28700f,   5.84000f,   5.09300f,   4.84200f,   5.20600f,   5.66000f,  10.25800f,   5.96100f,   5.36400f,   5.74800f,   4.38600f,   5.16100f,   4.76400f,  },  // P
	{   7.05700f,   5.90900f,   6.33100f,   6.22200f,   6.18200f,   5.13500f,   5.14400f,   5.95700f,   6.24100f,   5.58700f,   5.13700f,   6.69000f,   6.31200f,   5.96100f,   8.59200f,   6.00700f,   7.15800f,   4.83900f,   5.49900f,   5.51700f,  },  // S
	{   5.64100f,   4.54100f,   6.30800f,   5.50100f,   5.17000f,   4.84000f,   4.81800f,   6.32400f,   7.55100f,   5.56900f,   5.10500f,   6.13900f,   6.96600f,   5.36400f,   6.00700f,   9.34200f,   5.80800f,   4.86700f,   5.09600f,   5.44500f,  },  // R
	{   6.41600f,   5.83600f,   6.00500f,   5.86000f,   5.54300f,   5.35800f,   6.01800f,   5.60400f,   6.08100f,   6.07300f,   5.68800f,   6.37500f,   6.07200f,   5.74800f,   7.15800f,   5.80800f,   8.90800f,   4.92900f,   6.35000f,   5.50200f,  },  // T
	{   4.99200f,   5.15600f,   4.66000f,   4.05200f,   4.91000f,   6.84100f,   5.11900f,   5.29100f,   4.69600f,   5.50200f,   5.44400f,   4.50900f,   5.34900f,   4.38600f,   4.83900f,   4.86700f,   4.92900f,  11.74200f,   5.02900f,   7.46400f,  },  // W
	{   6.29400f,   5.87900f,   5.09000f,   4.62500f,   4.65800f,   5.95500f,   7.72200f,   4.76000f,   5.17900f,   6.75200f,   6.80400f,   4.87200f,   5.24900f,   5.16100f,   5.49900f,   5.09600f,   6.35000f,   5.02900f,   8.48900f,   5.66700f,  },  // V
	{   5.39000f,   5.06400f,   5.13400f,   4.61100f,   4.59500f,   7.88400f,   5.73700f,   7.36600f,   5.33100f,   5.78400f,   5.82300f,   5.21100f,   5.49200f,   4.76400f,   5.51700f,   5.44500f,   5.50200f,   7.46400f,   5.66700f,   9.89900f,  },  // Y
	};

void SetBLOSUM70C()
	{
	unsigned N = unsigned(strlen(Alphabet));
	Mx<float> &SubstMxf = GetSubstMxf();
	if (SubstMxf.m_Name == "BLOSUM70C")
		return;

	SubstMxf.Alloc("BLOSUM70C", 256, 256);
	SubstMxf.m_Alpha = string(Alphabet);
	SubstMxf.Init(0);
	float **Mx = SubstMxf.GetData();
	for (unsigned i = 0; i < N; ++i)
		{
		for (unsigned j = 0; j < N; ++j)
			{
			float v = BLOSUM70C[i][j];
			
			byte ui = (byte) toupper(Alphabet[i]);
			byte uj = (byte) toupper(Alphabet[j]);
			byte li = (byte) tolower(ui);
			byte lj = (byte) tolower(uj);
			ui = (byte) toupper(ui);
			uj = (byte) toupper(uj);

			Mx[ui][uj] = v;
			Mx[uj][ui] = v;

			Mx[ui][lj] = v;
			Mx[uj][li] = v;

			Mx[li][uj] = v;
			Mx[lj][ui] = v;

			Mx[li][lj] = v;
			Mx[lj][li] = v;
			}
		}
	}
