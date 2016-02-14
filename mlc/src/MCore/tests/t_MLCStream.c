// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests MLCIStream and MLCOStream.
                 Depends on files:
		   {t_MLCStream.cin,t_MLCStream.read1,t_MLCStream.read2,\
		    t_MLCStream.read3}
		 Creates files:
		   {t_MLCStream.out1,t_MLCStream.out2,t_MLCStream.out3\
		    t_MLCStream.out4,t_MLCStream.out5,t_MLCStream.out6\
		    t_MLCStream.out7}

  Doesn't test : 
  Enhancements :
  History      :  James Dougherty                                   10/15/94
                   Added tests for enhanced streams. These include wrapping,
		    manipulator, line position and line_count, binary write
		    and read, and strstream-based MLCOStream. 
		   
                  Richard Long                                       9/02/93
                   Initial revision


***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <MLCStream.h>
#include <checkstream.h>
#include <string.h>
#include <math.h>          // for fabs()
#include <Array.h>
#include <mlcIO.h>

RCSID("MLC++, $RCSfile: t_MLCStream.c,v $ $Revision: 1.44 $")

char* mlcPath = NULL; // Note: In Global scope since memory pointed to
                      //         by this pointer becomes part of the env. 




/***************************************************************************
  Description : Tests nine lines of embedded \n in a string and checks that
                  the line number is in fact correct.
  Comments    : Location independent test (can be called anywhere).
***************************************************************************/
void test_embedded_newlines()
{
   Mcout.set_width(0);
   Mcout << "Testing embedded newlines ... " << endl;
   Mcout.set_width(10);
   Mcout.set_wrap_prefix("<WP>");
   Mcout.set_newline_prefix("<NLP>");
   int currentLineNumber = Mcout.line_count();
   Mcout << "Char with a newline\n there and \n there ..." ;
   ASSERT(Mcout.line_count() - currentLineNumber == 9);
   Mcout.set_width(0);
   Mcout << endl<< "Check .. " << endl;
}


