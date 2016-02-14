// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : External Inducer interfacing CN2 inducer.
  Assumptions  : shell must find the CN2 program in the path.
  Comments     : Every train_and_test() call will write three files.
                   1. attribute file derived from MLC++ names file.
		   2. training example file derived from MLC++ training file.
		   3. test example file derived from MLC++ test file.
		   4. fixed script to run cn2 program.
		   
  Complexity   :
  Enhancements :
  History      : Yeogirl Yun                                   7/15/95
                   Initial revision. 
***************************************************************************/

#include <basics.h>
#include <CN2Inducer.h>
#include <InstList.h>
#include <mlcIO.h>  // for get_temp_file_name
#include <CatTestResult.h>
#include <convDisplay.h>
#include <LogOptions.h>
#include <GetOption.h>
#include <MEnum.h>
#include <CN2Inducer.h>
#include <ConstInducer.h>
#include <string.h>
#include <ctype.h>

const MString PGM_NAME_HELP = "the name of CN2 program";
const MString REDIRECT_STRING_HELP = "You can take either '2>1 >' or '>&'. "
   "if you use sh, then use '2>1 >' otherwise '>&'. These are used to "
   "redirect all the output of OC1 program to get an accuracy.";

MString CN2Inducer::defaultPgmName("cn");
MString CN2Inducer::defaultRedirectString("2>1 >");

/*****************************************************************************
  Description : Parse the output of CN2 program.
                Currently, it finds accuracy line from CN2 program output.
  Comments    :
*****************************************************************************/

static Real parse_output(const MString& fileName, const MString& cmd)
{
   MLCIStream file(fileName);

   if (file.peek() == EOF) {
      Mcerr << "CN2Inducer::parse_output: Empty file " << fileName << endl
          << "Command executed: " << cmd << endl
	  << "Probable cause: CN2 is not installed on this system" << endl;
      return FALSE;
   }

   const MString token = "Overall accuracy:";
   
   MString line;
   MString lastAccuracy;
   while (!file.eof()) {
      line.get_line(file, '\n');
      GLOBLOG(2, line << endl);
      if (line.contains(token)) {
	 lastAccuracy = line;
	 break;
      }
   }

   GLOBLOG(2, "The last accuracy line : " << lastAccuracy << endl);
   if (lastAccuracy == "" )
      err << "Internal Error - Can't parse CN2 program output\n"
	 "See file " << fileName << fatal_error;

   MString subString = lastAccuracy.substring(token.length()+1, 5);
   Real acc  = subString.real_value();
   ASSERT(acc >= 0 && acc <= 100);

   GLOBLOG(2, " The accuracy is " << acc << "%" << endl);   
   return acc / 100; // percent
}



/***************************************************************************
  Description : run CN2 and return accuracy given train and test sets.
  Comments    :
***************************************************************************/
static Real runCN2(const MString& pgmName,
		   const MString& redirectString,
		   const InstanceList& trainList,
		   const InstanceList& testList)
{
   MString fileStem = get_temp_file_name();
   MString attrfile = fileStem + ".att";
   MString datafile = fileStem + ".train.exs";
   MString testfile = fileStem + ".test.exs";   
   MString outfile = fileStem + ".out";
   MString scriptfile = fileStem + ".scr";

   MLCOStream outDatafile(datafile);
   MLCOStream outTestfile(testfile);   
   MLCOStream outOutfile(outfile);
   MLCOStream outAttrfile(attrfile);
   MLCOStream outScriptfile(scriptfile);

   // now create CN2 files.
   display_list_CN2(trainList, outDatafile);
   display_list_CN2(testList, outTestfile);
   display_names_CN2(trainList.get_schema(), outAttrfile);
   display_script_CN2(datafile, testfile, attrfile, outScriptfile);

   outDatafile.close();
   outTestfile.close();   
   outOutfile.close();
   outAttrfile.close();
   outScriptfile.close();

   // now execute CN2.
   // echo at the end used to make it return 0.   
   MString command = pgmName + " < " + scriptfile + " " +
      redirectString + " " + outfile;
   
   // run it.
   if (system(command) != 0)  // this should return 0.
      err <<  "CN2Inducer::train_and_test: fail to run CN2 program as "
	  << command << fatal_error;

   Real acc = parse_output(outfile, command);

   // clean up.
   /*
   remove_file(datafile);
   remove_file(outfile);
   remove_file(attrfile);
   remove_file(testfile);
   remove_file(scriptfile);
   */

   return acc;
}



/*****************************************************************************
  Description : Constructor/Destructor.
                Intialize optoins.
  Comments    :
*****************************************************************************/
CN2Inducer::CN2Inducer(const MString& description, 
		       const MString& thePgmName)
   : BaseInducer(description),
     pgmName(thePgmName)
{
   set_pgm_name(defaultPgmName + " ");
   set_redirect_string(defaultRedirectString);
}


CN2Inducer::~CN2Inducer()
{
}


/*****************************************************************************
  Description : Set the redirect string.
  Comments    :
*****************************************************************************/
void CN2Inducer::set_redirect_string(const MString& val)
{
   if (!val.contains("2>1 >") && !val.contains(">&"))
      err << "CN2Inducer::set_redirect_string : unknown redirect string "
	 "set : " << val << " Use either '2>1 >' or '>&' " << fatal_error;
   redirectString = val;
}


/*****************************************************************************
  Description : Returns a new nominal attribute info with no leading
                  digits.
  Comments    :
*****************************************************************************/
static NominalAttrInfo* nai_no_leading_digit(const NominalAttrInfo& nai)
{
   DblLinkList<MString>* attrVals = new DblLinkList<MString>;
   for (int j = 0; j < nai.num_values(); j++) {
      MString val(nai.get_value(j));
      if (!isalpha(val[0]))
         val = "X" + val;
      for (int i = 0; i < val.length(); i++)
	 if (!isalnum(val[i]))
	    val.set_char(i, 'x');
      attrVals->append(val);
   }
   NominalAttrInfo* newNai;
   newNai = new NominalAttrInfo(nai.name(), attrVals);
   ASSERT(attrVals == NULL);
   return newNai;
}


/*****************************************************************************
  Description : Set a new schema that has no leading digits in nominal
                  attribute values.
  Comments    :
*****************************************************************************/
static void set_new_schema(InstanceList& instList)
{
   const SchemaRC& schema = instList.get_schema();
   DblLinkList<AttrInfoPtr>* attrInfos = new DblLinkList<AttrInfoPtr>;
   for (int i = 0; i < schema.num_attr(); i++) {
      if (schema.attr_info(i).can_cast_to_nominal()) {
	 attrInfos->append(nai_no_leading_digit(
	    schema.attr_info(i).cast_to_nominal()));
      }
      else 
	 attrInfos->append(schema.attr_info(i).clone());
   }

   AttrInfo* nai =
      nai_no_leading_digit(schema.label_info().cast_to_nominal());

   SchemaRC newSchema(attrInfos, nai);
   instList.set_schema(newSchema);
}	 
	 

/***************************************************************************
  Description : Train and test by running CN2's CN2 program.
  Comments    :
***************************************************************************/
Real CN2Inducer::train_and_test(InstanceBag* trainingSet,
 			        const InstanceBag& testBag)
{
   // we don't want to take ownership of the given InstanceBag, so
   // we add instances instead of using InstanceList constructor.
   InstanceList trainList(trainingSet->get_schema());
   for (Pix pix = trainingSet->first(); pix; trainingSet->next(pix))
      trainList.add_instance(trainingSet->get_instance(pix));

   set_new_schema(trainList);
   InstanceList testList(testBag.get_schema());
   for (pix = testBag.first(); pix; testBag.next(pix))
      testList.add_instance(testBag.get_instance(pix));

   set_new_schema(testList);
   if (trainList.get_schema().num_attr() == 0) {
      // since CN2 do not handle 0-attribute instances, we run const
      // inducer here for completeness. Esp. this is necesary for FSS.
      LOG(1, " 0-attribute instances detected. Running const inducer."
	  << endl);
      ConstInducer constInd("const inducer");
      return constInd.train_and_test(&trainList, testList);
   }      

   return runCN2(get_pgm_name(),
		 get_redirect_string(),
		 trainList,
		 testList);
}


/*****************************************************************************
  Description : Set user options.
  Comments    :
*****************************************************************************/
void CN2Inducer::set_user_options(const MString& preFix)
{
   set_pgm_name(get_option_string(preFix + "CN2_PGM_NAME",
				  defaultPgmName,
				  PGM_NAME_HELP,
				  TRUE));
   set_redirect_string(
      get_option_string(preFix + "CN2_REDIRECT_STRING",
			defaultRedirectString,
			REDIRECT_STRING_HELP,
			FALSE));
}





