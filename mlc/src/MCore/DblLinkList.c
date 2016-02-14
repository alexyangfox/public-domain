// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The DLLPix class provides safe pixes for the DblLinkList
		   class.  It is derived from the GenPix class.  All pixes
		   used to iterate through a DblLinkList should be DLLPixes.
		   (see GenPix.c)
  Assumptions  :
  Comments     :
  Complexity   :
  Enhancements : See GenPix.c for an enhancement to is_valid.
  History      : Brian Frasca                                      11/28/93
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <MLCStream.h>
#include <error.h>
// Note that this must be included even thought it's a template, or
// you get an error because the .c file is included before the .h file.
#include <DblLinkList.h>

// RCSID causes duplicate definitions of ::id.
// RCSID("MLC++, $RCSfile: DblLinkList.c,v $ $Revision: 1.16 $")

/***************************************************************************
  Description : Pix constructor.  Given a doubly linked list, this function
		  creates a pix to iterate through its elements.  The
		  element initially indexed depends on the value of dir:
                        dir = -1 : set pix to last element
                        dir =  0 : set pix to "clear"
                        dir =  1 : set pix to first element
                  If no value is given for dir, it will default to 0 and
                  set the pix to "clear." (see GenPix.c)
  Comments    :
***************************************************************************/
template <class T>
DLLPix<T>::DLLPix(const DblLinkList<T>& dblLinkList, int dir)
   : GenPix<T, DLLNode<T>*, DblLinkList<T> >(dblLinkList)
{
   switch (dir) {
      case -1:   last();  break;
      case  0:            break; // automatically set to "clear" in GenPix
      case  1:   first(); break;
      default:   err << "DLLPix<>::DLLPix(): illegal value for dir ("
                     << dir << ')' << fatal_error;
   }
}

/***************************************************************************
  Description : The next two functions set the pix to index the first/last
                  element in the container.
  Comments    :
***************************************************************************/
template <class T>
void DLLPix<T>::first()
{
   iter = container.head;
}

template <class T>
void DLLPix<T>::last()
{
   iter = container.tail;
}

/***************************************************************************
  Description : The next two functions set the pix to index the element of
                  the container after/before the element currently indexed.
                If the pix indexes the last element and next() is called,
                  then the pix is set to "clear."  Similarly if the pix
                  indexes the first element and prev() is called.
  Comments    :	It is an error to call next()/prev() if the pix is "clear."
***************************************************************************/
template <class T>
void DLLPix<T>::next()
{
   DBGSLOW(is_valid(TRUE));
   DBG(if (is_clear())
      err << "DLLPix<>::next(): pix is clear" << fatal_error);

   iter = iter->next;
}

template <class T>
void DLLPix<T>::prev()
{
   DBGSLOW(is_valid(TRUE));
   DBG(if (is_clear())
      err << "DLLPix<>::prev(): pix is clear" << fatal_error);

   iter = iter->prev;
}

/***************************************************************************
  Description : Returns TRUE iff the pix indexes a valid element.
		Aborts if the pix does not index a valid element and the
		  fatalOnFalse flag is set to TRUE.
		fatalOnFalse defaults to FALSE.
  Comments    : This function does not completely guarantee that the
		  element is valid.  For a better (and more expensive) test
		  use owns(pix) from the DblLinkList class.
                This function uses is_valid_pointer() from error.c to
                  determine if the pointer is in the program's address space.
		See the enhancements for an improvement to is_valid.
***************************************************************************/
template <class T>
Bool DLLPix<T>::is_valid(Bool fatalOnFalse) const
{
   if (is_clear() || iter == container.head || iter == container.tail ||
       (is_valid_pointer(iter) && 
	iter->next != NULL && is_valid_pointer(iter->next) &&
        iter->prev != NULL && is_valid_pointer(iter->prev) &&
        iter->next->prev != NULL && is_valid_pointer(iter->next->prev) &&
        iter->prev->next != NULL && is_valid_pointer(iter->prev->next) &&
        iter == iter->next->prev && iter == iter->prev->next))
      return TRUE;

   if (fatalOnFalse)
      err << "DLLPix<>::is_valid(): invalid pix" << fatal_error;

   return FALSE;
}