/***************************************************************************
  Description : Test opening and using various MLCstreams.
  Comments    : These are mostly tests from the first revision of
                  t_MLCStream.c
***************************************************************************/
void basic_tests()
{
#ifdef ATTACHBUG
// @@ OC bug causes attached stream to be closed.
// To: ronnyk@cs.stanford.edu
// Cc: support-log@centerline.com
// Subject: Re: Attach problem
// Date: Wed, 16 Nov 94 16:54:02 PST
// From: Ron Levasseur <ron@centerline.com>


// | The program below compiles in OC 2.0.4 and outputs foo, then bar, but
// | on the new OC 2.1, it outputs only foo.
// | 
// | Was there a change in the definition of "close" so that it closes
// | the original stream that we attach to?
// | 
// | 
// | ______________________________________________________________________
// | 
// | #include <fstream.h> 
// | main()
// | {
// |    ofstream x;
// |    x.attach(1);
// |    x << "foo" << endl;
// |    x.close();
// | 
// |    cout << "bar" << endl;
// | }
// | 

// Hi Ronny,

// You are right...fstream::close is broken.  It should know not
// to close the underlying file descriptor.

// According to the AT&T C++ Library Manual, p. 3-12:
// A subtle point is that closing a file stream (either explicitly
// or implicitly in the destructor) will close the underlying file
// descriptor if it was opened with a filename, but not if it was
// supplied with attach();

// I tried your test case with vanilla cfront 3.0.2, 3.0.2.15,
// and 3.0.3 beta, and found that they all acted the same as
// CC from OCtr 2.1.  So, my theory is that this bug is something
// we've been fixing manually in our releases before shipping
// our CC.  There was extra work done on libC for this release,
// and it looks like that patch got left out this time around.
// I'll see what we can do about getting a patch. 

// For now, I imagine you could just unarchive fstream.o from libC
// (from OCtr 2.0.4) and link it explicitly on the CC 2.1 command
// line.  Talk to you later,

// Ron


  MLCOStream *mlcout2 = new MLCOStream(1, "standard output 2");
  mlcout2->setbuf(NULL, 0);

#endif
  
  MLCIStream inFile("t_MLCStream.read1");
  MString string;
  inFile >> string;
  ASSERT(string == "test_read");
  inFile.ignore();
  ASSERT(inFile.peek() == 'a');
  inFile.ignore();
  ASSERT(inFile.get() == 'b');
  Mcout << "Please type in 'test1': \n";

  Mcin.setbuf(NULL,0);
  Mcin >> string;
  ASSERT(string == "test1");

#ifdef ATTACHBUG
  *mlcout2 << "Please type in 'test2': \n";
  MLCIStream* mlcin2 = new MLCIStream(0, "standard input 2");
  mlcin2->setbuf(NULL,0);
  *mlcin2 >> string;
  ASSERT(string == "test2");
  delete mlcout2;
  delete mlcin2;
#else
  Mcout << "Please type in 'test2': \n";
  Mcin >> string;
  ASSERT(string == "test2");
#endif
  
  #ifndef MEMCHECK
  TEST_ERROR("Xstream is invalid", MLCOStream mo(1,MString("x"),XStream));
  ofstream fooBar;
  TEST_ERROR("Xstream is invalid", MLCOStream m("x",fooBar,XStream));
  #endif
  

  // check that cout and cin still work
  cout << "Please type in 'test3': " << endl;
  check_ostream(cout, "cout");
  Mcin >> string;
  ASSERT(string == "test3");

  MLCOStream* outFile1 = new MLCOStream("t_MLCStream.out1");
  *outFile1 << "t_MLCStream.out1 line 1" << endl;

  delete outFile1;
  outFile1 = new MLCOStream("t_MLCStream.out1", FileStream, TRUE);
  *outFile1 << "t_MLCStream.out1 line 2" << endl;

  delete outFile1;

  MLCOStream outFile2("t_MLCStream.out2");
  // make sure old macro still works
  outFile2 << "t_MLCStream.out2 line 1" << endl;

  Mcerr.setbuf(NULL,0);
  Mcerr << "Should go to cerr (1)" << endl;

#ifdef ATTACHBUG
  MLCOStream mlcerr2 (2, "error 2");
  mlcerr2 << "Should go to cerr (2)" << endl;
#else
  Mcerr << "Should go to cerr (2)" << endl;
#endif
  
  MLCOStream outFile4("t_MLCStream.out3");
  outFile4.include_file("t_MLCStream.out2");

  MLCOStream outFile5("t_MLCStream.out4");
  outFile5 << "Some textw et 3456345tlhreiugh,fkjhtgo371y485y34o " << endl;
  outFile5.close();
  #ifndef MEMCHECK
  TEST_ERROR("attempted operation", outFile5 << "Should crash ..." << endl);
  #endif
  
  MLCOStream outFile6("t_MLCStream.out4");
  for(int j=0; j <320; j++)
     outFile6.write_bin(j);
  outFile6.close();
     
  ifstream inFile6("t_MLCStream.out4");
  MLCIStream inFile7("t_MLCStream.out4",inFile6);
  char c;
  inFile7.get(c);
  
  inFile7.close();
  inFile6.close();

  
#ifdef ATTACHBUG
  ofstream outFile8("t_MLCStream.out5");
  MLCOStream outFile9("t_MLCStream.out5",outFile8);
  outFile8.close();
  outFile9.close();
#endif
}


