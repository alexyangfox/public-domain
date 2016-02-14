// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _GenPix_h
#define _GenPix_h 1

#include <basics.h>	// for Bool

template <class DT, class IT, class CT>		// DT = data type
class GenPix {					// IT = index type
protected:					// CT = container type
   const CT& container;
   IT iter;
public:
   static IT const clearValue;

   GenPix(const CT& initialContainer);
   GenPix(const GenPix<DT,IT,CT>& src);		// copy constructor

   virtual void first() = 0;
   virtual void last() = 0;
   virtual void next() = 0;
   virtual void prev() = 0;
   virtual Bool is_valid(Bool fatalOnFalse = FALSE) const = 0;

   Bool is_clear() const { return iter == clearValue; }
   void set_clear() { iter = clearValue; }
   operator Bool() const { return !is_clear(); }
   Bool operator!() const { return is_clear(); }

   void operator=(const GenPix<DT,IT,CT>&);
   Bool operator==(const GenPix<DT,IT,CT>&) const;
   Bool operator!=(const GenPix<DT,IT,CT>&) const;
   const DT& operator*() const;	// Use CT::operator() for the non-const
				//   version; a pix should not have write
				//   access to the container.
};

#endif
