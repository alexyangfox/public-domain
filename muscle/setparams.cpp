#include "myutils.h"
#include "params.h"
#include "sumlog.h"

static string g_Model;

const string &GetModel()
	{
	return g_Model;
	}

void SetParams(const string &Model)
	{
	g_Model = Model;
	}
