#include "myutils.h"
#include "mx.h"

void ReadSubstMx(const char *FileName, Mx<float> &Mxf)
	{
	if (Mxf.m_RowCount != 256 || Mxf.m_ColCount != 256)
		Mxf.Clear();
	Mxf.Alloc(FileName, 256, 256);
	Mxf.Init(0);
	float **Mx = Mxf.GetData();

	FILE *f = OpenStdioFile(FileName);

	string Line;
	for (;;)
		{
		bool Ok = ReadLineStdioFile(f, Line);
		if (!Ok)
			Die("ReadSubstMx, end-of-file in %.32s without finding data", FileName);
		if (Line.empty() || Line[0] == '#')
			continue;
		else if (Line[0] == ' ')
			break;
		else
			Die("ReadSubstMx, file %.32s has unexpected line '%.32s'",
			  FileName, Line.c_str());
		}

	vector<string> Headings;
	Split(Line, Headings);

	unsigned N = (unsigned) Headings.size();
	for (unsigned Row = 0; Row < N; ++Row)
		{
		const string &Heading = Headings[Row];
		if (Heading.size() != 1)
			Die("ReadSubstMx(%.32s), heading '%s' not one char", FileName, Heading.c_str());
		byte RowLetter = (byte) Heading[0];

		bool Ok = ReadLineStdioFile(f, Line);
		if (!Ok)
			Die("ReadSubstMx, premature end-of-file in %.32s", FileName);

		vector<string> Values;
		Split(Line, Values);
		if (Values.size() != N + 1)
			Die("ReadSubstMx(%.32s), expected %u fields, got %u",
			  FileName, N + 1, (unsigned) Values.size());

		for (unsigned Col = 0; Col < N; ++Col)
			{
			const string &Heading = Headings[Col];
			if (Heading.size() != 1)
				Die("ReadSubstMx(%.32s), heading '%s' not one char", FileName, Heading.c_str());
			byte ColLetter = (byte) Heading[0];

			const string &strValue = Values[Col+1];
			float Value = (float) atof(strValue.c_str());
			Mx[RowLetter][ColLetter] = Value;
			}
		}
	Mxf.m_Alpha.clear();
	for (unsigned i = 0; i < N; ++i)
		Mxf.m_Alpha.push_back(Headings[i][0]);
	}
