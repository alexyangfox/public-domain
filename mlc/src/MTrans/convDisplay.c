// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Display a bag, converting nominal attributes to
                   a local representation where a nominal attributes having
                   k values will be converted into k comma-separated
                   bits, all zero, except the one corresponding to its value.`
		 Convert MLC data format into Aha's IB data format.   
  Assumptions  : 
  Comments     : All non-nominal attributes are not converted to binary,
                   although they may be normalized.
                 The label is not converted either.
		 Separators should include the trailing space.
  Complexity   : O(sum_{i \in attribute) num_val(i) * n) where n is
                    the number of instances.
  Enhancements : Allow names file to have 0,1 or continuous.
                 Make this collection of utilities into a class, including
		   adding set_user_options function.
  History      : Yeogirl Yun                                        7/16/95
                   Added conversion routine for CN2 program    
                 Yeogirl Yun                                        6/17/95
                   Added conversion routine for OC1 program
                 Yeogirl Yun                                        5/29/95
                   Added conversion routine for Aha's IB
                 Robert Allen                                       2/10/95
                   Add options for attribute delimeters
		   and normalizing continuous attributes.
                 Chia-Hsin Li
                   Added code to handle the unknowns.
                 Ronny Kohavi                                       9/06/93
                   Initial revision (.h,.c)
 ***************************************************************************/

#include <convDisplay.h>
#include <Attribute.h>

RCSID("MLC++, $RCSfile: convDisplay.c,v $ $Revision: 1.19 $")


MString delimeterString(Delimeter option)
{
   switch(option) {
      case space:
	 return "";
      case comma:
	 return ",";
      case period:
	 return ".";
      case semicolon:
	 return ";";
      case colon:
	 return ":";
   }
   return "";
}


static void display_one_attr(const MString separator,
			     int position,
			     NominalVal val,
			     MLCOStream& stream)
{
   if (position + FIRST_CATEGORY_VAL == val)
      stream << '1' << separator;
   else
      stream << '0' << separator;
}

/***************************************************************************
  Description : Display single attribute with separators
  Comments    :
***************************************************************************/

static void display_nominal_attr(const MString endSeparator,
				 const NominalAttrInfo& nai,
				 NominalVal val,
				 MLCOStream& stream)
{
   stream << nai.get_value(val) << endSeparator;
}



/***************************************************************************
  Description : Display the local-encoding conversion of the Nominal val.
                  Then dump the output to stream.  The midSeparator and
		  endSeparator will be the same except when there is a
		  need to delimit between original attributes, e.g. after
		  the last attribute and before the label in some data formats.
  Comments    : In binary-encoding, representation of all 0's is reserved for
                unknown value. Thus, 0 is represented as (0,0,...,0,1) and
		1 is represented as 2, etc.
***************************************************************************/
static void display_nominal_attr_local(const MString midSeparator,
				       const MString endSeparator,
				       const NominalAttrInfo& nai,
                                       NominalVal val,
                                       MLCOStream& stream)
{
   if (val == UNKNOWN_CATEGORY_VAL) {
      for (int i = 0; i < nai.num_values() - 1;
		      stream << '0' << midSeparator, i++);
      stream << '0' << endSeparator;
   }
   else {
      for (int i = 0; i < nai.num_values() - 1; i++)
	 display_one_attr(midSeparator, i, val, stream);
      display_one_attr(endSeparator, i, val, stream);
   }
}


static void display_nominal_attr_bin(const MString midSeparator,
				     const MString endSeparator,
				     const NominalAttrInfo& nai,
				     NominalVal val,
				     MLCOStream& stream)
{
   ASSERT(val >= -1);
   ASSERT(nai.num_values() > 1);
   // The first "+1" is for unknown value.
   int reversedStrLength=(int) ceil(log(nai.num_values()+1) / log(2));
   Array<char> reversedStr(reversedStrLength);
   int i;

   if (val == UNKNOWN_CATEGORY_VAL) {
      for (i = 0; i < reversedStrLength - 1; i++)
	 stream << '0' << midSeparator;
      stream << '0' << endSeparator;
   }
   else {
      val++; // 0 is reserved for unknown, and 0 is represented as 1.
      int pos = reversedStrLength-1;
      for (i = 1;i < (nai.num_values()+1); i *= 2, pos--) {
	 if ((val % 2) == 1)
	    reversedStr[pos] = '1';
	 else
	    reversedStr[pos] = '0';
	 val = val >> 1;
      }
      for (i=0; i < reversedStrLength - 1; i++)
	 stream << reversedStr[i] << midSeparator;
      stream << reversedStr[i] << endSeparator;
   }
}

/***************************************************************************
  Description : Display the converted representation of an attribute.
                  If localBin is TRUE, local-encoding conversion is
		  represented. Else, binary-encoding conversion is represented.
  Comments    :
***************************************************************************/
static void display_attr(int attrNum,
			 const SchemaRC& schema,
			 const InstanceRC& instance,
			 MLCOStream& stream,
			 Conversion conv,
			 Normalization normOpt,
			 const MString midSeparator,
			 const MString endSeparator,
			 const Array2<Real>* continStats = NULL)
{
   const AttrInfo& attrInfo = schema.attr_info(attrNum);
   if (!attrInfo.can_cast_to_nominal())
      switch (normOpt) {
	 case noNormalization:
	    stream <<  attrInfo.attrValue_to_string(instance[attrNum])
		   << endSeparator;
	    break;
	 case normalDist:
	    {
	       if (continStats == NULL)
		  err << "convDisplay:display_inst_rep: Normalization option "
		     "requested but continStats not provided" << fatal_error;
	       
	       const RealAttrInfo& rai = attrInfo.cast_to_real();
	       if (rai.is_unknown(instance[attrNum]))
		  stream << "?" << endSeparator;
               else {
		  Real val = rai.get_real_val(instance[attrNum]);
		  Real normVal =
		     (val - (*continStats)(attrNum,0) ) /
		     (*continStats)(attrNum,1);
		  stream <<  normVal << endSeparator;
	       }
	       break;
	    }
	 case extreme:
            {
	       const RealAttrInfo& rai = attrInfo.cast_to_real();
	       if (rai.is_unknown(instance[attrNum]))
		  stream << "?" << endSeparator;
               else {
		  Real val = rai.normalized_value(instance[attrNum]);
		  stream <<  val << endSeparator;
	       }
	       break;
	    }
               
	 default:
	    err << "convDisplay:display_inst_rep: Normilization option "
		<< normOpt << " not recognized." << fatal_error;
      }
   else {
      const NominalAttrInfo& nai = attrInfo.cast_to_nominal();
      if (conv == local)  // if TRUE, present local conversion
	 display_nominal_attr_local(midSeparator,
				    endSeparator,
				    nai,
				    nai.get_nominal_val(instance[attrNum]),
				    stream);
      else if (conv == binary)
	 display_nominal_attr_bin(midSeparator,
				  endSeparator,
				  nai,
				  nai.get_nominal_val(instance[attrNum]),
				  stream);
      else if (conv == noConversion)
	 display_nominal_attr(endSeparator,
			      nai,
			      nai.get_nominal_val(instance[attrNum]),
			      stream);
      else
	 ASSERT(FALSE);
   }
}

/***************************************************************************
  Description : Display the converted representation of an instance.
  Comments    : We don't want the schema from the instance, because
                  when we convert the test-set, we want the schema
		  from the training-set bag, so that it contains
		  the appropriate min/max for normalized_value.
***************************************************************************/
static void display_inst_rep(const InstanceRC& instance,
			     const SchemaRC& schema,
			     MLCOStream& stream,
			     Conversion conv,
			     Normalization normOpt,
			     const MString separator,
			     const MString labelSeparator,
			     const MString eolChar,
			     const Array2<Real>* continStats)
{
   //   create function for normalizing continuous.
   //   get options in initial function? or let options be shipped in?
   
   for (int attrNum = 0; attrNum < schema.num_attr() - 1; attrNum++) {
      display_attr(attrNum, schema, instance, stream, conv, normOpt,
		   separator, separator, continStats);
   }
   if (schema.num_attr() > 0)
      display_attr(attrNum, schema, instance, stream, conv, normOpt,
		   separator, labelSeparator, continStats);

   const AttrInfo& attrInfo = instance.label_info();
   stream << attrInfo.attrValue_to_string(instance.get_label()) + eolChar
	  << endl;
}

/*****************************************************************************
  Description : Print one instance in OC1 format (local encoding in case of
                  a nominal with labels being numbers).
  Comments    :
*****************************************************************************/
static void display_inst_rep_OC1(const InstanceRC& instance,
				 MLCOStream& stream,
				 Bool hasNominals)
{
   const SchemaRC& schema = instance.get_schema();
   for (int attrNum = 0; attrNum <= schema.num_attr() - 1; attrNum++)
      if (hasNominals)
	 display_attr(attrNum, schema, instance, stream, local,
		      noNormalization, '\t', '\t', NULL);
      else {
	 const RealAttrInfo& rai =
	    schema.attr_info(attrNum).cast_to_real();
	 if (rai.is_unknown(instance[attrNum])) 
	    stream << '?' << '\t';
	 else
	    stream << rai.get_real_val(instance[attrNum]) << '\t';
      }

   const AttrInfo& attrInfo = instance.label_info();
   NominalVal val = attrInfo.get_nominal_val(instance.get_label()) + 1;
   stream << val << endl;
}



/***************************************************************************
  Description : Helper function for create_continuous_stats().
  Comments    :
***************************************************************************/
void extract_stat_data(StatData& statData,
		       Array2<Real>* continStats, int attrNum)
{
   if (statData.size() == 0 ) {
      (*continStats)(attrNum,0) = 0.0;  // mean
      (*continStats)(attrNum,1) = 1.0;  // S.Dev.
   }
   else {
      (*continStats)(attrNum,0) = statData.mean();
      if (statData.size() == 1 )
	 (*continStats)(attrNum,1) = 1.0;
      else {
	 Real var = statData.variance();
	 if (var == 0)
	    (*continStats)(attrNum,1) = 0.01;
	 else
	    (*continStats)(attrNum,1) = sqrt(var);
      }
   }
}


/***************************************************************************
  Description : Create a table of stats so that contiuous variables can
                  be normalized.  Should be called with training instances
		  or all instances.  Do not call seperately for test set
		  and training set or the normalization process may convert
		  different data into the same value.
  Comments    :
***************************************************************************/
Array2<Real>* create_continuous_stats(const InstanceList& trainingList,
				      Normalization normOpt)
{
   Array2<Real>* continStats = NULL;
   if (normOpt == normalDist) {
      // loop through the attributes and create a array of statistical data
      const SchemaRC schema =
	 trainingList.get_instance(trainingList.first()).get_schema();
      int numAttributes = schema.num_attr();

      Array<StatData> continStatData(0, numAttributes);
      continStats = new Array2<Real>(0, 0, numAttributes, 2, 0.0);
      
      // loop through the attributes
      for (int attrNum = 0; attrNum < numAttributes; attrNum++) {
	 const AttrInfo& attrInfo = schema.attr_info(attrNum);

	 // for each continuous attribute, read the data and create stats
	 if (!attrInfo.can_cast_to_nominal()) { 
	  
	    for (Pix pix = trainingList.first(); pix; trainingList.next(pix)) {
	       const InstanceRC& inst = trainingList.get_instance(pix);
	       if ( !attrInfo.is_unknown(inst[attrNum])) { // Ignore unknowns.
		  Real value = attrInfo.get_real_val(inst[attrNum]);
		  continStatData[attrNum].insert( value );
	       }
	    }
	    // extract Normal Density parameters into continStats table
	    extract_stat_data(continStatData[attrNum], continStats, attrNum);
	 }
      }
   }
   return continStats;
}
				     
				     
/***************************************************************************
  Description : Display the converted representation of a instanceList.
                conv determines the conversion.
  Comments    :
***************************************************************************/
void display_list_rep(const InstanceList& instList,
		      const SchemaRC& schema,
		      MLCOStream& stream,
		      Conversion conv,
		      Normalization normOpt,
		      const MString separator,
		      const MString labelSeperator,
		      const MString eolChar,
		      Array2<Real>* continStats)
{
   ASSERT(normOpt == noNormalization || normOpt == extreme || continStats);

   // translate each instance
   for (Pix pix = instList.first(); pix; instList.next(pix)) {
      InstanceRC instance = instList.get_instance(pix);
      display_inst_rep(instance, schema, stream, conv, normOpt,
		       separator, labelSeperator, eolChar, continStats);
   }
   if (instList.no_instances())
      stream << "|No instances" << endl;
}


/***************************************************************************
  Description : Display the Pebls format of a instanceList.
  Comments    :
***************************************************************************/
void display_list_pebls(const InstanceList& instList,
			MLCOStream& stream,
			const MString& header)
{
   // translate each instance
   int seqNum = 0;
   stream << header << endl; 
   for (Pix pix = instList.first(); pix; instList.next(pix)) {
      const InstanceRC& instance = instList.get_instance(pix);
      const AttrInfo& attrInfo = instance.label_info();
      stream << attrInfo.attrValue_to_string(instance.get_label()) 
	     << ", " << seqNum++ << ", "; 
      const SchemaRC& schema = instance.get_schema();
      for (int attrNum = 0; attrNum < schema.num_attr() - 1; attrNum++) 
	 stream << schema.attr_info(attrNum).
	    attrValue_to_string(instance[attrNum]) << " ";
      stream << endl;
   }
   if (instList.no_instances())
      stream << "#No instances" << endl;
}


/*****************************************************************************
  Description : Display the OC1 format of an InstanceList.
  Comments    :
*****************************************************************************/
void display_list_OC1(const InstanceList& instList,
		      MLCOStream& stream)
{
   const SchemaRC& schema = instList.get_schema();
   // now find out if there are nominal attributes.
   Bool hasNominals = FALSE;
   for (int i = 0; i < schema.num_attr(); i++) 
      if (schema.attr_info(i).can_cast_to_nominal()) {
	 hasNominals = TRUE;
	 break;
      }

   for (Pix pix = instList.first(); pix; instList.next(pix)) 
      display_inst_rep_OC1(instList.get_instance(pix), stream,
			   hasNominals);
}



/*****************************************************************************
  Description : Dispatch appropriate display_names_XXX routines based on
                  conv arguement value.
  Comments    :
*****************************************************************************/
void display_names(const SchemaRC& schema,
		   MLCOStream& stream,
		   Conversion conv,
		   const MString& header)
{
   if (conv == local)
      display_names_local(schema, stream, header);
   else if (conv == binary)
      display_names_bin(schema, stream, header);
   else if (conv == noConversion)
      schema.display_names(stream, TRUE, header);
   else if (conv == ahaConv)
      display_names_aha(schema, stream);
   else
      err << "display_names: unknown conversion method : " << conv <<
	 fatal_error;
}
      

/*****************************************************************************
  Description  : Create the names file with the new attribute descriptions.
                   The first function is for local encoding, the second for
		   binary encoding.
  Comments     :
*****************************************************************************/
void display_names_local(const SchemaRC& schm,
			 MLCOStream& stream,
			 const MString& header)
{
   stream << "|" << header << endl;
   schm.label_info().display_attr_values(stream);
   // display attribute values
   for (int attrNum = 0; attrNum < schm.num_attr(); attrNum++) {
      const AttrInfo& attrInfo = schm.attr_info(attrNum);
      if (!schm.attr_info(attrNum).can_cast_to_nominal()) {
	 stream << attrInfo.name() << ": "; 
	 attrInfo.display_attr_values(stream);
      }
      else {
	 const NominalAttrInfo& nai = attrInfo.cast_to_nominal();
	 for (int valueNum = 0;
	      valueNum < nai.num_values();
	      valueNum++) {
	    stream << nai.name() << "-" << nai.get_value(valueNum) <<
	       ": continuous" << endl;
	 }
      }
   }
}


void display_names_bin(const SchemaRC& schm,
		       MLCOStream& stream,
		       const MString& header)
{
   stream << "|" << header << endl;
   schm.label_info().display_attr_values(stream);

   // display attribute values
   for (int attrNum = 0; attrNum < schm.num_attr(); attrNum++) {
      const AttrInfo& attrInfo = schm.attr_info(attrNum);
      if (!schm.attr_info(attrNum).can_cast_to_nominal()) {
	 stream << attrInfo.name() << ": ";
	 attrInfo.display_attr_values(stream);
      }
      else {
	 const NominalAttrInfo& nai = attrInfo.cast_to_nominal();
	 int bitNum = (int) ceil(log(nai.num_values()+1) / log(2));
	 for (int valueNum = 0;
	      valueNum < bitNum;
	      valueNum++) {
	    stream << nai.name() << "-B" << valueNum <<
	       ": continuous" << endl;
	 }
      }
   }
}



/*****************************************************************************
  Description : Display Aha's namesfile from given InstanceList.
  Comments    : Aha doesn't seem to provide comment capability in the names
                  files.
*****************************************************************************/
void display_names_aha(const SchemaRC& schm, MLCOStream& stream)
{
   // display attribute values
   for (int attrNum = 0; attrNum < schm.num_attr(); attrNum++) {
      const AttrInfo& attrInfo = schm.attr_info(attrNum);
      if (!schm.attr_info(attrNum).can_cast_to_nominal()) {
	 stream << attrInfo.name() << ": numeric";
      }
      else {
	 const NominalAttrInfo& nai = attrInfo.cast_to_nominal();
	 stream << nai.name() << " : nominal "; 

	 int numValues = nai.num_values();
	 for (int i = 0; i < numValues; i++) {
	    stream << nai.get_value(FIRST_CATEGORY_VAL+i);
	    if (i != numValues - 1)
	       stream << ",";
	 }
      }
      stream << endl;
   }

   // now display label info.
   stream << "class: nominal ";
   // assume that labels are always nominal.
   const NominalAttrInfo& nai = schm.label_info().cast_to_nominal();
   int numValues = nai.num_values();
   for (int i = 0; i < numValues; i++) {
      stream << nai.get_value(FIRST_CATEGORY_VAL+i);
      if (i != numValues - 1)
	 stream << ",";
   }
   stream << endl; // required newline
}
   

/*****************************************************************************
  Description : Displays description file for Aha's IB inducers.
  Comments    :
*****************************************************************************/
void display_description_aha(const SchemaRC& schm, MLCOStream& stream)
{
   for (int attrNum = 0; attrNum < schm.num_attr(); attrNum++) 
      stream << "0 ";
   stream << "1" << endl;
   
   for (attrNum = 0; attrNum < schm.num_attr(); attrNum++) 
      stream << "1 ";
   stream << "0" << endl ;
}


/*****************************************************************************
  Description : Displays intances in InstanceList in CN2 format.
  Comments    :
*****************************************************************************/
void display_list_CN2(const InstanceList& trainList, MLCOStream& out)
{
   out << "**EXAMPLE FILE**" << endl << endl;
   display_list_rep(trainList, trainList.get_schema(),
		    out, noConversion, noNormalization, " ", " ", ";", NULL);
   out << endl;
}


/*****************************************************************************
  Description : Displays names files in CN2 format.
  Comments    :
*****************************************************************************/
void display_names_CN2(const SchemaRC& schema, MLCOStream& out)
{
   out << "**ATTRIBUTE FILE**" << endl << endl;

   // display attribute values
   for (int attrNum = 0; attrNum < schema.num_attr(); attrNum++) {
      const AttrInfo& ai = schema.attr_info(attrNum);
      out << "attr_" << attrNum << ": ";
      if (ai.can_cast_to_real())
	 out << "(FLOAT) " << endl;
      else {
	 const NominalAttrInfo& nai = ai.cast_to_nominal();
	 int numValues = nai.num_values();
	 for (int i = 0; i < numValues; i++) {
	    out << nai.get_value(FIRST_CATEGORY_VAL+i);
	    if (i != numValues - 1)
	       out << " ";
	    else
	       out << ";" << endl;
	 }
      }
   }

   out << "class: ";
   const NominalAttrInfo& nai = schema.label_info().cast_to_nominal();
   int numValues = nai.num_values();
   for (int i = 0; i < numValues; i++) {
      out << nai.get_value(FIRST_CATEGORY_VAL+i);
      if (i != numValues - 1)
	 out << " ";
      else
	 out << ";" << endl;
   }
   out << endl;
}



/*****************************************************************************
  Description : Write out CN2 script file.
  Comments    :
*****************************************************************************/
void display_script_CN2(const MString& datafile,
			const MString& testfile,
			const MString& attrfile,
			MLCOStream& out)
{
   out << "read atts " << attrfile << endl;
   out << "read exs " << datafile << endl;
   out << "induce" << endl;
   out << "read exs " << testfile << endl;
   out << "Xecute all" << endl;
   out << "quit" << endl;
   out << "quit" << endl;
}
   




