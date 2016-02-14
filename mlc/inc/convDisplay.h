// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _convDisplay_h
#define _convDisplay_h 1

#include <math.h>
#include <basics.h>
#include <Array.h>
#include <Array2.h>
#include <InstList.h>
#include <StatData.h>
#include <GetOption.h>

enum Delimeter    { space, comma, period, semicolon, colon };
enum Normalization { noNormalization, normalDist, extreme };
enum Conversion   { local, binary, noConversion, ahaConv };


// delimeter options
const MEnum delimeterEnum =
  MEnum("space", space) <<
  MEnum("comma", comma) <<
  MEnum("period", period) <<
  MEnum("semicolon", semicolon) <<
  MEnum("colon", colon);
const MString attrDelimeterHelp = "This option specifies the character to be "
  "placed between attributes in the output file.";
const MString lastAttrDelimeterHelp = "This option specifies the character "
  "to place after the last attribute and before the label.";
const MString endOfInstanceHelp = "This option specifies the character to "
  "place at the end of an instance before the newline.";
// defaults:
const Delimeter defaultAttrDelim = comma;
const Delimeter defaultLastAttrDelim = comma;
const Delimeter defaultEolDelim = period;

// normalizing options
const MEnum normalizeEnum =
  MEnum("none", noNormalization) <<
  MEnum("normalDist", normalDist) <<
  MEnum("extreme", extreme);
const MString normalizeHelp = "This option specifies what type of "
  "normalizing should be done to continuous variables.";
const Normalization defaultNorm = noNormalization;


const MEnum conversionEnum =
  MEnum("local", local) <<
  MEnum("binary", binary) <<
  MEnum("none", noConversion) <<
  MEnum("aha", ahaConv);
const MString conversionHelp =
  "This option specifies the type of conversion to perform.  Local "
  "encoding represents nominal attributes as a set of boolean "
  "indicator attributes.  Binary encoding represents multi-valued "
  "nominals as a set of bit attributes which combine to form a "
  "binary number indicating the original value of the nominal.";

const Conversion defaultConversion = local;


MString delimeterString(Delimeter option);

Array2<Real>* create_continuous_stats(const InstanceList& trainingList,
				      Normalization normOpt);


void display_list_rep(const InstanceList& instList,
                      const SchemaRC& schema,
		      MLCOStream& stream,
		      Conversion conv,
		      Normalization normOpt,
		      const MString separator,
		      const MString labelSeperator, 
		      const MString eolChar,
		      Array2<Real>* continStats);

void display_list_pebls(const InstanceList& instList,
			MLCOStream& stream,
			const MString& header);

void display_names(const SchemaRC& schema,
		   MLCOStream& stream,
		   Conversion conv,
		   const MString& header);

void display_names_local(const SchemaRC& schema,
			 MLCOStream& stream,
			 const MString& header);

void display_names_bin(const SchemaRC& schema,
		       MLCOStream& stream,
		       const MString& header);

void display_names_aha(const SchemaRC& schema, MLCOStream& stream);
void display_list_OC1(const InstanceList& instList,
		      MLCOStream& stream);
void display_description_aha(const SchemaRC& schm, MLCOStream& stream);

// cn2 routines.
void display_list_CN2(const InstanceList& trainList,
		      MLCOStream& outDatafile);
void display_names_CN2(const SchemaRC& schema,
		       MLCOStream& outAttrfile);

void display_script_CN2(const MString& datafile,
			const MString& testfile,
			const MString& attrfile,
			MLCOStream& outScriptfile);
#endif
