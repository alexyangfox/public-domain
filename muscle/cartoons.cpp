#include "myutils.h"

unsigned Overlap(unsigned StartA, unsigned EndA, unsigned StartB, unsigned EndB);
bool IsPalindrome(unsigned Start1, unsigned End1, unsigned Start2, unsigned End2);

unsigned UpdateCartoon(string &Cartoon,
  unsigned SeqLenBases, unsigned SeqStartPixel,
  unsigned FromBase, unsigned ToBase, char c)
	{
	const unsigned CartoonWidth = SIZE(Cartoon);
	const unsigned SeqLenPixels = CartoonWidth - SeqStartPixel;
	float BasesToPixels = float(SeqLenPixels)/float(SeqLenBases);
	unsigned FromPixel = SeqStartPixel + unsigned(FromBase*BasesToPixels + 0.5);
	unsigned ToPixel = SeqStartPixel + unsigned(ToBase*BasesToPixels + 0.5);
	FromPixel = min(FromPixel, CartoonWidth - 1);
	ToPixel = min(ToPixel, CartoonWidth - 1);
	for (unsigned i = FromPixel; i <= ToPixel; ++i)
		Cartoon[i] = c;
	return FromPixel;
	}

unsigned UpdateCartoon(string &Cartoon,
  unsigned SeqLenBases, unsigned SeqStartPixel,
  unsigned FromBase, unsigned ToBase, const string &s)
	{
	const unsigned CartoonWidth = SIZE(Cartoon);
	const unsigned SeqLenPixels = CartoonWidth - SeqStartPixel;
	float BasesToPixels = float(SeqLenPixels)/float(SeqLenBases);
	unsigned FromPixel = SeqStartPixel + unsigned(FromBase*BasesToPixels + 0.5);
	unsigned ToPixel = SeqStartPixel + unsigned(ToBase*BasesToPixels + 0.5);
	unsigned PixelCount = ToPixel - FromPixel + 1;
	const unsigned L = SIZE(s);
	if (PixelCount%L != 0)
		ToPixel += PixelCount + (L - PixelCount%L);
	FromPixel = min(FromPixel, CartoonWidth - 1);
	ToPixel = min(ToPixel, CartoonWidth - 1);
	unsigned j = 0;
	for (unsigned i = FromPixel; i <= ToPixel; ++i)
		{
		char c = s[j++];
		if (j >= L)
			j = 0;
		Cartoon[i] = c;
		}
	return FromPixel;
	}

static void InitSelfCartoon(string &Cartoon)
	{
	Cartoon.clear();
	const unsigned SeqWidthPixels = 32;
	Cartoon.resize(SeqWidthPixels, '~');
	}

static void InitCartoonPair(unsigned LA, unsigned LB,
  string &CartoonA, string &CartoonB,
  unsigned PosA, unsigned PosB,
  unsigned &SeqStartPixelA, unsigned &SeqStartPixelB)
	{
	CartoonA.clear();
	CartoonB.clear();

	const unsigned SeqWidthPixels = 32;
	const unsigned MaxL = max(LA, LB);
	float BasesToPixels = float(SeqWidthPixels)/float(MaxL);
	if (PosB >= PosA)
		{
		unsigned SeqStartPosA = PosB - PosA;
		SeqStartPixelA = unsigned(SeqStartPosA*BasesToPixels + 0.5);
		SeqStartPixelB = 0;
		}
	else
		{
		unsigned SeqStartPosB = PosA - PosB;
		SeqStartPixelA = 0;
		SeqStartPixelB = unsigned(SeqStartPosB*BasesToPixels + 0.5);
		}

	unsigned SeqLenPixelsA = unsigned(LA*BasesToPixels + 0.5);
	unsigned SeqLenPixelsB = unsigned(LB*BasesToPixels + 0.5);

	unsigned CartoonLenPixelsA = SeqStartPixelA + SeqLenPixelsA;
	unsigned CartoonLenPixelsB = SeqStartPixelB + SeqLenPixelsB;

	CartoonA.resize(CartoonLenPixelsA, '~');
	CartoonB.resize(CartoonLenPixelsB, '~');

	for (unsigned i = 0; i < SeqStartPixelA; ++i)
		CartoonA[i] = ' ';
	for (unsigned i = 0; i < SeqStartPixelB; ++i)
		CartoonB[i] = ' ';
	}

void LogSelfMatchCartoon(unsigned L, unsigned StartA, unsigned EndA,
  unsigned StartB, unsigned EndB)
	{
	unsigned Start1;
	unsigned Start2;
	unsigned End1;
	unsigned End2;
	if (StartA < StartB)
		{
		Start1 = StartA;
		End1 = EndA;
		Start2 = StartB;
		End2 = EndB;
		}
	else
		{
		Start1 = StartB;
		End1 = EndB;
		Start2 = StartA;
		End2 = EndA;
		}

	string Cartoon;
	InitSelfCartoon(Cartoon);
	UpdateCartoon(Cartoon, L, 0, Start1, End1, '1');
	UpdateCartoon(Cartoon, L, 0, Start2, End2, '2');

	if (End1 >= Start2)
		UpdateCartoon(Cartoon, L, 0, Start2, End1, 'X');

	Log("%u-%u, %u-%u %s %u\n",
	  Start1+1, End1+1, Start2+1, End2+1, Cartoon.c_str(), L);
	}

