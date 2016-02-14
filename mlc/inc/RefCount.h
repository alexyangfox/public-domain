// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : There is no guard for this include file because it may be
                    included multiple times if more than one handle class
		    is declared in the same the file. 
		 A file which includes RefCount.h must define the following:
		    (1) BODY_CLASS is the name of body class being counted and
		    (2) HANDLE_CLASS is the name of handle class doing the
		        reference counting.
		 The handle class must define:
		    (3) "rep" to be a data member pointing to the BODY_CLASS
		        (i.e. "BODY_CLASS *rep;").
		 The body class must define
		    (4) "refCount" to be a data member which counts the
		        number of references to the body class
			(i.e. "int refCount;").
		    (5) A copy constructor of the form:
		        BODY_CLASS(const BODY_CLASS&, CtorDummy);
		 If the handle class is a template class, then the
		    following must be defined: 
                    (6) HANDLE_TEMPLATE is the name of the handle class
		       including any template arguments.
  Comments     : The trace helps locate leaks.  The main problem is that
                   if function A allocates something and B just increments
		   the count, A looks responsible in leak detection.
		   With trace, B will leave a trace behind.
  Enhancements :
  History      : Ronny Kohavi                                      10/20/94
                   Added tracing and safe constructor.
                 Brian Frasca & Richard Long                       3/06/94
                   Initial revision
***************************************************************************/


#ifndef HANDLE_TEMPLATE
#define HANDLE_TEMPLATE HANDLE_CLASS
#endif

private:
    BODY_CLASS* rep;
    DBG_DECLARE(char *trace;)

protected:

/***************************************************************************
  Description : Set representation pointer.  Also sets trace if needed.
                Note that there is a public constructor from the BODY_CLASS
		  that should be used.  This function is a protected
		  version that does not reset the caller's pointer to NULL.
		  It is meant to be used when you have a temporary pointer
		  such as when doing new.
  Comments    : All constructors that set the rep pointer should use
                  this function.
***************************************************************************/

void set_rep(BODY_CLASS *newRep)
{
   rep = newRep;
   DBGSLOW(trace = new char[1]);
}


/***************************************************************************
  Description : Returns a constant pointer to the body class object.
  Comments    : This is public for efficiency.  If you're doing many
                  operations with the instance, it might be better
		  to have one instance keeping the ref count up and
		  all others accessing the body directly.
***************************************************************************/
public:
const BODY_CLASS* read_rep() const
{
   DBG(ASSERT(rep != NULL); ASSERT(rep->refCount > 0));
   return rep;
}
protected:

/***************************************************************************
  Description : Returns a non-constant pointer to the body class object.
		This function makes a new copy of the body class object if
		   there is more than one handle to that object.  It assumes
		   that a copy constructor exists for the body class.  The
		   copy constructor must take a dummy argument (this prevents
		   undesired copying from occurring).
		The returned object is referenced only by one handle and
		   therefore can be modified.
		If TIME_STAMP is defined, then the handle class's "timeStamp"
		   is incremented each time a copy of the body class "rep" is
		   made.
  Comments    :
***************************************************************************/
BODY_CLASS* write_rep()
{
   ASSERT(rep != NULL);
   ASSERT(rep->refCount > 0);
   if (rep->refCount > 1) {
      rep->refCount--;
      rep = new BODY_CLASS(*rep, ctorDummy); 
      rep->refCount = 1;
#ifdef TIME_STAMP
      DBG(timeStamp++); // "timeStamp" is only declared in DBG mode.
#endif
   }
   ASSERT(rep->refCount == 1);
   return rep;
}



// These members provide the reference counting storage management.
// Note that the real copy constructor and assignment operator are used for
//   the handle class (not ones that take a dummy argument).  This is not
//   expensive because reference counting is used (the body class is not
//   actually copied).


public:

/***************************************************************************
  Description : Copy Constructor.
  Comments    :
***************************************************************************/
HANDLE_CLASS(const HANDLE_TEMPLATE& src)
{
   ASSERT(src.rep != NULL);
   ASSERT(src.rep->refCount > 0);
   set_rep(src.rep);
   rep->refCount++;
}

/***************************************************************************
  Description : Constructor from body class.
                Body class must set initialize reference count to 1.
  Comments    :
***************************************************************************/

HANDLE_CLASS(BODY_CLASS*& bodyClass)
{
   DBG(ASSERT(bodyClass != NULL));
   DBG(ASSERT(bodyClass->refCount == 1));
   set_rep(bodyClass);
   DBG(bodyClass = NULL); // don't let anyone use it.
}


/***************************************************************************
  Description : Destructor.
  Comments    :
***************************************************************************/
virtual ~HANDLE_CLASS()
{
   ASSERT(rep != NULL);
   ASSERT(rep->refCount > 0);
   if (--rep->refCount <= 0) {
      delete rep;
      DBG(rep = NULL);
   }
   DBGSLOW(delete trace);
}


/***************************************************************************
  Description : Calling this function will ensure that there is only one
		  handle to the body.  See write_rep for a more complete
		  description.
  Comments    : Make sure the copy you have is unique.  This is useful
                  when you are about to cast away constness and do not
		  want to affect other copies.  Example: normalize
		  in InstanceBag.  This usually indicates a design problem,
		  but sometimes it is needed for efficiency considerations.
***************************************************************************/
void get_unique_copy()
{
   (void) write_rep();
}



/***************************************************************************
  Description : Assignment Operator.
  Comments    : This will work when assigning an object to itself.  The
		  reference count of the object is incremented before
		  checking to determine if the object should be deleted.
***************************************************************************/
HANDLE_TEMPLATE& operator=(const HANDLE_TEMPLATE& src)
{
   ASSERT(rep != NULL);
   ASSERT(rep->refCount > 0);
   ASSERT(src.rep->refCount > 0);
   src.rep->refCount++;
   if (--rep->refCount <= 0)
      delete rep;
   set_rep(src.rep);
   return *this;
}


#undef BODY_CLASS
#undef HANDLE_CLASS
#undef HANDLE_TEMPLATE
#ifdef TIME_STAMP
#undef TIME_STAMP
#endif

// All class members listed after including this file will be private
// unless otherwise specified.
private:
