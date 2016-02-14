// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _DblLinkList_h
#define _DblLinkList_h 1

#include <GenPix.h>

// If you are creating a DblLinkList of a type that has a copy constructor
//   with ctorDummy, you can redefine DLLNode<T> yourself to make
//   the copy.  However, this is usually a sign that the copy is
//   taking too long, and you might want to consider encapsulating
//   the class in another class that has operator== defined,
//   and simply stores a pointer to the class.

template<class T> class DLLNode {
   NO_COPY_CTOR_TEMP(DLLNode,DLLNode<T>);
public:
   // Note that it is important to initial data in initialization
   //   list and not in the statements as {data = initialData;}
   //   The reason is that in the latter case you need a NULL constructor and
   //   operator=, while in this format, only a copy constructor is needed.
   DLLNode(const T& initialData) : data(initialData) {}
   T data;
   DLLNode<T>* next;
   DLLNode<T>* prev;
};


template <class T> class DblLinkList;

// The following classes do not have virtual functions because they need to
// be very fast and no classes are derived from them.

/***************************************************************************
  DLLPix
***************************************************************************/
template <class T>
class DLLPix : public GenPix<T, DLLNode<T>*, DblLinkList<T> > {

// DblLinkList uses DLLPix::iter
friend class DblLinkList<T>;

public:
   DLLPix(const DblLinkList<T>& c,int dir = 0);	// dir determines init elem:
                                                //   -1 sets pix to last elem
                                                //    0 sets pix to "clear"
                                                //    1 sets pix to first elem
   // copy constructor
   DLLPix(const DLLPix<T>& srcPix)
      : GenPix<T, DLLNode<T>*, DblLinkList<T> >(srcPix) {}

   void first();
   void last();
   void next();
   void prev();
   Bool is_valid(Bool fatalOnFalse = FALSE) const;
   const DLLPix<T>& operator++();
   const DLLPix<T>& operator--();
};

// The spaces between dataType and the <>'s are necessary for template
//   dataTypes.
#define SET_DLLPIX_CLEAR(dataType, pixClearVal)				\
           DLLNode< dataType >* const					\
              GenPix< dataType, DLLNode< dataType >*, 			\
                     DblLinkList< dataType > >::clearValue = pixClearVal;


/***************************************************************************
  DblLinkList
***************************************************************************/
template<class T> class DblLinkList {

// DLLPix<T> must access head and tail when setting the pix to first or last.
friend class DLLPix<T>;

// GenPix<...> uses protected member data() in operator*().
friend class GenPix<T, DLLNode<T>*, DblLinkList<T> >;

   DLLNode<T>* head;
   DLLNode<T>* tail;
   int listLength;
   NO_COPY_CTOR_TEMP(DblLinkList,DblLinkList<T>);

protected:
   const T& data(const DLLNode<T>*) const;

public:
   void OK(int level = 0) const;
   DblLinkList();
   DblLinkList(const DblLinkList& src, CtorDummy);

   ~DblLinkList();
   void free();

   int length() const;
   Bool empty(Bool fatal_on_empty = FALSE) const;	
   Bool owns(const DLLPix<T>&, Bool fatalOnFalse = FALSE) const;

   const T& front() const;
   const T& rear() const;
   const T& operator()(const DLLPix<T>&) const;
   T& operator()(const DLLPix<T>&);

   DLLPix<T> ins_after(const DLLPix<T>&,const T&);
   DLLPix<T> ins_before(const DLLPix<T>&,const T&);
   DLLPix<T> prepend(const T&);
   DLLPix<T> append(const T&);
   void del(DLLPix<T>&, int dir = 1);	// dir =  1: pix becomes next element
					// dir = -1: pix becomes prev element
   T remove_front();
   T remove_rear();
   void del_front();
   void del_rear();
   void del_after(const DLLPix<T>&);
   void display(MLCOStream& stream = Mcout) const;
};

template <class T>
DECLARE_DISPLAY(DblLinkList<T>);


#endif