void LogSelfInvertCartoon(unsigned L,
  unsigned StartA, unsigned EndA, unsigned StartB, unsigned EndB)
	{
	bool Pal = IsPalindrome(StartA, EndA, StartB, EndB);

	unsigned Start1;
	unsigned Start2;
	unsigned End1;
	unsigned End2;
	if (StartA < StartB)
		{
		Start1 = StartA;
		End1 = EndA;
		Start2 = StartB;
		End2 = EndB;
		}
	else
		{
		Start1 = StartB;
		End1 = EndB;
		Start2 = StartA;
		End2 = EndA;
		}

	string Cartoon;
	InitSelfCartoon(Cartoon);
	if (Pal)
		UpdateCartoon(Cartoon, L, 0, Start1, End1, "<>");
	else
		{
		UpdateCartoon(Cartoon, L, 0, Start1, End1, '>');
		UpdateCartoon(Cartoon, L, 0, Start2, End2, '<');
		}

	Log("%u-%u, %u-%u %s %u\n",
	  Start1+1, End1+1, Start2+1, End2+1, Cartoon.c_str(), L);
	}

// From and To are ZERO-based
static void FormatFTPair(unsigned FromA, unsigned ToA,
  unsigned FromB, unsigned ToB, char *FTA, char *FTB)
	{
	char FromAstr[16];
	char FromBstr[16];

	char ToAstr[16];
	char ToBstr[16];

	char NAstr[16];
	char NBstr[16];

	sprintf(FromAstr, "%u", FromA+1);
	sprintf(FromBstr, "%u", FromB+1);

	sprintf(ToAstr, "%u", ToA+1);
	sprintf(ToBstr, "%u", ToB+1);

	unsigned NA = max(FromA, ToA) - min(FromA, ToA) + 1;
	unsigned NB = max(FromB, ToB) - min(FromB, ToB) + 1;

	sprintf(NAstr, "%u", NA);
	sprintf(NBstr, "%u", NB);

	unsigned FromN = (unsigned) max(strlen(FromAstr), strlen(FromBstr));
	unsigned ToN = (unsigned) max(strlen(ToAstr), strlen(ToBstr));
	unsigned NN = (unsigned) max(strlen(NAstr), strlen(NBstr));

	sprintf(FTA, "%*.*s - %*.*s (%*.*s)",
	  FromN, FromN, FromAstr, ToN, ToN, ToAstr, NN, NN, NAstr);
	sprintf(FTB, "%*.*s - %*.*s (%*.*s)",
	  FromN, FromN, FromBstr, ToN, ToN, ToBstr, NN, NN, NBstr);
	}

void LogLocalMatchCartoon(unsigned LA, unsigned LB, unsigned StartA,
  unsigned EndA, unsigned StartB, unsigned EndB, bool Self)
	{
	if (Self && Overlap(StartA, EndA, StartB, EndB) == 0)
		{
		asserta(LA == LB);
		LogSelfMatchCartoon(LA, StartA, EndA, StartB, EndB);
		return;
		}

	unsigned PosA = (StartA + EndA)/2;
	unsigned PosB = (StartB + EndB)/2;

	string CartoonA;
	string CartoonB;

	unsigned SeqStartPixelA;
	unsigned SeqStartPixelB;
	InitCartoonPair(LA, LB, CartoonA, CartoonB, PosA, PosB,
	  SeqStartPixelA, SeqStartPixelB);

	UpdateCartoon(CartoonA, LA, SeqStartPixelA, StartA, EndA, 'X');
	UpdateCartoon(CartoonB, LB, SeqStartPixelB, StartB, EndB, 'X');

	char FTA[64];
	char FTB[64];
	FormatFTPair(StartA, EndA, StartB, EndB, FTA, FTB);

	Log("%s  %s %u\n", FTA, CartoonA.c_str(), LA);
	Log("%s  %s %u\n", FTB, CartoonB.c_str(), LB);
	}

void LogInvertCartoon(unsigned LA, unsigned LB, unsigned StartA,
  unsigned EndA, unsigned StartB, unsigned EndB, bool Self)
	{
	if (Self)
		{
		LogSelfInvertCartoon(LA, StartA, EndA, StartB, EndB);
		return;
		}

	unsigned PosA = (StartA + EndA)/2;
	unsigned PosB = (StartB + EndB)/2;

	string CartoonA;
	string CartoonB;

	unsigned SeqStartPixelA;
	unsigned SeqStartPixelB;
	InitCartoonPair(LA, LB, CartoonA, CartoonB, PosA, PosB,
	  SeqStartPixelA, SeqStartPixelB);

	UpdateCartoon(CartoonA, LA, SeqStartPixelA, StartA, EndA, '>');
	UpdateCartoon(CartoonB, LB, SeqStartPixelB, StartB, EndB, '<');

	char FTA[64];
	char FTB[64];
	FormatFTPair(StartA, EndA, StartB, EndB, FTA, FTB);

	Log("%s  %s %u\n", FTA, CartoonA.c_str(), LA);
	Log("%s  %s %u\n", FTB, CartoonB.c_str(), LB);
	}
