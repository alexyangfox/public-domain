// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a utility for MLC++
// It is not part of the MLC++ library itself, but instead uses the
//   library to provide a useful function.

/***************************************************************************
  Description  : Generate output files for cross validation.
  Usage        : Environment variables to set:
                   DATAFILE
                   NAMESFILE (defaults to basename of DATAFILE + ".names")
                   DUMPSTEM  file stem to dump files (defaults to /tmp/data).
  Enhancements :
  History      : Ronny Kohavi                                       1/22/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <CValidator.h>
#include <CtrInstList.h>
#include <MRandom.h>

RCSID("MLC++, $RCSfile: u_GenCVFiles.c,v $ $Revision: 1.10 $")

main()
{
   Mcout <<"CrossValidator dump executing" << endl;
   MString dataFile = get_env("DATAFILE");

   MString rootname;
   if (dataFile.contains("."))
      rootname = MString(dataFile, dataFile.index("."));
   else {
      rootname = dataFile;
      dataFile += ".data";
   }

   MString namesFile = get_env_default("NAMESFILE", rootname);
   int numFolds = get_env_default("CV_FOLDS", "10").short_value();
   int numTimes = get_env_default("CV_TIMES", "1").short_value();

   if (!namesFile.contains("."))
      namesFile += ".names";

   MString dumpStem  = get_env_default("DUMPSTEM",  "/tmp/data");
   if (dumpStem.contains("."))
      err << "DUMPSTEM=" << dumpStem << " must not contain period" <<
         fatal_error;
   
   GLOBLOG(1, "DATAFILE=" << dataFile
         << "\nNAMESFILE=" << namesFile
         << "\nDUMPSTEM=" << dumpStem
         << "\nCV_FOLDS=" << numFolds
         << "\nCV_TIMES=" << numTimes  << endl);

   InstanceList data("", namesFile, dataFile);
   CrossValidator crossValidator(numFolds, numTimes);
   crossValidator.init_rand_num_gen(7258789);
   crossValidator.dump_files(data, dumpStem);

   return 0; // return success to shell
}   


