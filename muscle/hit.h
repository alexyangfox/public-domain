#ifndef hit_h
#define hit_h

struct SegData
	{
	unsigned Lo;
	unsigned Hi;
	bool Strand;

	unsigned GetLength() const
		{
		return Hi - Lo + 1;
		}
	};

struct SeqDB;

struct HitData
	{
	HitData()
		{
		Clear();
		}

	HitData(const HitData &rhs)
		{
		*this = rhs;
		}

	unsigned LoA;
	unsigned HiA;
	unsigned LoB;
	unsigned HiB;
	bool Strand;
	float Score;
	string Path;
	unsigned User;

	void Clear()
		{
		LoA = UINT_MAX;
		HiA = UINT_MAX;
		LoB = UINT_MAX;
		HiB = UINT_MAX;
		User = UINT_MAX;
		Strand = false;
		Score = 0;
		Path.clear();
		}

	HitData &operator=(const HitData &rhs)
		{
	// If !Strand, LoB,HiB are 0-based w.r.t start of B /before/ rev-comp.
		LoA = rhs.LoA;
		LoB = rhs.LoB;
		HiA = rhs.HiA;
		HiB = rhs.HiB;
		Strand = rhs.Strand;
		Score = rhs.Score;
		Path = rhs.Path;
		User = rhs.User;
		return *this;
		}

	unsigned GetLo(unsigned SeqIndex) const
		{
		return SeqIndex == 0 ? LoA : LoB;
		}

	unsigned GetHi(unsigned SeqIndex) const
		{
		return SeqIndex == 0 ? HiA : HiB;
		}

	unsigned GetLengthA() const
		{
		return HiA - LoA + 1;
		}

	unsigned GetLengthB() const
		{
		return HiB - LoB + 1;
		}

	unsigned GetAvgLength() const
		{
		return (HiA - LoA + 1 + HiB - LoB + 1)/2;
		}

	void GetSegA(SegData &Seg) const
		{
		Seg.Lo = LoA;
		Seg.Hi = HiA;
		Seg.Strand = true;
		}

	void GetSegB(SegData &Seg) const
		{
		Seg.Lo = LoB;
		Seg.Hi = HiB;
		Seg.Strand = Strand;
		}

	void ValidatePath() const
		{
		void GetLetterCounts(const string &Path, unsigned &i, unsigned &j);
		unsigned Ni;
		unsigned Nj;
		GetLetterCounts(Path, Ni, Nj);
		asserta(HiA == LoA + Ni - 1);
		asserta(HiB == LoB + Nj - 1);
		}

	void LogMe(bool WithPath = false) const
		{
		Log("%u-%u, %u-%u%c", LoA, HiA, LoB, HiB, pom(Strand));
		if (WithPath)
			Log(" %s\n", Path.c_str());
		}
	};

struct BPData
	{
	unsigned Pos;
	bool lo;
	unsigned User;

	bool operator<(const BPData &rhs) const
		{
		if (Pos == rhs.Pos)
			return lo && !rhs.lo;

		return Pos < rhs.Pos;
		}
	};

void LogHits(const vector<HitData> &Hits);
void DotPlotHits(const vector<HitData> &Hits, unsigned LA, unsigned LB);
void AppendBPs(const vector<HitData> &Hits, vector<BPData> &BPs, bool DoA);
void GetUncoveredSegs(vector<BPData> &BPs, unsigned SeqLength,
  vector<SegData> &Segs);

#endif
