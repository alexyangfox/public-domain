// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The GenPix class is an abstract base class template. It
		   is an alternative to the Gnu library pixes, providing
		   better error checking and greater functionality.
		 Pix stands for Pseudo IndeX.  Pixes are used to iterate
		   through elements in a container.  Each pix is associated
		   with a particular instance of a container class and
		   indexes a specific element in the container.  Once
		   created, a pix must always be an index into the same
		   container instance or be "clear".  The specific element
		   that the pix indexes, however, is not fixed -- it will
		   vary as the pix iterates through the container.
		 Every pix has a special value "clear" (similar to the idea
		   of NULL) that cannot be a legal index for an element in
		   the container.  This value is stored in the clearValue
		   data member and must be set before the constructor is
		   called (see the DblLinkList.c header for an example).
		   Incrementing the pix after the last element, or
		   decrementing from the first element, causes the pix to be
		   set to clear.  No operation can be performed on a clear
		   pix other than setting it and checking its state.
		 GenPix has three data members:
		   (1) A constant reference to the container it is iterating
		       through and
		   (2) A pointer or index to an element in the container and
		   (3) A "clear" value which is a special value of the index
		       type (IT) reserved to indicate that the pix does not
		       refer to any element in the list.
		 The second member is essentially the same as the Gnu pix.
		   For an array it might be an integer (the element's index)
		   and for a linked list it might be a pointer to the element
		   node.  The second data member is also used to store the
		   "clear" value when appropriate.
		 The GenPix template takes three arguments:
		   (1) The data type (DT): the type of data stored in the
		       container.
		   (2) The index type (IT): the type of the second data
		       member.  It might be an int for an array or a
		       pointer for a linked list.
		   (3) The container type (CT): the type of the first data
		       member (e.g. "ArrayType").
  Assumptions  :
  Comments     : clearValue is a static member and is therefore shared by
		   all instances of a particular GenPix template
		   instantiation (i.e. it only needs to be set once).
  Complexity   :
  Enhancements : The is_valid function can be improved (in order to provide
		   safer pixes) by keeping track of of the number of deletions
		   in the container.  Both the container class and the pix
		   class would keep their own count.  If the counters differ
		   then the container would check that the pix is valid by
		   traversing its elements.  The pix then updates its count.
		   The code and counters should be DBG.
  History      : Brian Frasca                                      11/29/93
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>

// Note doubly included template can't have id defined 
// RCSID("MLC++, $RCSfile: GenPix.c,v $ $Revision: 1.2 $")

/***************************************************************************
  Description : Generic pix constructor.  This function initializes the
		  pix to index a specific instance of a container class.
		  It also sets the pix to "clear."  If a derived class
		  wants to create a pix with an initial value other than
		  "clear," that should be done in the constructor of the
		  derived class.
  Comments    : See header for clearValue initialization comment.
***************************************************************************/
template <class DT, class IT, class CT>
GenPix<DT,IT,CT>::GenPix(const CT& initialContainer)
   : container(initialContainer)
{
   iter = clearValue;
}

/***************************************************************************
  Description : Copy constructor.
  Comments    : This once caused a compiler error and had to be in the .h
		  file.  It seems to be working now, but if there is a
		  problem it can be moved back.
***************************************************************************/
template <class DT, class IT, class CT>
GenPix<DT,IT,CT>::GenPix(const GenPix<DT,IT,CT>& src)
   : container(src.container), iter(src.iter)
{
}

/***************************************************************************
  Description : Assignment operator.  This function sets the pix to index
		  the same element as the given pix.
  Comments    : It is an error to assign a pix to a different container.
***************************************************************************/
template <class DT, class IT, class CT>
void GenPix<DT,IT,CT>::operator=(const GenPix<DT,IT,CT>& src)
{
   DBG(if (&container != &src.container)
          err << "GenPix<>::operator=(): pixes index different containers ("
              << (void*)&container << " and " << (void*)&src.container << ')'
              << fatal_error);

   iter = src.iter;
}

/***************************************************************************
  Description : Returns TRUE iff both pixes index the same element.
  Comments    : It is an error to compare "clear" pixes or pixes of
		different containers.
***************************************************************************/
template <class DT, class IT, class CT>
Bool GenPix<DT,IT,CT>::operator==(const GenPix<DT,IT,CT>& pix) const
{
   DBG(is_valid(TRUE);
       pix.is_valid(TRUE);
       if (is_clear())
          err << "GenPix<>::operator==(): lhs pix is clear" << fatal_error;
       if (pix.is_clear())
          err << "GenPix<>::operator==(): rhs pix is clear" << fatal_error;
       if (&container != &pix.container)
          err << "GenPix<>::operator==(): pixes index different containers ("
              << (void*)&container << " and " << (void*)&pix.container << ')'
              << fatal_error);

   return iter == pix.iter;
}

/***************************************************************************
  Description : Returns TRUE iff the pixes index different elements.
  Comments    : It is an error to compare pixes of different containers.
***************************************************************************/
template <class DT, class IT, class CT>
Bool GenPix<DT,IT,CT>::operator!=(const GenPix<DT,IT,CT>& pix) const
{
   DBG(is_valid(TRUE);
       pix.is_valid(TRUE);
       if (is_clear())
          err << "GenPix<>::operator!=(): lhs pix is clear" << fatal_error;
       if (pix.is_clear())
          err << "GenPix<>::operator!=(): rhs pix is clear" << fatal_error;
       if (&container != &pix.container)
          err << "GenPix<>::operator!=(): pixes index different containers ("
              << (void*)&container << " and " << (void*)&pix.container << ')'
              << fatal_error);

   return iter != pix.iter;
}

/***************************************************************************
  Description : Returns the value of the element indexed by the pix.
  Comments    : There is no non-const version of this operator because a
		  pix should not have write access to the container class.
		The container class must have a function "data" which takes
		  an index (of class IT) and returns the data (of class DT)
		  stored at that index.
***************************************************************************/
template <class DT, class IT, class CT>
const DT& GenPix<DT,IT,CT>::operator*() const
{
   DBG(is_valid(TRUE);
       if (is_clear())
          err << "GenPix<>::operator*(): pix is clear" << fatal_error);

   return container.data(iter);
}
