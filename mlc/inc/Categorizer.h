// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _Categorizer_h
#define _Categorizer_h 1

// Define class IDs for Categorizers.  Note that these are not enums
//   because we want to give users the ability to define more of these
//   without changing the header file.
#define CLASS_CONST_CATEGORIZER      1
//@@ Is CLASS_AUG_CATEGORIZER used someplace?
#define CLASS_AUG_CATEGORIZER        2
#define CLASS_THRESHOLD_CATEGORIZER  3
#define CLASS_ATTR_CATEGORIZER       4
#define CLASS_RDG_CATEGORIZER        5
#define CLASS_BAD_CATEGORIZER        6
#define CLASS_TABLE_CATEGORIZER      7
#define CLASS_IB_CATEGORIZER         8
#define CLASS_LAZYDT_CATEGORIZER     9
#define CLASS_NB_CATEGORIZER	    10
#define CLASS_PROJECT_CATEGORIZER   11
#define CLASS_DISC_CATEGORIZER      12
#define CLASS_ATTR_EQ_CATEGORIZER   13
#define CLASS_ATTR_SET_CATEGORIZER  14
#define CLASS_BAGGING_CATEGORIZER   15
#define CLASS_LINDISCR_CATEGORIZER  16
#define CLASS_CASCADE_CATEGORIZER   17


#include <InstanceRC.h>
#include <MLCStream.h>
#include <AugCategory.h>
#include <DisplayPref.h>
#include <LogOptions.h>
#include <Array.h>

class NodeInfo; // needed for friend function
class CtrInstanceBag;

// A categorizer returns a category given an instance.
class Categorizer { // ABC
   const int    numCat;
   MString descr; // ASCII description.  Subclasses may have
                        // optional descriptors (e.g. graphics)
   Array<int> *distrArray; // Probability distribution of categories (label)
   int instancesNum; // Number of instances while building the
			   // categorizer. 
   
   NO_COPY_CTOR(Categorizer);
   LOG_OPTIONS;
protected:
  virtual void short_display(ostream&) const; // Used in Graph::display

public:
   Categorizer(int noCat, const MString& dscr);
   Categorizer(const Categorizer& source, CtorDummy);
   virtual void OK(int /* level */) const {}
   virtual ~Categorizer();
   virtual int num_categories() const { return numCat;}
   const MString&  description() const {return descr;}
   //@@ We want to set the description after the Categorizer is initialized.
   // This function shouldn't be virtual since every categorizer should be
   // able to set its description after it is initialized.
   void set_description(MString& val) { descr = val; }
   
   virtual AugCategory categorize(const InstanceRC&) const = 0;
   virtual void display_struct(MLCOStream& stream,
			       const DisplayPref& dp) const = 0;
   // Returns a pointer to a deep copy of this Categorizer
   virtual Categorizer* copy() const = 0;
   
   // Build the probability distribution;
   virtual void build_distr(const CtrInstanceBag& bag);

   // Copy the probability distribution;
   virtual void set_distr(const Array<int>& val);

   // Add the probability distribution;
   virtual void add_distr(const Array<int>& val);   
   
   // Get the probability distribution.
   virtual const Array<int>& get_distr() const
   { ASSERT(distrArray); return *distrArray; }
   
   // Get the majority of distribution array
   virtual Category majority_category() const ;
   
   // Returns the number of instances in the categorizer
   virtual int num_instances() const { return instancesNum; }

   // Returns the class id
   virtual int class_id() const = 0;

   virtual Bool operator==(const Categorizer&)const  = 0;
   Bool operator!=(const Categorizer& cat) const
   { return ! ((*this)== cat);}
// uses short_display()
friend void Print(const NodeInfo* categorizer, ostream& stream);
};

#endif
