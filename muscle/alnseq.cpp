#include "myutils.h"

bool IsEmitState(char c, bool First)
	{
	if (c == 'M')
		return true;
	if (First)
		return c == 'D';
	else
		return c == 'I';
	}

byte *MakeGappedSeq(const byte *A, const string &Path, bool First)
	{
	unsigned i = 0;
	const unsigned ColCount = SIZE(Path);
	byte *sA = myalloc<byte>(ColCount);
	for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
		{
		char c = Path[ColIndex];
		if (IsEmitState(c, First))
			{
			char a = A[i++];
			sA[ColIndex] = a;
			}
		else
			sA[ColIndex] = '-';
		}
	return sA;
	}

// LoPos is lo offset of aligned region in non-rev-comp'd sequence.
byte *MakeGappedSeqRevComp(const byte *A, unsigned LoPos, const string &Path,
  bool First)
	{
	const unsigned ColCount = SIZE(Path);
	byte *sA = myalloc<byte>(ColCount);
	unsigned N = 0;
	for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
		{
		char c = Path[ColIndex];
		if (IsEmitState(c, First))
			++N;
		}
	unsigned i = LoPos + N -1;
	for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
		{
		char c = Path[ColIndex];
		if (IsEmitState(c, First))
			{
			byte CompLetter(byte c);

			char a = A[i--];
			sA[ColIndex] = CompLetter(a);
			}
		else
			sA[ColIndex] = '-';
		}
	return sA;
	}

byte *MakeGappedSeq2(const byte *A, unsigned LoPos, const string &Path,
  bool First, bool Plus)
	{
	if (!Plus)
		return MakeGappedSeqRevComp(A, LoPos, Path, First);

	unsigned i = LoPos;
	const unsigned ColCount = SIZE(Path);
	byte *sA = myalloc<byte>(ColCount);
	for (unsigned ColIndex = 0; ColIndex < ColCount; ++ColIndex)
		{
		char c = Path[ColIndex];
		if (IsEmitState(c, First))
			{
			char a = A[i++];
			sA[ColIndex] = a;
			}
		else
			sA[ColIndex] = '-';
		}
	return sA;
	}
