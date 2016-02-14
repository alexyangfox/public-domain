// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : BoolArray is an Array<Bool> that knows how to print itself
		   as a comma-separated list of "T"s and "F"s.
  Assumptions  :
  Comments     : This was created because Bool is really a char and thus
		   printing an Array<Bool> was the same as printing an
		   Array<char>.
  Complexity   :
  Enhancements :
  History      :   James Dougherty                                    1/25/95
                   Added BoolArray(const Array<int>&);
                   Brian Frasca                                       5/07/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <MLCStream.h>
#include <BoolArray.h>

RCSID("MLC++, $RCSfile: BoolArray.c,v $ $Revision: 1.9 $")

/***************************************************************************
  Description : Constructors.
  Comments    :
***************************************************************************/
BoolArray::BoolArray(const BoolArray& source, CtorDummy dummyArg)
   : Array<Bool>(source, dummyArg)
{
}

BoolArray::BoolArray(const Array<int>& source)
   : Array<Bool>(0, source.size(), FALSE)
{
   for(int i = 0; i < size(); i++)
      if(source.index(i))
	 index(i) = TRUE;
}

BoolArray::BoolArray(int base, int size) : Array<Bool>(base, size)
{
}

BoolArray::BoolArray(int size) : Array<Bool>(size)
{
}

BoolArray::BoolArray(int base, int size, const Bool& initValue)
   : Array<Bool>(base, size, initValue)
{
}

/***************************************************************************
  Description : Display the array.  Use T,F for values.
  Comments    :
***************************************************************************/
void BoolArray::display(MLCOStream& stream) const
{
  int i;
  for (i = 0; i < size() - 1; i++)
      stream << (index(i) ? "T, " : "F, ");

   if (size() > 0) // Don't print comma at end.
      stream << (index(i) ? "T" : "F");
}

DEF_DISPLAY(BoolArray)

     
/***************************************************************************
  Description : Returns an MString containing a comma separated list of
                  those indexes whose corresponding elements are TRUE.
  Comments    : This is expensive for large arrays because it
                  constructs new strings for every TRUE element.
***************************************************************************/
MString BoolArray::get_true_indexes() const
{
   int lastTrueIndex = high();
   while (lastTrueIndex >= low() && !operator[](lastTrueIndex))
      lastTrueIndex--;
   
   if (lastTrueIndex < low())
      return MString("");
   if (lastTrueIndex == low())
      return MString(lastTrueIndex,0);

   MString returnString;
   for (int i = low(); i < lastTrueIndex; i++)
      if (operator[](i)) {
	 returnString += MString(i,0);
	 returnString += ", ";
      }
   returnString += MString(lastTrueIndex,0);
   
   return returnString;
}


/***************************************************************************
  Description : Returns a count of the number of TRUE elements in the
                  array.
  Comments    : 
***************************************************************************/
int BoolArray::num_true() const
{
   int sum = 0;
   for(int i = low(); i <= high(); i++)
      if(operator[](i))
	 sum++;
   return sum;
}
