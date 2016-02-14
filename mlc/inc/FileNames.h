// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _FileNames_h
#define _FileNames_h 1

class FileNames {
   Bool    suggestTestFile; // TRUE if DATAFILE does not have .all suffix.
   Bool    testFilePrompted; // TRUE if the test file has been prompted
   MString rootName;
   MString dataFile;
   MString namesFile;
   MString testFile;
   MString dumpStem;
public:
   FileNames();
   MString data_file() const; 
   MString root_name() const { return rootName;}
   MString names_file();
   // abort if non-derivable or empty
   MString test_file(Bool abortOnEmpty = TRUE);
   MString dump_stem(); 
};
#endif