/***************************************************************************
  Description : The prefix ++/-- operators make the pix index the next/prev
		  element in the container.  Returns the new pix value.
  Comments    : There is no postfix version of these operators.  Since it
		  is an error to do this operation on a "clear" pix,
		  statements like while (pix++) are very error prone.
		Also, the postfix versions would require an extra pix copy.
***************************************************************************/
template <class T>
const DLLPix<T>& DLLPix<T>::operator++()
{
   next();
   return *this;
}

template <class T>
const DLLPix<T>& DLLPix<T>::operator--()
{
   prev();
   return *this;
}


/***************************************************************************
  Description  : DblLinkList is a doubly linked list template.  The one
		   template argument specifies the type of data stored in
		   the list.  The constructor takes no arguments and creates
		   an empty list.  The data is copied into the list.  If you
		   don't want to make a copy, make the data pointers to the
		   stored items.
		 This class was written to replace the Gnu DLList class.
		   The Gnu length function took linear time whereas this
		   class does it in constant time.  Also, this class uses
		   the enhanced GenPix class instead of Gnu void* pixes.
  Assumptions  : There must be a copy constructor for the data type stored
		   in the list.  The copy constructor must be an MLC++ type
		   constructor taking ctorDummy as second argument.
		   If such a constructor doesn't exist, this class cannot be
		   instantiated, and you must explicitly define
		   DLLNode<T> for your type (a few lines).
  Comments     : Space is allocated dynamically each time a new element is
		   inserted into the list.
  Complexity   : OK()/owns() take O(num-elem) time.
		 Destructor/free() take O(num-elem * elem-destructor-time)
		   time.
  Enhancements :
  History      : Brian Frasca                                      11/28/93
                   Initial revision (.h,.c)
***************************************************************************/


/***************************************************************************
  Description : data() returns the data stored in the given node.
  Comments    : This was created to enable GenPix::operator*().
		Protected member.
***************************************************************************/
template <class T>
const T& DblLinkList<T>::data(const DLLNode<T>* node) const
{
   DBG(if (node == NULL)
          err << "DblLinkList<>::data(): invalid node" << fatal_error;
       OK(1));

   return node->data;
}

/***************************************************************************
  Description : OK() checks that the structure of the list is consistent.
		It aborts if any of the following invariants is not true:
		  1. The listLength must always agree with the number of
		     elements in the list.
		  2. For every elem, prev(next(elem)) == elem and
		     next(prev(elem)) == elem.
		  3. next(tail) == NULL; prev(head) == NULL.
		  4. The list has no cycles.
  Comments    : OK(1) takes linear time but only checks the listLength,
		  head, and tail of the list.
		OK() defaults to OK(0) which does the more extensive check
		  described above.
***************************************************************************/
template <class T>
void DblLinkList<T>::OK(int level) const
{
   ASSERT((listLength == 0 && head == NULL && tail == NULL) ||
          (listLength == 1 && head != NULL && head->prev == NULL
                           && head == tail && head->next == NULL) ||
          (listLength >  1 && head != NULL && head->prev == NULL
                           && tail != NULL && tail->next == NULL));
   if (level >= 1) return;

   // OK(0)
   // traverse list forward
   DLLNode<T> *cur = head, *last = NULL;
   for (int i = listLength; cur != NULL && cur->prev == last && i != 0; i--) {
      last = cur;
      cur = cur->next;
   }
   ASSERT(cur == NULL && last == tail && i == 0);

   // traverse list backward
   cur = tail;
   last = NULL;
   for (i = listLength; cur != NULL && cur->next == last && i != 0; i--) {
      last = cur;
      cur = cur->prev;
   }
   ASSERT(cur == NULL && last == head && i == 0);
}

