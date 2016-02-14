// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Guarantees that new never returns a NULL pointer.
                 set_new_handler() sets a function that is called when
		   a call to new fails. (See Stroustrup page 99 and Annotated
		   C++ Reference Manual page 280).
		 The reservedMemory is necessary because fatal_error(),
		   which is called by out_of_memory_handler() calls new.
		   The reservedMemory is freed to allow the error
		   message to be printed.  See fatal_error() in error.c
		   for additional details.
  Assumptions  : 
  Comments     : 
  Complexity   :
  Enhancements :
  History      : Richard Long                                      10/13/93
                   Initial revision (.c)
                 Ronny Kohavi and Richard Long                     10/13/93
                   Initial revision (.h)
***************************************************************************/

#include <basics.h>
#include <new.h>
#include <safe_new.h>

RCSID("MLC++, $RCSfile: safe_new.c,v $ $Revision: 1.5 $")

const int reservedMemSize = 1024;
static char* reservedMemory = NULL;  // pointer to memory that may be
				     // needed by a call to fatal_error

/***************************************************************************
  Description : Releases reservedMemory so that it can be used by fatal_error.
                If you get here with the reservedMemory set to NULL,
		  fatal_error() is called with a message "out of
		  memory twice." This may happen if the "new" in
		  fatal_error() fails.
  Comments    : init_safe_new() should be called first.
***************************************************************************/
void out_of_memory_handler()
{
   if (reservedMemory == NULL)
      err << "safe_new.c::out_of_memory_handler: Out of memory twice"
	  << fatal_error;
   delete reservedMemory;
   reservedMemory = NULL;
   putenv("DUMPCORE=no"); // make sure not to dump such a huge core
                          // this usually fills up file systems fast!
   err << "safe_new.c::out_of_memory_handler: Out of memory" << fatal_error;
}


/***************************************************************************
  Description : Sets the new_handler.  Allocates memory that may be needed
                  for fatal_error() calls.
  Comments    : Must be called before out_of_memory_handler().
***************************************************************************/
void init_safe_new()
{
   reservedMemory = new char[reservedMemSize];
   set_new_handler(out_of_memory_handler);
}


/***************************************************************************
  Description : Frees the reservedMemory.
  Comments    :
***************************************************************************/
void end_safe_new()
{
   delete reservedMemory;
   reservedMemory = NULL;
}
