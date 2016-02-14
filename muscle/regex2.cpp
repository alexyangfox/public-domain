#include "myutils.h"
#include "regex.h"
#include "seqdb.h"

void SeqDB::GetShortLabel(unsigned Id, string &ShortLabel) const
	{
	ShortLabel.clear();

	static bool CompileDone = false;
	if (!CompileDone && opt_labelregex != "")
		{
		re_comp(opt_labelregex.c_str());
		CompileDone = true;
		}

	const string &Label = GetLabel(Id);
	if (CompileDone)
		{
		bool Matched = re_exec(Label.c_str());
		if (Matched)
			{
			unsigned L = GetGroupLength(1);
			if (L > 0)
				{
				const char *Grp = GetGroupStart(0);
				ShortLabel.reserve(L);
				for (unsigned i = 0; i < L; ++i)
					ShortLabel.push_back(Grp[i]);
				return;
				}
			}
		}

	if (SIZE(Label) <= opt_maxlabel)
		ShortLabel = Label;
	else
		for (unsigned i = 0; i < opt_maxlabel; ++i)
			ShortLabel.push_back(Label[i]);
	}