/***************************************************************************
  Description : Test for file errors.
  Comments    : These are mostly tests from the first revision of
                  t_MLCStream.c
***************************************************************************/
void test_errors()
{
  #ifndef MEMCHECK
  TEST_ERROR("MLCOStream::MLCOStream(const MString&, const Bool, int): Could "
	     "not open file /MLCDOESNOTEXISTDIR/test",
	     MLCOStream* foo = new MLCOStream("/MLCDOESNOTEXISTDIR/test"));
  MLCIStream* eofStream = new MLCIStream("t_MLCStream.eof");
  TEST_ERROR("MLCIStream::get: Tried to get EOF in stream t_MLCStream.eof",
	     eofStream->get());

  MLCIStream bar("t_MLCStream.data");
  bar.close();
  TEST_ERROR("attempted operation on a closed stream",
	     bar.check_stream_is_valid());

  TEST_ERROR("Unknown istream", MLCIStream mi(Mcin.get_stream()));
  TEST_ERROR("Xstream is invalid", MLCOStream dork("data",XStream,TRUE));
  TEST_ERROR("Xstream is invalid", MLCOStream dork(Mcout.get_stream(),XStream));
  TEST_ERROR("Unknown ostream", MLCOStream mo(Mcout.get_stream(), FileStream));
  #endif
}

/***************************************************************************
  Description : Tests the manipulators.
  Comments    : This code is mainly from the second version of the
                 stream class.
***************************************************************************/
void manip_tests()
{
   Mcout << "Testing stream manipulators and input..." << endl;
   int i;
   char c;
   float f;
   Real d;
   unsigned int ui;
   unsigned char uc;
   unsigned long ul;
   unsigned short us;

   MString str="foo-str";
   Mcout << str <<endl;

   Mcout << "foo" << endl;
   Mcout << 5.1234567 << endl;
   Mcout << setprecision(3) << 5.1234567 << endl;
   Mcin >> setw(3) >> i;
   ASSERT( 1 == i);  //reads an int in from t_MLCStream.cin
   Mcin >> c;
   ASSERT('2' == c);   //reads a char in from t_MLCStream.cin
   Mcin >> f;
   ASSERT(fabs(3.141591727 - f) <= 0.0000001);//float accuracy 6 places @starry
   Mcin >> d;
   Mcout << setprecision(10) << d << endl;
   ASSERT(fabs(7.777777777 - d) <= REAL_EPSILON);
   Mcin >> us;
   ASSERT(5 == us);
   Mcin >> ul;
   ASSERT(400 == ul);
   Mcin >> ui;
   ASSERT(53 == ui);
   Mcin >> uc;
   ASSERT(0x80 == uc);
   MString testStr;
   Mcin >> testStr;
   ASSERT("This_is_the_last_line_read_from_Mcin-#11" == testStr);
   ASSERT(11 == Mcin.line_count());
   Mcout << Mcin.line_count() <<" Passed." << endl;
}

/***************************************************************************
  Description : Test for multiple path name.
  Comments    :
***************************************************************************/
void path_tests()
{
   MString oldPath(MLCPATH + "=" + get_env_default(MLCPATH,"."));
   
   MLCIStream a("t_MLCStream.read1");
   MString path(MLCPATH + "=" + "/foo:" + get_env_default(MLCPATH, "."));
   //putenv must get a non-const char * which becomes part of the environment.
   delete mlcPath;
   
   mlcPath = new char[path.length() + 1];
   strcpy(mlcPath, path);
   ASSERT(putenv(mlcPath) == 0);
   MLCIStream b("t_MLCStream.read1");
   path = MLCPATH + "=" + get_env_default(MLCPATH, ".") + "::";
   delete mlcPath;

   mlcPath = new char[path.length() + 1];
   strcpy(mlcPath, path);
   ASSERT(putenv(mlcPath) == 0);

   path = MLCPATH + "= /foo:";
   delete mlcPath;

   mlcPath = new char[path.length() + 1];
   strcpy(mlcPath, path);
   ASSERT(putenv(mlcPath) == 0);
   
   delete mlcPath;

   mlcPath = new char[oldPath.length() + 1];
   strcpy(mlcPath,oldPath);
   ASSERT(putenv(mlcPath) == 0);
}
  
