#include "myutils.h"
#include "params.h"
#include "mx.h"
#include "seqdb.h"
#include <set>

void ReadSubstMx(const char *FileName, Mx<float> &Mxf);

static void AlgoFwdBwd(const string &Algo, const string &FwdBwdName, FWD_BWD FwdBwd);
static void AlgoTrans(const string &Algo, const string &Trans, float *Address);
static void ModelAlgo(const string &Model, const string &Algo);
static void ModelOptTrans(const string &Model, const string &Opt, const string &Trans);
static void ModelOptDefaultHelp(const string &Model, const string &Opt, float Default, const string &Help);
static void Opt(const string &Name, double *Address);

#define C(x)	float FwdBwd##x(Mx<float> &PPf); AlgoFwdBwd(#x, "FwdBwd" #x, FwdBwd##x);

#define A(x, y)			AlgoTrans(#x, #y, &Trans##y);
#define M(x, y)			ModelAlgo(#x, #y);
#define P(x, y, z)		ModelOptTrans(#x, #y, #z);
#define D(w, x, y, z)	ModelOptDefaultHelp(#w, #x, float(y), #x);
#define	Q(x)			Opt(#x, &opt_##x);

static string g_Model;

const string &GetModel()
	{
	return g_Model;
	}

void InitModelData()
	{
	static bool InitDone = false;
	if (InitDone)
		return;
#include "models.h"
	InitDone = true;
	}

struct AlgoFwdBwdData
	{
	string Algo;
	string FwdBwdName;
	FWD_BWD FwdBwd;
	};

struct AlgoTransData
	{
	string Algo;
	string Trans;
	float *Address;
	};

struct ModelAlgoData
	{
	string Model;
	string Algo;
	};

struct ModelOptTransData
	{
	string Model;
	string Opt;
	string Trans;
	};

struct ModelOptDefaultHelpData
	{
	string Model;
	string Opt;
	float Default;
	string Help;
	};

struct OptData
	{
	string Name;
	double *Address;
	};

static vector<AlgoFwdBwdData> g_AlgoFwdBwd;
static vector<AlgoTransData> g_AlgoTrans;
static vector<ModelAlgoData> g_ModelAlgo;
static vector<ModelOptTransData> g_ModelOptTrans;
static vector<ModelOptDefaultHelpData> g_ModelOptDefaultHelp;
static vector<OptData> g_Opt;
static set<string> g_Algos;

static void AlgoFwdBwd(const string &Algo, const string &FwdBwdName, FWD_BWD FwdBwd)
	{
	AlgoFwdBwdData D;
	D.Algo = Algo;
	D.FwdBwd = FwdBwd;
	D.FwdBwdName = FwdBwdName;
	g_AlgoFwdBwd.push_back(D);
	}

static void AlgoTrans(const string &Algo, const string &Trans, float *Address)
	{
	AlgoTransData D;
	D.Algo = Algo;
	D.Trans = Trans;
	D.Address = Address;
	g_AlgoTrans.push_back(D);

	if (g_Algos.find(Algo) == g_Algos.end())
		g_Algos.insert(Algo);
	}

static void ModelAlgo(const string &Model, const string &Algo)
	{
	ModelAlgoData D;
	D.Model = Model;
	D.Algo = Algo;
	g_ModelAlgo.push_back(D);
	}

static void ModelOptTrans(const string &Model, const string &Opt, const string &Trans)
	{
	ModelOptTransData D;
	D.Model = Model;
	D.Opt = Opt;
	D.Trans = Trans;
	g_ModelOptTrans.push_back(D);
	}

static void ModelOptDefaultHelp(const string &Model, const string &Opt, float Default,
  const string &Help)
	{
	ModelOptDefaultHelpData D;
	D.Model = Model;
	D.Opt = Opt;
	D.Default = Default;
	D.Help = Help;
	g_ModelOptDefaultHelp.push_back(D);
	}

static void Opt(const string &Name, double *Address)
	{
	OptData D;
	D.Name = Name;
	D.Address = Address;
	g_Opt.push_back(D);
	}

static const ModelOptTransData *GetModelTransOpt(const string &Model,
  const string &Trans)
	{
	for (unsigned i = 0; i < SIZE(g_ModelOptTrans); ++i)
		{
		const ModelOptTransData &MOT = g_ModelOptTrans[i];
		if (MOT.Model == Model && MOT.Trans == Trans)
			return &MOT;
		}
	return 0;
	}

