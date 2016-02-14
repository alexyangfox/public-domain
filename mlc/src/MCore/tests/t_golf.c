// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : An include file for defining information relevant to the
                   Quinlan's famous Golf Database.  
                 This is Quinlan's favorite example.  C4.5 book page 18.
  Enhancements :
  History      : Ronny Kohavi                                       9/04/93
                   Initial revision
***************************************************************************/

// The error below is so that bin/checknoobj will not declare this
//    file as a file without a matching .o
// #error "This file can't be compiled by itself"

// Declare all the "global" stuff that makes it easy to work with.
const NominalAttrInfo* label;
const NominalAttrInfo* outlook;
const NominalAttrInfo* temp;
const NominalAttrInfo* humidity;
const NominalAttrInfo* windy;

NominalVal play;
NominalVal dontPlay;

NominalVal sunny;
NominalVal overcast;
NominalVal rain;

NominalVal windyT;
NominalVal windyF;



// Define the basic constants to allow us to work with names.
// These are all global vars.
void def_consts(const SchemaRC& schema)
{
#  define DEFATR(var, num, dscr) \
      var = &schema.attr_info(num).cast_to_nominal();\
      ASSERT(var->name() == dscr)

   label = &schema.label_info().cast_to_nominal();
   ASSERT(label->name() == "Label");
   DEFATR(outlook, 0, "outlook");
   DEFATR(temp,    1, "temperature");
   DEFATR(humidity,2, "humidity");
   DEFATR(windy,   3, "windy");

   play     = label->nominal_to_int("Play");
   dontPlay = label->nominal_to_int("Don't Play");

   sunny    = outlook->nominal_to_int("sunny");
   overcast = outlook->nominal_to_int("overcast");
   rain = outlook->nominal_to_int("rain");

   windyT = windy->nominal_to_int("true");
   windyF = windy->nominal_to_int("false");
}

void def_consts(const InstanceBag& bag)
{
   SchemaRC schema = bag.get_schema();
   def_consts(schema);
}

