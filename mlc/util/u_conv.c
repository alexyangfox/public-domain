// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a utility for MLC++
// It is not part of the MLC++ library itself, but instead uses the
//   library to provide a useful function.

/***************************************************************************
  Description  : Local encoding conversion of attributes.
  Usage        : Environment varialbes to set:
	    	   DATAFILE    The datafile name.
		   DUMPSTEM    The dumpfile name.
		   CONVERSION  Specify which conversion to perform.
		   
  Enhancements :
  History      : Yeogirl Yun                                        5/30/95
                   Restructured and added aha interface.
                 Robert Allen                                       2/10/95
                   Add handling for other program's file needs.
                 Chia-Hsin Li                                       1/17/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <InstList.h>
#include <mlcIO.h>
#include <FileNames.h>
#include <GetOption.h>
#include <convDisplay.h> 

RCSID("MLC++, $RCSfile: u_conv.c,v $ $Revision: 1.9 $")

static void standard_conversion(Conversion conversion,
				InstanceList& datafile,
				const InstanceList* testfile,
				MLCOStream& outDatafile,
				MLCOStream& outNamefile,
				MLCOStream* outTestfile)
{
   Delimeter attrDelimetor = get_option_enum("ATTR_DELIM", delimeterEnum,
					     defaultAttrDelim,
					     attrDelimeterHelp, TRUE);
   MString attrSeparator = delimeterString(attrDelimetor) + " ";

   Delimeter lastAttrDelimeter = get_option_enum("LAST_ATTR_DELIM",
						 delimeterEnum,
						 defaultLastAttrDelim,
						 lastAttrDelimeterHelp, TRUE);
   MString lastAttrSeparator = delimeterString(lastAttrDelimeter) + " ";

   Delimeter eolCharOpt = get_option_enum("END_OF_LINE_DELIM",
					  delimeterEnum,
					  defaultEolDelim,
					  endOfInstanceHelp, TRUE);
   MString eolChar = delimeterString(eolCharOpt);

   Normalization continuousOption = get_option_enum("NORMALIZATION",
						    normalizeEnum,
						    defaultNorm,
						    normalizeHelp, FALSE);

   if (continuousOption != noNormalization && outTestfile == NULL)
      cerr << "Warning: normalization of whole dataset might be "
	      "incorrect if portions will be used as test set "
	      "because the normalization 'sees' the test instances." << endl;
   
   if (continuousOption == extreme)
      datafile.normalize_bag(InstanceBag::extreme);

   Array2<Real>* continStats = NULL;
   if (continuousOption == normalDist)
      continStats = create_continuous_stats(datafile, continuousOption);
   
   switch(conversion) {
      case local:
	 display_list_rep(datafile, datafile.get_schema(),
			  outDatafile, local, continuousOption,
			  attrSeparator, lastAttrSeparator, eolChar,
			  continStats);
         // make sure to pass the schema from the datafile so that it has the
	 // correct normalization factors (min/max)
         if (testfile) {
	    ASSERT(outTestfile);
	    display_list_rep(*testfile, datafile.get_schema(),
			     *outTestfile, local, continuousOption,
			     attrSeparator, lastAttrSeparator, eolChar,
			     continStats);
	 }
	 display_names_local(datafile.get_schema(),
			     outNamefile, "Local coding conversion");
	 break;

      case binary:
	 display_list_rep(datafile, datafile.get_schema(),
			  outDatafile, binary, continuousOption,
			  attrSeparator, lastAttrSeparator, eolChar,
			  continStats);
         if (testfile) {
	    ASSERT(outTestfile);
	    display_list_rep(*testfile, datafile.get_schema(),
			     *outTestfile, binary, continuousOption,
			     attrSeparator, lastAttrSeparator, eolChar,
			     continStats);
	 }
	 display_names_bin(datafile.get_schema(),
			   outNamefile, "Binary coding conversion");
	 break;

      case noConversion:
	 display_list_rep(datafile, datafile.get_schema(),
			  outDatafile, noConversion,
			  continuousOption, attrSeparator, lastAttrSeparator,
			  eolChar, continStats);
         if (testfile) {
	    ASSERT(outTestfile);
	    display_list_rep(*testfile, datafile.get_schema(),
		     *outTestfile, noConversion,
		     continuousOption, attrSeparator, lastAttrSeparator,
		     eolChar, continStats);
	 }
	 datafile.display_names(outNamefile, TRUE, " No Conversion ");
	 break;
      default:
	 ASSERT(FALSE);
   }
}


static void convert_to_aha(const InstanceList& datafile,
			   const InstanceList* testfile,
			   MLCOStream& outDatafile,
			   MLCOStream& outNamefile,
			   MLCOStream* outTestfile,
			   MLCOStream& outDescripfile)
{
   display_list_rep(datafile, datafile.get_schema(), 
		    outDatafile, noConversion,
		    noNormalization, " ", " ", "", NULL);
   if (testfile) {
      ASSERT(outTestfile);
      display_list_rep(*testfile, datafile.get_schema(),
		       *outTestfile, noConversion,
		       noNormalization, " ", " ", "", NULL);
   }
   display_names_aha(datafile.get_schema(), outNamefile);
   display_description_aha(datafile.get_schema(), outDescripfile);
}

main()
{
   FileNames files;
   // don't want any overwriting:
   if (files.names_file() == files.dump_stem() + ".names")
      err << "u_conv:main(): output file will step on input file.  "
	 "Change DUMPSTEM environment variable" << fatal_error;
   InstanceList datafile("", files.names_file(), files.data_file());
   InstanceList *testfile = NULL;
   
   if(files.test_file(FALSE) != "")
      testfile = new InstanceList("", files.names_file(), files.test_file());

   MString dumpDataFile = files.dump_stem();
   MLCOStream* outTestfile = NULL;
   if (testfile) {
      dumpDataFile += ".data";
      outTestfile = new MLCOStream(files.dump_stem() + ".test");
   }
   else
      dumpDataFile += ".all";
   
   MLCOStream outDatafile(dumpDataFile);
   MLCOStream outNamefile(files.dump_stem() + ".names");

   Conversion conversion = get_option_enum("CONVERSION", conversionEnum,
					   defaultConversion, conversionHelp,
					   FALSE);
   if (conversion == ahaConv) {
      MLCOStream outDescripFile(files.dump_stem() + ".dscr");
      convert_to_aha(datafile, testfile,
		     outDatafile, outNamefile, outTestfile, outDescripFile);
   }
   else 
      standard_conversion(conversion, datafile, testfile,
			  outDatafile, outNamefile, outTestfile);
       
   return 0; // return success to shell
}   