static FWD_BWD GetFwdBwd(const string &Algo)
	{
	for (unsigned i = 0; i < SIZE(g_AlgoFwdBwd); ++i)
		{
		const AlgoFwdBwdData &D = g_AlgoFwdBwd[i];
		if (D.Algo == Algo)
			return D.FwdBwd;
		}
	Die("GetFwdBwd(%s), not found", Algo.c_str());
	ureturn(0);
	}

static float GetOptValue(const string &Name)
	{
	for (unsigned i = 0; i < SIZE(g_Opt); ++i)
		{
		const OptData &OD = g_Opt[i];
		if (OD.Name == Name)
			return *OD.Address;
		}
	Die("GetOptValue(%s), not found", Name.c_str());
	ureturn(0);
	}

static float GetDefaultOptValue(const string &Model, const string &Opt)
	{
	for (unsigned i = 0; i < SIZE(g_ModelOptDefaultHelp); ++i)
		{
		const ModelOptDefaultHelpData &D = g_ModelOptDefaultHelp[i];
		if (D.Model == Model && D.Opt == Opt)
			return D.Default;
		}
	Die("GetDefaultOptValue(%s, %s), not found", Model.c_str(), Opt.c_str());
	ureturn(0);
	}

static void SetModelParams(const ModelAlgoData &MAD)
	{
	const string &Algo = MAD.Algo;
	const string &Model = MAD.Model;

	const unsigned TransCount = SIZE(g_AlgoTrans);
	for (unsigned TransIndex = 0; TransIndex < TransCount; ++TransIndex)
		{
		const AlgoTransData &AT = g_AlgoTrans[TransIndex];
		if (AT.Algo == Algo)
			{
			const ModelOptTransData *MOT = GetModelTransOpt(Model, AT.Trans);
			if (MOT == 0)
				{
				*AT.Address = LOG_ZERO;
				if (opt_trace)
					Log("%s = *\n", AT.Trans.c_str());
				}
			else
				{
				float Value = GetOptValue(MOT->Opt);
				if (Value == FLT_MAX)
					{
					Value = GetDefaultOptValue(MAD.Model, MOT->Opt);
					if (opt_trace)
						Log("%s = %s = %g (default)\n", AT.Trans.c_str(), MOT->Opt.c_str(), Value);
					}
				else
					{
					if (opt_trace)
						Log("%s = %s = %g (command-line)\n", AT.Trans.c_str(), MOT->Opt.c_str(), Value);
					}

				*AT.Address = -Value;
				}
			}
		}
	}

void LogModels()
	{
	InitModelData();

	const unsigned ModelCount = SIZE(g_ModelAlgo);
	const unsigned TransCount = SIZE(g_AlgoTrans);

	for (set<string>::const_iterator p = g_Algos.begin();
	  p != g_Algos.end(); ++p)
		{
		const string &Algo = *p;
		Log("\n");
		Log("Algo %s\n", Algo.c_str());

		vector<string> Models;
		for (unsigned ModelIndex = 0; ModelIndex < ModelCount; ++ModelIndex)
			{
			const ModelAlgoData &MA = g_ModelAlgo[ModelIndex];
			if (MA.Algo == Algo)
				Models.push_back(MA.Model);
			}

		vector<string> Transs;
		for (unsigned TransIndex = 0; TransIndex < TransCount; ++TransIndex)
			{
			const AlgoTransData AT = g_AlgoTrans[TransIndex];
			if (AT.Algo == Algo)
				Transs.push_back(AT.Trans);
			}

		const unsigned AlgoModelCount = SIZE(Models);
		Log("%5.5s", "Trans");
		for (unsigned i = 0; i < AlgoModelCount; ++i)
			Log("  %16.16s", Models[i].c_str());
		Log("\n");

		Log("-----");
		for (unsigned i = 0; i < AlgoModelCount; ++i)
			Log("  ----------------");
		Log("\n");

		for (unsigned TransIndex = 0; TransIndex < SIZE(Transs); ++TransIndex)
			{
			const string &Trans = Transs[TransIndex].c_str();
			Log("%5.5s", Trans.c_str());

			for (unsigned i = 0; i < AlgoModelCount; ++i)
				{
				const string &Model = Models[i];
				const ModelOptTransData *MOT = GetModelTransOpt(Model, Trans);
				string s = "-";
				if (MOT != 0)
					s = MOT->Opt;
				Log("  %16.16s", s.c_str());
				}

			Log("\n");
			}
		}
	}

