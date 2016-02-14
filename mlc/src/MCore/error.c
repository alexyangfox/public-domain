// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Provide error handling capabilities.
                 Usage is suggested as follows (fatal_error is an I/O
                   manipulator like flush):
                   err << "Class::member:" << info << ... << fatal_error;
                 fatal_unless() provides minimal error catching capability.
                 A period is appended at the end of message.

                 is_valid_pointer() checks if a given address is valid as
                    the address of a pointer.
  Assumptions  : When we get to fatal_error, the err stream should contain
                   the message.
  Comments     : User may redefine "fatal_error" to catch errors.
                 See "errorUnless.h" for some error catching ability.
                 fatal_abort() can be called to achieve the same
                   fatal_error behavior if fatal_error is overridden.

                 The basic version provided here allows catching one error
                    and executing a longjmp().  It is mainly used in tester
                    routines.  The longjmp() returns with value 1.
                 An error is caught if the global variable fatal_expected
                   of type (char *) is contained in the actual error.
                   If this is the case, a longjmp is executed to backFromFatal.
                 Note that memory allocated between the setjmp and the
                    fatal error may be lost (if there are no pointers to
                    it).
                 The string fatal_expected is cleared (NULL) to avoid
                   infinite loops.   The err stream is also cleared.
  Complexity   : 
  Enhancements : Provide a way of introducing warnings, perhaps
                   suppressable, provide separate error handling for
                   different classes.  The list is endless, but these
                   enhancements should wait for implementations of
                   exceptions in C++ compilers which will change everything.
                 Currently the error is just written to cerr.  We may
                   want to save it to a file too.  This is especially
                   true if for some reason cerr is corrupted (in which
                   case we are called to report the error...)
  History      : Ronny Kohavi                                       7/13/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <MLCStream.h>
#include <errorUnless.h>
#include <signal.h>

RCSID("MLC++, $RCSfile: error.c,v $ $Revision: 1.32 $")
jmp_buf backFromFatal;
static Bool loop = FALSE;  // to detect loops in fatal_error if the
			   // free store is exhausted

MLCOStream& fatal_error(MLCOStream& err2) // this should be err
{
   if (loop) {
      cerr << "error.c::fatal_error: fatal_error loop" << endl;
      // The for loop is used because it is possible that two messages
      // are in err_text.  If "cerr << err_text" were used, then the
      // second error message would not be used.  The first message
      // was terminated with '\0' by the first call to fatal_error,
      // but that character is unreadable, so it is replaced by a
      // newline if the loop is encountered.
      for (int i = 0; i < err.get_strstream().pcount(); i++)
	 err_text[i] ? cerr << err_text[i] : cerr << endl;
      cerr << '.' << endl;
      abort();
   }
   loop = TRUE;  // in case there is an error in the new
   
   // Put period and End-Of-MString because we are reading the char[] buffer.
   // Note that "." does NOT append an EOS so we must force it.
   err << '.' << '\0';
   if (fatal_expected != "") {

      // a pointer so we can free it before the longjmp()
      MString *errmsg = new MString((const char *)err_text);

      if (errmsg->contains(fatal_expected)) {
         delete errmsg;
         fatal_expected = "";
         err.get_strstream().seekp(0); // reset stream
	 loop = FALSE; // about to leave fatal_error
         longjmp(backFromFatal, 1); // value returned from longjmp is 1
         ASSERT(FALSE);
      }
   }
   fatal_abort(err2.get_strstream()); // now abort.
   ASSERT(FALSE); // should never get here
   return err; 
}

/***************************************************************************
  Description : Check if a given address is valid as the address of a
                  pointer.
  Comments    : We check sizeof(pointer) bytes in that address.
***************************************************************************/

jmp_buf jumpOnInvalid;
void *indirectPtr; // has to be extern or the compiler optimizes it out
                   //   (happened on SGI CC 4.0 (NCC) for t_error in
		   //   DEBUGLEVEL 2).

#if defined(CFRONT)
#define signalArg int
#else 
#define signalArg ...
// Don't need an else unsupported compiler because this will generate
// a syntax error.
#endif

static void is_valid_pointer_signal_handler(signalArg)

{
   longjmp(jumpOnInvalid,1);
}

#if defined(__CENTERLINE__)
   extern "C" int centerline_ignore(char *);
   extern "C" int centerline_catch(char *);
#endif

Bool is_valid_pointer(void *ptr, Bool fatalOnFalse)
{
   if (ptr == NULL)
      err << "error::is_valid_pointer: NULL pointer" << fatal_error;
   
   // Save bus error signal and segmentation violation

   void (*busFunc)(signalArg)  = signal(SIGBUS, 
					&is_valid_pointer_signal_handler);
   void (*segvFunc)(signalArg) = signal(SIGSEGV,
					&is_valid_pointer_signal_handler);
   
#  if defined(__CENTERLINE__)
   centerline_ignore("SIGBUS");
   centerline_ignore("SIGSEGV");
#  endif

   Bool valid = FALSE;

   if (setjmp(jumpOnInvalid) == 0) {
      indirectPtr = *(int **)ptr; // check pointer access
      valid = TRUE;
   }


#  if defined(__CENTERLINE__)
   ASSERT(centerline_catch("SIGBUS") == 0);
   ASSERT(centerline_catch("SIGSEGV") == 0);
#  endif

   // restore signals
   (void)signal(SIGBUS, busFunc);
   (void)signal(SIGSEGV, segvFunc);

   if (!valid && fatalOnFalse)
      err << "error::is_valid_pointer(): pointer (" << ptr << ") is invalid"
          << fatal_error;
   return valid;
}
