#ifndef info_h
#define info_h

struct RepeatInfo
	{
	unsigned InputSeqIndex;
	string Label;
	unsigned Start;
	unsigned End;
	unsigned Length;
	float Count;
	float AvgPctId;
	};

struct DupeInfo
	{
	unsigned InputSeqIndex;
	string Label;
	unsigned Start1;
	unsigned End1;
	unsigned Start2;
	unsigned End2;
	float PctId;
	};

struct InvertInfo
	{
	unsigned InputSeqIndex1;
	unsigned InputSeqIndex2;
	string Label1;
	string Label2;
	unsigned Start1;
	unsigned End1;
	unsigned Start2;
	unsigned End2;

	bool operator<(const InvertInfo &rhs) const
		{
		if (InputSeqIndex1 < rhs.InputSeqIndex1)
			return true;
		if (InputSeqIndex1 == rhs.InputSeqIndex1 && InputSeqIndex2 < rhs.InputSeqIndex2)
			return true;
		return false;
		}
	};

const vector<RepeatInfo> &GetRepeatInfos();
const vector<DupeInfo> &GetDupeInfos();
const vector<InvertInfo> &GetInvertInfos();

#endif // info_h