/***************************************************************************
  Description :  Tests the binary output/input methods read_bin/write_bin
  Comments    :
***************************************************************************/
void test_bin_io(void)
{
   int i;
   Mcout << "Testing binary IO ... " << endl;
   const int size =10;
   const Real pi_4 = 3.141591727 / 4;

   Real  rValues[size],rv;
   char  cValues[size],cv;
   int   iValues[size],iv;
   float fValues[size],fv;
   short sValues[size],sv;
   long  lValues[size],lv;
   
   for(i = 0; i < size; i++){
      rValues[i] = fValues[i] = i*pi_4;
      cValues[i] = iValues[i] = sValues[i] = 2*i;
      lValues[i] = 100*i;
   }
   
   MLCOStream foo("t_MLCStream.out6");
   for(i  = 0; i < size; i++){
      foo.write_bin(rValues[i]);
      foo.write_bin(cValues[i]);
      foo.write_bin(iValues[i]);
      foo.write_bin(fValues[i]);
      foo.write_bin(sValues[i]);
      foo.write_bin(lValues[i]);
   }
   foo.close();

   MLCIStream bar("t_MLCStream.out6");
   for(i = 0; i < size; i++){
      bar.read_bin(rv);
      ASSERT(rv == rValues[i]);
      bar.read_bin(cv);
      ASSERT(cv == cValues[i]);
      bar.read_bin(iv);
      ASSERT(iv == iValues[i]);
      bar.read_bin(fv);
      ASSERT(fv == fValues[i]);
      bar.read_bin(sv);
      ASSERT(sv == sValues[i]);
      bar.read_bin(lv);
      ASSERT(lv == lValues[i]);
   }
   Mcout << "Check..." << endl;
   bar.close();
}

/***************************************************************************
  Description :  Tests the output wrapping feature of the streams.
  Comments    :
***************************************************************************/
void test_wrapping()
{
   #ifndef MEMCHECK
   Mcout.set_width(0);
   TEST_ERROR("wrapping is not on", Mcout.pos_in_line());
   #endif
   
   Mcout.set_width(80);
   Mcout.set_wrap_prefix("");
   Mcout.set_newline_prefix("");
   MStreamOptions defaultOptions = Mcout.get_options();

   Mcout << "test four point five " << 4.5 << " three " << 3 << endl;
   
   Mcout << "\n" << "foo" << endl;
   Mcout << "C4.5 run yields accuracy " << .97 << " (pruned), "
         << .95 << " (unpruned)\n"
         << "     size " << 10 << " (pruned), "
         << 15 << " (unpruned)" << endl;
   
   Mcout << "Testing Line Position " << endl;
   Mcout << "ABCDE" << flush;
   ASSERT(6 == Mcout.pos_in_line());
   Mcout << endl << "Check ..." << endl;
	      
   Mcout.set_wrap_prefix("<WP>");
   Mcout.set_newline_prefix("x");
   
   Mcout << endl << endl;
   Mcout << "foo" << endl << endl;
   Mcout << "foo";
   
   Mcout.set_newline_prefix("y");
   Mcout << endl << "bar" << endl;
   Mcout << setprecision(5);
   
   Mcout.set_newline_prefix("z");
   Mcout << 3 << endl << setprecision(3) << endl;

   Mcout.set_width(10);
   Mcout << "<0000000000>" << endl << "<0000000000>" << "foo" << endl;
   
   Mcout.set_width(12);
   Mcout << "This is a real wrapping test to see how this works ..." << endl;

   Mcout.set_width(80);
   Mcout.set_wrap_prefix("");
   Mcout.set_newline_prefix("");
   Mcout << "Reset MLC++ stream options to defaults ..." << endl;
   
   #ifndef MEMCHECK
   Mcout.set_width(3);
   TEST_ERROR("now has invalid wrap options",
	      Mcout.set_wrap_prefix("this should crash"));
   TEST_ERROR("now has invalid wrap options",
	      Mcout.set_newline_prefix("This should as well"));
   #endif

   Mcout.set_width(80);
   Mcout.set_wrap_prefix("");
   Mcout.set_newline_prefix("");

   Mcout << "DATAFILE=" << "foo" << "\nNAMESFILE=" << "bar" << endl;
}