/***************************************************************************
  Description : Constructor that creates an empty list.
  Comments    :
***************************************************************************/
template <class T>
DblLinkList<T>::DblLinkList()
{
   head = tail = NULL;
   listLength = 0;
}

/***************************************************************************
  Description : Copy constructor.
  Comments    : The argument "dummy" is in DBG_DECLARE to avoid compiler
		warning in FAST mode.
***************************************************************************/
template <class T>
DblLinkList<T>::DblLinkList(const DblLinkList<T>& src, 
			    CtorDummy /* dummyArg */)
{
   head = tail = NULL;
   listLength = 0;
   for (DLLPix<T> p(src,1); p; ++p)
      append((*this)(p));
   ASSERT(length() == src.length());
   DBG(OK());
}



/***************************************************************************
  Description : Destructor/free functions delete each element of the list.
  Comments    :
***************************************************************************/
template <class T>
DblLinkList<T>::~DblLinkList()
{
   free();
}

template <class T>
void DblLinkList<T>::free()
{
   DBG(OK());
   while (head != NULL) {
      DLLNode<T>* p = head;
      head = p->next;
      DBG(p->next = p->prev = NULL);
      delete p;
   }
   tail = NULL;
   listLength = 0;
}

/***************************************************************************
  Description : Returns the number of elements in the list.
  Comments    :
***************************************************************************/
template <class T>
int DblLinkList<T>::length() const
{
   DBG(OK(1));
   return listLength;
}

/***************************************************************************
  Description : Returns FALSE if the list is not empty.  Returns TRUE or
		  aborts (if the fatal_on_empty flag is TRUE) if the list
		  is empty.
  Comments    : fatal_on_empty defaults to FALSE
***************************************************************************/
template <class T>
Bool DblLinkList<T>::empty(Bool fatal_on_empty) const
{
   if (length() == 0)
      if (fatal_on_empty)
         err << "DblLinkList<>::empty(): list is empty" << fatal_error;
      else
         return TRUE;
   return FALSE;
}

/***************************************************************************
  Description : Returns TRUE iff list contains the element indexed by pix.
		Aborts on FALSE if the fatalOnFalse flag is TRUE.
  Comments    : fatalOnFalse defaults to FALSE
***************************************************************************/
template <class T>
Bool DblLinkList<T>::owns(const DLLPix<T>& pix, Bool fatalOnFalse) const
{
   DBG(if (pix.is_clear())
          err << "DblLinkList<>::owns(): pix is clear" << fatal_error;
       OK(1));

   for (DLLPix<T> p(*this,1); p; ++p)
      if (p.iter == pix.iter) return TRUE;

   if (fatalOnFalse)
      err << "DblLinkLIst<>::owns(): pix not in list" << fatal_error;

   return FALSE;
}

/***************************************************************************
  Description : The next two functions return a constant reference to the
		  first/last element in the list.
  Comments    :
***************************************************************************/
template <class T>
const T& DblLinkList<T>::front() const
{
   DBG(empty(TRUE);
       OK(1));

   return head->data;
}

template <class T>
const T& DblLinkList<T>::rear() const
{
   DBG(empty(TRUE);
       OK(1));

   return tail->data;
}

/***************************************************************************
  Description : operator() returns the element of the list pointed to by
		  the given pix.
  Comments    :
***************************************************************************/
template <class T>
const T& DblLinkList<T>::operator()(const DLLPix<T>& pix) const
{
   DBGSLOW(pix.is_valid(TRUE));
   DBG(if (pix.is_clear())
      err << "DblLinkList<>::operator()(): pix is clear" << fatal_error);
   DBG(OK(1));

   DBG(ASSERT(pix.iter != NULL));
   return pix.iter->data;
}

template <class T>
T& DblLinkList<T>::operator()(const DLLPix<T>& pix)
{
   DBGSLOW(pix.is_valid(TRUE));
   DBG(if (pix.is_clear())
          err << "DblLinkList<>::operator()(): pix is clear" << fatal_error);
   DBG(OK(1));

   ASSERT(pix.iter != NULL);
   return pix.iter->data;
}

/***************************************************************************
  Description : This function will insert an element in the list after the
		  element pointed to by pix.  If pix is clear, the element
		  will be inserted in the front of the list.
  Comments    : This function is called by all the other insertion functions.
***************************************************************************/
template <class T>
DLLPix<T> DblLinkList<T>::ins_after(const DLLPix<T>& pix, const T& data)
{
   DBGSLOW(pix.is_valid(TRUE));

   if (pix.is_clear())
      return prepend(data);
   else if (pix.iter == tail)
      return append(data);
   else {
      DBG(OK(1));
      ASSERT(!empty());

      // initialize node with data -- requires copy constructor for data
      DLLNode<T>* node = new DLLNode<T>(data);

      // insert node in list
      node->prev = pix.iter;
      node->next = pix.iter->next;
      pix.iter->next->prev = node;
      pix.iter->next = node;
      ++listLength;

      // create pix to newly inserted element
      DLLPix<T> newPix = pix; newPix.next();
      DBGSLOW(ASSERT(newPix.is_valid()));

      DBG(OK(1));
      return newPix;
   }
}

/***************************************************************************
  Description : This function will insert an element in the list before the
		  element pointed to by pix.
  Comments    : Calls ins_after to do the work.
***************************************************************************/
template <class T>
DLLPix<T> DblLinkList<T>::ins_before(const DLLPix<T>& pix, const T& data)
{
   DBGSLOW(pix.is_valid(TRUE));
   if (pix.is_clear())
          err << "DblLinkList<>::ins_before(): pix is clear" << fatal_error;

   // create pix to element before insertion point
   DLLPix<T> prevPix = pix; prevPix.prev();
   DBGSLOW(ASSERT(prevPix.is_valid()));

   return ins_after(prevPix, data);
}

/***************************************************************************
  Description : Adds an element to the front of the list.
  Comments    :
***************************************************************************/
template <class T>
DLLPix<T> DblLinkList<T>::prepend(const T& data)
{
   DBG(OK(1));

   // initialize node with data -- requires copy constructor for data
   DLLNode<T>* node = new DLLNode<T>(data);

   // insert node in list
   if (empty()) {
      node->prev = node->next = NULL;
      head = tail = node;
   }
   else{
      node->prev = NULL;
      node->next = head;
      head->prev = node;
      head = node;
   }
   ++listLength;

   DBG(OK(1));
   return DLLPix<T>(*this,1); // return pix to head
}

/***************************************************************************
  Description : Adds an element to the end of the list.
  Comments    :
***************************************************************************/
template <class T>
DLLPix<T> DblLinkList<T>::append(const T& data)
{
   DBG(OK(1));

   // initialize node with data -- requires copy constructor for data
   DLLNode<T>* node = new DLLNode<T>(data);

   // insert node in list
   if (empty()) {
      node->prev = node->next = NULL;
      head = tail = node;
   }
   else{
      node->prev = tail;
      node->next = NULL;
      tail->next = node;
      tail = node;
   }
   ++listLength;

   DBG(OK(1));
   return DLLPix<T>(*this,-1); // return pix to tail
}

