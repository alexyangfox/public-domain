#include "myutils.h"
#include "isgap.h"

static byte g_DNAIndexToAA[65];

static byte LetterToDNA(byte c)
	{
	switch (c)
		{
	case 'A':
	case 'a':
		return 0;
	case 'C':
	case 'c':
		return 1;
	case 'G':
	case 'g':
		return 2;
	case 'T':
	case 't':
		return 3;
		}
	return UCHAR_MAX;
	}

static unsigned DNAToIndex(const byte *DNA)
	{
	unsigned b1 = LetterToDNA(DNA[0]);
	unsigned b2 = LetterToDNA(DNA[1]);
	unsigned b3 = LetterToDNA(DNA[2]);
	if (b1 == UCHAR_MAX || b2 == UCHAR_MAX || b3 == UCHAR_MAX)
		return 64;

	unsigned Index = b1*16 + b2*4 + b3;
	assert(Index < 64);
	return Index;
	}

static void InitCodon(const char *DNA, byte AA)
	{
	unsigned Index = DNAToIndex((const byte *) DNA);
	g_DNAIndexToAA[Index] = AA;
	}

byte CodonToAA(const byte *DNA)
	{
	unsigned Index = DNAToIndex(DNA);
	return g_DNAIndexToAA[Index];
	}

static bool InitTable()
	{
	InitCodon("AAA", 'K');
	InitCodon("AAC", 'N');
	InitCodon("AAG", 'K');
	InitCodon("AAT", 'N');
	InitCodon("ACA", 'T');
	InitCodon("ACC", 'T');
	InitCodon("ACG", 'T');
	InitCodon("ACT", 'T');
	InitCodon("AGA", 'R');
	InitCodon("AGC", 'S');
	InitCodon("AGG", 'R');
	InitCodon("AGT", 'S');
	InitCodon("ATA", 'I');
	InitCodon("ATC", 'I');
	InitCodon("ATG", 'M');
	InitCodon("ATT", 'I');
	InitCodon("CAA", 'Q');
	InitCodon("CAC", 'H');
	InitCodon("CAG", 'Q');
	InitCodon("CAT", 'H');
	InitCodon("CCA", 'P');
	InitCodon("CCC", 'P');
	InitCodon("CCG", 'P');
	InitCodon("CCT", 'P');
	InitCodon("CGA", 'R');
	InitCodon("CGC", 'R');
	InitCodon("CGG", 'R');
	InitCodon("CGT", 'R');
	InitCodon("CTA", 'L');
	InitCodon("CTC", 'L');
	InitCodon("CTG", 'L');
	InitCodon("CTT", 'L');
	InitCodon("GAA", 'E');
	InitCodon("GAC", 'D');
	InitCodon("GAG", 'E');
	InitCodon("GAT", 'D');
	InitCodon("GCA", 'A');
	InitCodon("GCC", 'A');
	InitCodon("GCG", 'A');
	InitCodon("GCT", 'A');
	InitCodon("GGA", 'G');
	InitCodon("GGC", 'G');
	InitCodon("GGG", 'G');
	InitCodon("GGT", 'G');
	InitCodon("GTA", 'V');
	InitCodon("GTC", 'V');
	InitCodon("GTG", 'V');
	InitCodon("GTT", 'V');
	InitCodon("TAA", '*');
	InitCodon("TAC", 'Y');
	InitCodon("TAG", '*');
	InitCodon("TAT", 'Y');
	InitCodon("TCA", 'S');
	InitCodon("TCC", 'S');
	InitCodon("TCG", 'S');
	InitCodon("TCT", 'S');
	InitCodon("TGA", '*');
	InitCodon("TGC", 'C');
	InitCodon("TGG", 'W');
	InitCodon("TGT", 'C');
	InitCodon("TTA", 'L');
	InitCodon("TTC", 'F');
	InitCodon("TTG", 'L');
	InitCodon("TTT", 'F');
	InitCodon("NNN", 'X');
	return true;
	}
static bool InitDone = InitTable();

void DNASeqToAA(const byte *Seq, unsigned L, string &AAs)
	{
	AAs.clear();
	AAs.reserve(L/3);

	byte Codon[3];
	unsigned Pos = 0;
	for (unsigned i = 0; i < L; ++i)
		{
		byte c = Seq[i];
		if (isgap(c))
			continue;
		Codon[Pos++] = c;
		if (Pos == 3)
			{
			char AA = CodonToAA(Codon);
			AAs.push_back(AA);
			Pos = 0;
			}
		}
	}