void test_nl()
{
   Mcout.set_newline_prefix("t_MLCStream.c:: test_nl: " );
   Mcout << "test\n\n\ntest" << endl;
   Mcout.set_newline_prefix("");
}

// test overflow of formatStreamBuf
void test_overflow()
{
   Array<char> a(0, 10000, 'A'); // probably won't be as big
   Mcout << a.get_elements() << endl;
}



/***************************************************************************
  Description :  Tests normal stream output.
  Comments    :  This test is done after the wrapping test to see if there
                   are any adverse effects of calling get_options, modifying
		   them, and then performing plain IO.
***************************************************************************/
void test_generic()
{
   Mcout.set_width(0);

//test that our input file line-counting is working
   char c;
   int lc = 1;
   MLCIStream fileStream("t_MLCStream.read2");
   while(fileStream.get_fstream().good()){
      c = fileStream.get_fstream().get();

      if(c == '\n'){
	 lc++;
	 ASSERT(lc == fileStream.line_count());
      }
   }
   fileStream.get_stream().clear();
   fileStream.close();
   Mcout << "Check..." << endl;
   MString foo;
   MLCIStream lineTest("t_MLCStream.read3");
   while(lineTest.get_fstream().good()){
      lineTest >> foo;
      Mcout << foo << ": " << lineTest.line_count();
      Mcout << "position: " << lineTest.pos_in_line() << endl;
      ASSERT(15 == lineTest.pos_in_line() && 1 == lineTest.line_count());
      lineTest.skip_white();
   }
   // You must clear your stream before closing it if using good.
   lineTest.get_stream().clear();
   lineTest.close(); 
   Mcout << "Check..." << endl;
}

/***************************************************************************
  Description : Tests that the destructor works correctly if the stream
                  is an XStream.
  Comments    : 
***************************************************************************/
void test_XStream()
{
   MLCOStream xStream(XStream);
   xStream.OK();
}

/***************************************************************************
  Description : Tests that the tie statement in basics.c works correctly.
  Comments    :
***************************************************************************/
void test_tie()
{
   int u;
   Mcout << "Enter an integer : " ; //You should see this first ...
   Mcin  >> u;
   Mcout << "You entered " << u << endl;
}


/***************************************************************************
  Description : Tests an MStream used as an strstream.
  Comments    : An input strstream is currently unsupported.
***************************************************************************/
void test_string_stream()
{
   Mcout << "Testing string stream ..." << endl;
   char buffer[10000];
   //test a basic MemStream
   
   MLCOStream strStream("strStream",buffer,10000);
   for(int i =0; i < 100; i++)
      strStream << i % 10;
   
   MString testString = strStream.mem_buf();
   ASSERT(100 == testString.length());
   Mcout << "Check..." << endl;
   #ifdef OBJECT_CENTER
   Mcout << strStream.mem_buf() << endl;
   #endif

   //Now test error conditions
   #ifndef MEMCHECK
   MLCOStream f("t_MLCStream.out7");
   TEST_ERROR("undefined operation", f.mem_buf());
   f.close();
   TEST_ERROR("attempted operation", f.check_stream_is_valid());
   TEST_ERROR("undefined operation",strStream.close());
   TEST_ERROR("strStream has not been set",f.get_strstream());
   #endif   
}

