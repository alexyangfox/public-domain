// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Generates random numbers that are really random--the lsb is
		   usable. Each instance of MRandom class has its own state.
		   Therefore two instances of MRandom, initialized
		   with the same seed, produce exactly the same sequence
		   of numbers regardless of any interleaving between
		   the calls to them. Also calls to MRandom will not
		   affect anybody else using random().
		                  
  Assumptions  : There exists a default state, so the first call to
                   initstate will return a pointer to it.
		 random() is assumed to be a good random number generator.
  Comments     : 
  Complexity   :
  Enhancements : Add other tests for randomness.  Information can be
                   found in:
                   "Random Number Generators: Good Ones Are Hard To Find",
                    Park & Miller, Communications of the ACM, Oct. 1988, 
                    pp. 1192-1201.  And the Technical Correspondence on this,
                    Communications of the ACM, July 1993, pp. 105-110.
                    Source code in C for this generator can be found
                    in "Data Structures and Algorithm Analysis in C" by
                    Mark Allen Weiss, copyright 1993 Benjamin/Cummings.
                    Note that this code is written so as to avoid overflow
		    due to multiplication - this problem and its solution
		    (the Schrage procedure) is discussed in the first
		    reference.                               Ronnyk

  History      : Svetlozar Nestorov                          12/19/93
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <random.h> // Definitions for random.  Solaris doesn't have these.
#include <MRandom.h>
#include <sys/types.h>     
#include <sys/time.h>
#include <unistd.h> // needed for getpid on Solaris
#include <math.h>

RCSID("MLC++, $RCSfile: MRandom.c,v $ $Revision: 1.7 $")

/***************************************************************************
  Description : Check that MRandom was initialized
  Comments    :
***************************************************************************/

void MRandom::check_init()
{
   if (initialized == FALSE)
      err << "MRandom.c: check_init: MRandom was never seeded.  Use "
	     " a seed of zero for a random seed" << fatal_error;
}



/***************************************************************************
  Description : Returns a random number in the range of 0 to LONG_MAX.
  Comments    : We restore the previous state after we get a random
                  number, using the state of the object, for the
		  following reasons. First, there is a bug in
		  setstate() because when it is called with the
		  current state it does not function properly.
		  Second, we want to make sure that somebody outside
		  MRandom can use random() and will not be affected by
		  MRandom.
		Since this is a static function it does not have
		  access to the members of MRandom so the objects
		  calling it pass the state as a parameter.
***************************************************************************/
static unsigned long MLC_random(long *state)
{
   char *oldState = Berkeley_setstate((char *)state);  // get previous state and set
				              // state to be the newstate
   if (oldState == (char *)state)
      err << "MRandom.c::MLC_random: The current state is passed as"
	     " a new state. This is not allowed because of the bug in"
	     " setstate()" << fatal_error;
   unsigned long result = Berkeley_random();   
   Berkeley_setstate(oldState);                // restore the previous state
   return result;
}
   

/***************************************************************************
  Description : Constructors. A seed of zero gives a "random" seed.
                Although we supply a no-argument MRandom, this is mainly
		  for creating arrays of MRandom.  No operation is allowed
		  unless the generator hsa been seeded.  This is to
		  avoid mistakes of unintended unreplicable results.
		  You can always seed with zero to get a "random" seed.
  Comments    : The arbitrary value for the seed is taken from the
                  current time. 
***************************************************************************/
MRandom::MRandom()
{
   initialized = FALSE;
}

MRandom::MRandom(unsigned seed)
{
   initialized = TRUE;
   
   if (seed == 0)
      init((unsigned int)time(0) + (int)getpid());      
   else
      init(seed);
}


/***************************************************************************
  Description : Initializes MRandom with the new seed.
  Comments    : Since initstate actually sets the currently used state
                  to be the state passed in we have to restore the old
		  state because somebody else outside MRandom might
		  call random(). 
***************************************************************************/
void MRandom::init(unsigned seed)
{
   initialized = TRUE;
   char *oldState = Berkeley_initstate(seed, (char *)state, STATE_LEN);
   if (oldState == (char *)state)
      err << "MRandom::init: The current state is initialized as"
	     " a new state. This is not allowed because of the bug in"
	     " setstate()" << fatal_error;
   Berkeley_setstate(oldState);
}


/***************************************************************************
  Description : Two functions that return a random long integer and a
                  random integer in the range from low to high,
                  uniformly distributed.
  Comments    : High should be greater or equal than low.
                If high and low are equal high is returned without
		  using the state.
***************************************************************************/
long MRandom::long_int(long low, long high)
{
   check_init(); // do not put in DBG.  It will core dump if there was no
		 // initialization (on SGI)

   if (high < low)
      err << "MRandom::long_int: The lower bound (" << low << ") is "
             "greater than the higher bound (" << high << ")" << fatal_error;
   if (high == low) 
      return high;
   unsigned long result = MLC_random(state);
   long range = high - low + 1;  // the range is the number of different
 				 // long integers that could be generated 
   return low + result % range;
}

int MRandom::integer(int low, int high)
{
   return (int)long_int(low, high);
}


/***************************************************************************
  Description : Two functions that return a random long integer and a
                  random integer in the range from 0 to count - 1. 
  Comments    : Count should be greater than 0.
***************************************************************************/
long MRandom::long_int(long count)
{
   return long_int(0, count - 1);
}

int MRandom::integer(int count)
{
   return (int)long_int(0, count - 1);
}


/***************************************************************************
  Description : Returns a random real number uniformly distributed in the
                  interval [low, high).
  Comments    : High should be greater than or equal to low.
                If high and low are equal high is returned without
		  using the state.
                Note that since LONG_MAX has 31 bits the lsb of the
		  real may not be random.		  
***************************************************************************/
Real MRandom::real(Real low, Real high)
{
   check_init();

   if (high < low)
      err << "MRandom::real: The lower bound (" << low << ") is "
             "greater or equal to the higher bound (" << high <<
	     ")" << fatal_error;
   if (high == low) 
      return high;
   ASSERT (sizeof(Real) <= 2*sizeof(long int));
   // shortcut; also (Real)(LONG_MAX + 1) is negative ?!
   Real denominator = (Real)LONG_MAX + 1;
   
   Real range = high - low;
   // calculate the result in two stages in order to get twice the
   //   number of bits in long int
   Real part1 = (Real)MLC_random(state)*range/denominator;
   Real part2 = (((Real)MLC_random(state)/denominator)*range)/denominator;
   return low + part2 + part1; // if low is not zero, part2 probably doesn't
			       // matter.  If it is, we sum low to high which
			       // is more stable.
}


/***************************************************************************
  Description : Returns a random bit value. 
  Comments    : 
***************************************************************************/
Bool MRandom::boolean()
{
   check_init();

   return MLC_random(state)&1;
}


   
