// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a utility for MLC++
// It is not part of the MLC++ library itself, but instead uses the
//   library to provide a useful function.

/***************************************************************************
  Description  : Run a given accuracy estimator.
  Usage        : 
  Enhancements :
  History      : Dan Sommerfield                                  12/13/94
***************************************************************************/

#include <basics.h>
#include <Inducer.h>
#include <env_inducer.h>
#include <CtrInstList.h>
#include <AccEstDispatch.h>
#include <GetOption.h>
#include <FileNames.h>

RCSID("MLC++, $RCSfile: u_AccEst.c,v $ $Revision: 1.16 $")

main()
{
   // prompt for an inducer
   BaseInducer *inducer = env_inducer();
   inducer->set_log_level(globalLogLevel - 3);

   FileNames files;
   
   // set up an accuracy estimator using options
   AccEstDispatch accEst;
   accEst.set_user_options("");
   
   // train and test the estimator
   CtrInstanceList trainList("", files.names_file(), files.data_file());

   if(files.test_file(FALSE) != "") {
      CtrInstanceList testList("", files.names_file(), files.test_file());
      accEst.estimate_accuracy(*inducer, trainList, testList);
   }
   else
      accEst.estimate_accuracy(*inducer, trainList);

   accEst.display(Mcout);
   delete inducer;
   return 0;
}

   
      
   
