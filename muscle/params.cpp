#include "myutils.h"
#include "mx.h"
#include "params.h"
#include "sumlog.h"

Mx<float> g_SubstMxf;

#define T(x)	float Trans##x;
#include "transparams.h"

Mx<float> &GetSubstMxf()
	{
	return g_SubstMxf;
	}

float **GetSubstMx()
	{
	asserta(g_SubstMxf.m_RowCount == 256 && g_SubstMxf.m_ColCount == 256);
	MaskSubstMx();
	return g_SubstMxf.GetData();
	}

void LogSubstMx()
	{
	GetSubstMxf().LogMe();
	}

void LogParams()
	{
	Log("\n");
	Log("Model=%s\n", GetModel().c_str());

#define T(x)	if (Trans##x != LOG_ZERO) Log(#x " = %g\n", Trans##x);
#include "transparams.h"

	g_SubstMxf.LogMe();
	}

void MaskSubstMx()
	{
	float **M = g_SubstMxf.GetData();
	if (!opt_mask || M[(byte) 'a'][(byte) 'a'] == 0)
		return;
	for (unsigned i = 0; i < 256; ++i)
		{
		for (unsigned j = 0; j < 256; ++j)
			{
			if (islower(i) || islower(j))
				M[i][j] = 0;
			}
		}
	}
