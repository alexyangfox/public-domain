#ifndef transparams_h
#define transparams_h

#include "mx.h"
#include "seqdb.h"

typedef float (*FWD_BWD)(Mx<float> &PPf);

#define T(x)	extern float Trans##x;
#include "transparams.h"

void SetSimMx(SeqDB &DB, unsigned IdA, unsigned IdB);
Mx<float> &GetSubstMxf();
Mx<float> &GetSimMxf();

float **GetSubstMx();
float **GetSimMx();
void LogParams();
void LogModelParams();
void MaskSubstMx();

void SetHOXD55();
void SetHOXD70();
void SetPCCRFMX();
void SetPCRNA();
void SetBLOSUM70C();

const string &GetModel();
void GetLocalModel(SeqDB &DB, string &Model);
FWD_BWD SetModel(const string &Model);

#endif // transparams_h