/***************************************************************************
  Description : Tests that the lines are counting when wrapping is on.
  Comments    : Note that the first string does not affect the line pos
                  since wrapping is not on yet.
***************************************************************************/
void newline_tests()
{
   Mcout << "Testing linecounting and line position accuracy ... " << endl;
   Mcout.set_width(5); //wrapping is now on
   Mcout.set_newline_prefix("<NL>");
   Mcout.set_wrap_prefix("<WP>");
   int currentLine = Mcout.line_count();
   for(int i=0; i < 5; i++) {
      Mcout << Mcout.line_count() << ' ';
      ASSERT( Mcout.pos_in_line() == 5);
      ASSERT( Mcout.line_count() == ++currentLine);
   }
   Mcout << endl;   
   Mcout.set_width(0);      
}


/***************************************************************************
  Description : Tests everything else that wasn't tested in the tester
                  to bring the code coverage up.
  Comments    :
***************************************************************************/
void misc_tests()
{
#ifndef MEMCHECK
   ifstream foo("/u/mlc/src/james.todo");
   TEST_ERROR("Empty string is not a valid", MLCIStream badDescript("",foo));
#endif
   MLCOStream foo2("/tmp/t_MLCStream.blah");
   for(int i =0; i <10; i++)
      foo2 << "Yuccch.." << endl;
   foo2.close();
   MLCIStream foo3("/tmp/t_MLCStream.blah");
   remove_file("/tmp/t_MLCStream.blah");

   MString oldPath = "MLCPATH=" + get_env("MLCPATH");
   putenv("MLCPATH= :::");
#ifndef MEMCHECK
   TEST_ERROR("and contains ::", MLCIStream foo4("foo"));
#endif
   putenv((char*) (const char*)oldPath);
}

/***************************************************************************
  Description : Tests that the second string will appear at the leftmost
                  position once reset_pos_in_line is called and the string
		  has been input afterwards.
  Comments    : In the tester output, there will be a space or two after the
                  output of the ? but this is ok since we have tested this
		  separately.
***************************************************************************/
void test_reset_line_pos()
{
   MString foo;
   Mcout << "testing testing testing testing testing testing testing testing "
            "string? " << reset_pos_in_line;
   Mcin >> foo;
   Mcout << "testing testing testing testing testing testing testing testing "
         << endl;
}


/***************************************************************************
  Description : test get_stream.  We must make sure that manipulators on
                  get_stream work properly.  This is not trivial because
		  we alternate between formatStream and the real underlying
		  stream depending on the wrapping status.
  Comments    :
***************************************************************************/

void test_get_stream()
{
   Mcout.get_stream().precision(4);
   Mcout.get_stream().setf(ios::fixed); 
   Mcout << 1.2 << ", " << 1.5555 << endl;
   Mcout << 1.255 << ", " << 1.5555 << endl;
   Mcout << 1e-17 << ", " << 1.5555 << endl;
   Mcout << 1.0 << ", " << 0.0 << endl;

   MLCOStream prec("t_MLCStream.out5");
   prec.get_stream().precision(4);
   prec.get_stream().setf(ios::fixed); 
   prec << 1.2 << ", " << 1.5555 << endl;
   prec << 1.255 << ", " << 1.5555 << endl;
   prec << 1e-17 << ", " << 1.5555 << endl;
   prec << 1.0 << ", " << 0.0 << endl;
}
   

int main()
{
   if (globalLogLevel > 0)
      err << "Log level must be 0 for this tester, or setbuf  "
	   << " will be executed after some output has happened"
	   << fatal_error; 
   // Warning: setbuf must be called BEFORE any output is done to Mcout.
   Mcout.setbuf(NULL, 0);
   Mcout.set_width(0);
   Mcout << "t_MLCStream executing ..." << endl;
   newline_tests();
   test_embedded_newlines();
   basic_tests();
   path_tests();
   manip_tests();
   test_bin_io();
   test_wrapping();
   test_nl();
   test_overflow();
   test_generic();
   test_string_stream();
   test_XStream();
   Mcout.set_width(80);
   test_reset_line_pos();
   test_get_stream();
   Mcout << "Testing complete..." << endl;
   Mcout << "Testing complete..."; //Note -no newline

   delete mlcPath; //No longer needed.
   return 0;
}

