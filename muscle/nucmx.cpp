#include "myutils.h"
#include "params.h"
#include "mx.h"

/***
Matrices & penalties taken from this paper;
I'm not sure where they came from originally,
no citation is given:

Frith, et al. NAR 36(18): 5863–5871, Table 1.
doi: 10.1093/nar/gkn579.

HoxD55						HoxD70
	A	C	G	T			A	C	G	T
A	91 -90 -25 -100			91	-114 -31 -123
C	-90 100 -100 -25		-114 100 -125 -31
G	-25 -100 100 -90		-31 -125 100 -114
T	-100 -25 -90 91			-123 -31 -114 91

For both matrices, open = 400, extend = 30.

According to Frith et al, HOXD55 open=400 ext=30
is default for UCSC genome browset, HOXD70
open=400 ext=30 is default for BLASTZ v7.
***/

static const char Alphabet[] = "ACGT";

static float HOXD55[4][4] =
	{
	//                A       C       G      T
    /* A */    {     91,    -90,    -25,   -100    },
    /* C */    {    -90,    100,   -100,    -25    },
    /* G */    {    -25,   -100,    100,    -90    },
    /* T */    {   -100,    -25,    -90,     91    },
	};

static float HOXD70[4][4] =
	{
	//                A        C       G      T
    /* A */    {     91,    -114,    -31,  -123    },
    /* C */    {   -114,     100,   -125,   -31    },
    /* G */    {    -31,    -125,    100,  -114    },
    /* T */    {   -123,     -31,   -114,    91    },
	};

// Log-odds matrix derived from emission probabilities
// in Defaults.h found in the RNA version of PROBCONS:
// http://probcons.stanford.edu/probconsRNA.tar.gz
// According to Tom this was trained on BRALIBASE v2.
static float PCRNA[4][4] =
	{
//               A           C           G           T
/*A*/ {    1.05925f,  -1.09421f,  -0.57889f,  -0.85310f },
/*C*/ {   -1.09421f,   0.99323f,  -0.91464f,  -0.42753f },
/*G*/ {   -0.57889f,  -0.91464f,   0.89821f,  -1.05244f },
/*T*/ {   -0.85310f,  -0.42753f,  -1.05244f,   0.94153f }
	};

static void SetUT(float **Mx)
	{
	byte T = (byte) 'T';
	byte U = (byte) 'U';
	byte u = (byte) 'u';
	for (unsigned i = 0; i < 255; ++i)
		{
		float v = Mx[i][T];

		Mx[i][U] = v;
		Mx[i][u] = v;
		Mx[U][i] = v;
		Mx[u][i] = v;
		}
	}

static void Set(const string &Name, const float Hox[4][4], float Div = 1.0f)
	{
	unsigned N = unsigned(strlen(Alphabet));
	Mx<float> &SubstMxf = GetSubstMxf();
	SubstMxf.Alloc(Name, 256, 256);
	SubstMxf.m_Alpha = string("ACGTU");
	SubstMxf.Init(0);
	float **Mx = SubstMxf.GetData();
	for (unsigned i = 0; i < N; ++i)
		{
		for (unsigned j = 0; j < N; ++j)
			{
			float v = Hox[i][j];
			v /= Div;
			
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
	SetUT(Mx);
	}

void SetHOXD55()
	{
	Set("HOXD55", HOXD55, 100.0f);
	}

void SetHOXD70()
	{
	Set("HOXD70", HOXD70, 100.0f);
	}

void SetPCRNA()
	{
	Set("PRRNA", PCRNA);
	}