static void GetSubstMxName(const string &Model, string &Name)
	{
	if (opt_matrix != "")
		Name = opt_matrix;
	else if (Model == "localaffnuc" || Model == "globalaffnuc" || Model == "globalnuc")
		Name = "PCRNA";
	else
		Name = "PCCRFMX";
	}

void SetSubstMx(const string &Model)
	{
	string Name;
	if (Model.find('+') == string::npos)
		GetSubstMxName(Model, Name);
	else
		{
		vector<string> Fields;
		Split(Model, Fields, '+');
		if (Fields.size() != 2)
			Die("Invalid dual model %s", Model.c_str());

		const string &Model1 = Fields[0];
		const string &Model2 = Fields[1];

		string Name2;
		GetSubstMxName(Model1, Name);
		GetSubstMxName(Model2, Name2);
		
		if (Name2 != Name)
			Die("Invalid dual model, must use same subst matrix (%s,%s)",
			  Name.c_str(), Name2.c_str());
		}

	Mx<float> &SubstMxf = GetSubstMxf();
	if (SubstMxf.m_Name == Name)
		return;

	if (Name == "PCCRFMX")
		SetPCCRFMX();
	else if (Name == "HOXD70")
		SetHOXD70();
	else if (Name == "HOXD55")
		SetHOXD55();
	else if (Name == "PCRNA")
		SetPCRNA();
	else 
		ReadSubstMx(Name.c_str(), SubstMxf);
	}

FWD_BWD SetModel(const string &Model)
	{
	if (opt_trace)
		{
		Log("\n");
		Log("SetModel(%s)\n", Model.c_str());
		}

	InitModelData();
	SetSubstMx(Model);

	for (unsigned i = 0; i < SIZE(g_ModelAlgo); ++i)
		{
		const ModelAlgoData &MAD = g_ModelAlgo[i];
		if (MAD.Model == Model)
			{
			if (g_Model != Model)
				{
				SetModelParams(MAD);
				g_Model = Model;
				}
			return GetFwdBwd(MAD.Algo);
			}
		}
	Die("SetModel(%s), not found", Model.c_str());
	ureturn(0);
	}

static void LogModelParamsMAD(const ModelAlgoData &MAD)
	{
	const string &Algo = MAD.Algo;
	const string &Model = MAD.Model;

	const unsigned TransCount = SIZE(g_AlgoTrans);
	for (unsigned TransIndex = 0; TransIndex < TransCount; ++TransIndex)
		{
		const AlgoTransData &AT = g_AlgoTrans[TransIndex];
		if (AT.Algo == Algo)
			{
			const ModelOptTransData *MOT = GetModelTransOpt(Model, AT.Trans);
			if (MOT == 0)
				{
				*AT.Address = LOG_ZERO;
				Log("%s = *\n", AT.Trans.c_str());
				}
			else
				{
				float Value = GetOptValue(MOT->Opt);
				if (Value == FLT_MAX)
					{
					Value = GetDefaultOptValue(MAD.Model, MOT->Opt);
					Log("%s = %s = %g (default)\n", AT.Trans.c_str(), MOT->Opt.c_str(), Value);
					}
				else
					Log("%s = %s = %g (command-line)\n", AT.Trans.c_str(), MOT->Opt.c_str(), Value);
				}
			}
		}
	}

static void LogModelParamsModel(const string &Model)
	{
	Log("Model %s\n", Model.c_str());
	string MxName;
	GetSubstMxName(Model, MxName);
	SetSubstMx(Model);
	GetSubstMxf().LogMe();
	Log("Matrix %s\n", MxName.c_str());
	for (unsigned i = 0; i < SIZE(g_ModelAlgo); ++i)
		{
		const ModelAlgoData &MAD = g_ModelAlgo[i];
		if (MAD.Model == Model)
			{
			Log("Algorithm %s\n", MAD.Algo.c_str());
			LogModelParamsMAD(MAD);
			}
		}
	}

void LogModelParams()
	{
	InitModelData();

	const string &Model = opt_model;
	Log("\n");
	Log("Parameters:\n");
	vector<string> Models;
	Split(Model, Models, '+');
	const unsigned N = SIZE(Models);
	if (N > 1)
		Log("Multiple model: %s\n", Model.c_str());
	for (unsigned i = 0; i < SIZE(Models); ++i)
		{
		if (i > 0)
			Log("\n");
		LogModelParamsModel(Models[i]);
		}
	}
