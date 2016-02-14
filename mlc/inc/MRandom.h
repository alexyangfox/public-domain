// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file "MRandom.c".

#ifndef _MRandom_h
#define _MRandom_h 1

const int STATE_LEN = 256 / sizeof(long);  // number of bytes used as state
                                           // information--256 is the highest
                                           // state size supported.

class MRandom {
   NO_COPY_CTOR(MRandom);   // Expensive copy, and not trivial to do right.
                            // It's unclear whether the copy should start from
			    //   the original seed, or from the current stream
   long state[STATE_LEN];   // must be long aligned
   Bool initialized;        // Was there a seed?  Mainly for debug purposes.
   void check_init();
public:
   MRandom();
   MRandom(unsigned seed);
   virtual ~MRandom() {}      // nothing to deallocated

   virtual void init(unsigned seed);
   
   virtual long long_int(long count); // [0, count-1]
   virtual long long_int(long low, long high); // [low, high]
   virtual int integer (int count); // [0, count-1]
   virtual int integer (int low, int high); // [low, high]
   virtual Real real(Real low, Real high);  // [low, high)
   virtual Bool boolean();
};

// The following macro is meant to be used in class declarataions in the
// private part.  It will define a member randNumGen which is the random
// number generator, and provide public functions for operations on it.

#define RAND_OPTIONS \
public: \
   MRandom& rand_num_gen() {return randNumGen;} \
   void init_rand_num_gen(unsigned seed) {randNumGen.init(seed);} \
private: \
   MRandom randNumGen

#endif




