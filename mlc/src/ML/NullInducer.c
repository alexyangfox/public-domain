// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The Null Inducer is an inducer which does nothing.
                 If fatalOnCall is TRUE, any call which is related
		    to training a structure will abort.
		 If it is FALSE, calls will do the minimum necessary to allow
   		   the null inducer to execute in loops where it's used to
		   store and retrieve data.   The categorizer will constantly
		   return UNKNOWN_CATEGORY_VAL.
  Assumptions  : 
  Comments     :
  Complexity   :
  Enhancements :
  History      : Ronny Kohavi                                       9/12/94
                   Added fatalOnCall option
                 Brian Frasca 				            1/29/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <NullInducer.h>
#include <ConstCat.h>
#include <error.h>

RCSID("MLC++, $RCSfile: NullInducer.c,v $ $Revision: 1.14 $")

// Macro for error message
#define CANNOT_CALL(function) if (abortOnCalls) \
        err << "NullInducer::" #function "() - cannot call from " \
            << "a null inducer" << fatal_error

/***************************************************************************
  Description : See Inducer.c for descriptions 
  Comments    : 
***************************************************************************/

InstanceBag* NullInducer::assign_bag(InstanceBag*& newTS)
{
   CANNOT_CALL(assign_bag);
   return Inducer::assign_bag(newTS);
}

NullInducer::NullInducer(const MString& dscr, Bool fatalOnCalls)
  : Inducer(dscr)
{
   abortOnCalls = fatalOnCalls;
   trained = FALSE;
}

void NullInducer::read_data(const MString& file,
			    const MString& namesExtension, 
			    const MString& dataExtension)
{
   CANNOT_CALL(read_data);
   Inducer::read_data(file, namesExtension, dataExtension);
}

InstanceBag* NullInducer::release_data()
{
   CANNOT_CALL(release_data);
   return Inducer::release_data();
}

Bool NullInducer::has_data(Bool fatalOnFalse) const
{
   CANNOT_CALL(has_data);
   return Inducer::has_data(fatalOnFalse);
}


const InstanceBag& NullInducer::instance_bag() const
{
   CANNOT_CALL(instance_bag);
   return Inducer::instance_bag();
}

void NullInducer::train()
{
   CANNOT_CALL(train);
}

Bool NullInducer::was_trained(Bool fatalOnFalse) const
{
   CANNOT_CALL(was_trained);
   if (fatalOnFalse && !trained)
      err << "NullInducer::was_trained: Inducer not trained"
          << fatal_error;

   return trained;
}


const Categorizer& NullInducer::get_categorizer() const
{
   static MStringRC unknownStr("?");
   static AugCategory unknownAugCat(UNKNOWN_CATEGORY_VAL, unknownStr);
   static MString unknownDscr("NullInducer");
   static ConstCategorizer unknownCat(unknownDscr, unknownAugCat);

   CANNOT_CALL(get_categorizer);

   return unknownCat;
}


void NullInducer::display_struct(MLCOStream& stream,
                                 const DisplayPref& dp) const
{
   CANNOT_CALL(display_struct);
   if (stream.output_type() == XStream)
      err << "NullInducer::display_struct: Xstream is not a valid "
          << "stream for this display_struct"  << fatal_error;

   if (dp.preference_type() != DisplayPref::ASCIIDisplay)
      err << "NullInducer::display_struct: Only ASCIIDisplay is "
          << "valid for this display_struct"  << fatal_error;
      

   stream << "Null Categorizer " << description() << endl;
}