/***************************************************************************
  Description : Deletes the element pointed to by pix and updates pix to
		  point to next element in the list if dir is 1 or the
		  previous element if dir is -1.  dir defaults to 1.
  Comments    :
***************************************************************************/
template <class T>
void DblLinkList<T>::del(DLLPix<T>& pix, int dir)
{
   empty(TRUE);

   if (pix.is_clear())
      err << "DblLinkList<>::del(): pix is clear" << fatal_error;

   DBG(if (dir != 1 && dir != -1)
      err << "DblLinkList<>::del(): invalid direction (" << dir << ')'
	  << fatal_error);

   DBGSLOW(pix.is_valid(TRUE));

   // save pix to element that will be deleted
   DLLPix<T> delPix = pix;
   DBGSLOW(ASSERT(delPix.is_valid()));

   // update pix
   if (dir == 1)
      pix.next();
   else
      pix.prev();

   // delete the element indexed by delPix
   if (delPix.iter == head)
      del_front();			// calls DBG(OK(1))
   else if (delPix.iter == tail)
      del_rear();			// calls DBG(OK(1))
   else {
      DBG(OK(1));

      // remove element from list
      delPix.iter->prev->next = delPix.iter->next;
      delPix.iter->next->prev = delPix.iter->prev;
      --listLength;

      // delete element
      DBG(delPix.iter->next = delPix.iter->prev = NULL);
      delete delPix.iter;

      DBG(OK(1));
   }
}

/***************************************************************************
  Description : The next two functions delete the first/last element in the
		  list and return the deleted element.
  Comments    : Call del_front/del_rear to do the deletion.
***************************************************************************/
template <class T>
T DblLinkList<T>::remove_front()
{
   DBG(empty(TRUE));

   T frontData = head->data;
   del_front();
   return frontData;
}

template <class T>
T DblLinkList<T>::remove_rear()
{
   DBG(empty(TRUE));

   T rearData = tail->data;
   del_rear();
   return rearData;
}

/***************************************************************************
  Description : The next two functions delete the first/last element in the
		  list respectively.
  Comments    :
***************************************************************************/
template <class T>
void DblLinkList<T>::del_front()
{
   DBG(empty(TRUE);
       OK(1));

   // save pointer to old head
   DLLNode<T>* oldHead = head;

   // remove old head from list
   if (listLength == 1)
      head = tail = NULL;
   else {
      ASSERT(head->next != NULL);
      head->next->prev = NULL;
      head = head->next;
   }
   --listLength;

   // delete old head
   DBG(oldHead->next = oldHead->prev = NULL);
   delete oldHead;

   DBG(OK(1));
}

template <class T>
void DblLinkList<T>::del_rear()
{
   DBG(empty(TRUE);
       OK(1));

   // save pointer to old tail
   DLLNode<T>* oldTail = tail;

   // remove old tail from list
   if (listLength == 1)
      head = tail = NULL;
   else {
      ASSERT(tail->prev != NULL);
      tail->prev->next = NULL;
      tail = tail->prev;
   }
   --listLength;

   // delete old tail
   DBG(oldTail->next = oldTail->prev = NULL);
   delete oldTail;

   DBG(OK(1));
}

/***************************************************************************
  Description : Deletes the element in the list after the element pointed
		  to by pix.  Deletes the first element if pix is clear.
  Comments    : Calls del to do the work.
***************************************************************************/
template <class T>
void DblLinkList<T>::del_after(const DLLPix<T>& pix)
{
   empty(TRUE);
   DBGSLOW(pix.is_valid(TRUE));
   if (pix.iter == tail)
      err << "DblLinkList<>::del_after(): pix at end of list"
	  << fatal_error;

   if (pix.is_clear())
      del_front();
   else {
      // create pix to element that will be deleted
      DLLPix<T> delPix = pix; delPix.next();
      DBGSLOW(ASSERT(delPix.is_valid()));

      // use del() to delete
      del(delPix);
   }
}

/***************************************************************************
  Description : Displays the elements of the list in order.  The output is
		  sent to the given MLCOStream.
  Comments    :
***************************************************************************/
template <class T>
void DblLinkList<T>::display(MLCOStream& stream) const
{
   for (DLLPix<T> p(*this,1); p; ++p)
      stream << *p << ", ";
}

// Define operator<< for display()
template <class T>
DEF_DISPLAY(DblLinkList<T>)
