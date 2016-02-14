// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Creates several lists and tests each of the functions.
  Doesn't test :
  Enhancements :
  History      : Brian Frasca                                      11/28/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <MLCStream.h>
#include <errorUnless.h>
#include <DblLinkList.h>

RCSID("MLC++, $RCSfile: t_DblLinkList.c,v $ $Revision: 1.8 $")

const int NUM = 10; // NUM must be >= 10

SET_DLLPIX_CLEAR(int, NULL);

void check_copy(const DblLinkList<int>& l1)
{
   DblLinkList<int> l2(l1, ctorDummy);
   DLLPix<int> pix1(l1, 1), pix2(l2, 1);

   while (pix1 && pix2) {
      ASSERT(l1(pix1) == l2(pix2));
      Mcout << l1(pix1) << " " << l2(pix2) << endl;
      ++pix1; ++pix2;
   }
}   

main()
{
   Mcout << "t_DblLinkList executing" << endl;

   DblLinkList<int> list, diffList;
   DLLPix<int> pix(list), diffPix(diffList);

   TEST_ERROR("pixes index different containers", diffPix = pix);

   ASSERT(list.empty() && list.length() == 0);
   ASSERT(pix.is_clear() && !pix);
   Mcout << "Empty list: " << list << endl;

   // test empty list errors
   TEST_ERROR("list is empty", list.front());
   TEST_ERROR("list is empty", list.rear());
   TEST_ERROR("list is empty", list.del(pix));
   TEST_ERROR("list is empty", list.remove_front());
   TEST_ERROR("list is empty", list.remove_rear());
   TEST_ERROR("list is empty", list.del_front());
   TEST_ERROR("list is empty", list.del_rear());
   TEST_ERROR("list is empty", list.del_after(pix));

   // DblLinkList: prepend, empty, front, const operator()
   // DLLPix: copy constructor, is_clear, operator Bool, operator*,
   //   operator!, set_clear, first

   for (int i = 1; i <= NUM; ++i) {

      if (list.length() == 1)
         ASSERT(list.front() == list.rear());

      pix = list.prepend(i);
      ASSERT(list.front() == *pix && *pix == list(pix) && list(pix) == i &&
             pix.is_valid() && !pix.is_clear() && pix && list.owns(pix) &&
             !list.empty() && list.length() == i);

      pix.set_clear();
      ASSERT(pix.is_valid() && pix.is_clear() && !pix);

      pix.first();
      ASSERT(list.front() == *pix && *pix == list(pix) && list(pix) == i &&
             pix.is_valid() && !pix.is_clear() && pix && list.owns(pix) &&
             !list.empty() && list.length() == i);

      pix.last();
      ASSERT(list.rear() == *pix && *pix == list(pix) &&
             pix.is_valid() && !pix.is_clear() && pix && list.owns(pix) &&
             !list.empty() && list.length() == i);
   }

   Mcout << "Prepend [1," << NUM << "]: " << list << endl;

   // test pix is clear errors
   diffPix = diffList.prepend(1);
   TEST_ERROR("pixes index different containers", if (pix == diffPix) {});
   pix.set_clear();
   ASSERT(pix.is_clear() && !pix);
   TEST_ERROR("lhs pix is clear", if (pix == diffPix) {});
   TEST_ERROR("rhs pix is clear", if (diffPix == pix) {});
   TEST_ERROR("pix is clear", list(pix) = 5);
   const DblLinkList<int>& constList = list;
   TEST_ERROR("pix is clear", if (constList(pix) == 5) {});
   DBG(TEST_ERROR("pix is clear", pix.next()));
   DBG(TEST_ERROR("pix is clear", pix.prev()));
   DBG(TEST_ERROR("pix is clear", ++pix));
   DBG(TEST_ERROR("pix is clear", --pix));
   TEST_ERROR("pix is clear", if (*pix == 5) {});
   TEST_ERROR("pix is clear", list.owns(pix));
   TEST_ERROR("pix is clear", list.ins_before(pix,13));
   TEST_ERROR("pix is clear", list.del(pix));

   check_copy(list);

   // DblLinkList: remove_front, owns, length
   // DLLPix: is_valid
   for (i = NUM; i >= 1; --i) {

      if (list.length() == 1)
         ASSERT(list.front() == list.rear());

      pix.last();
      ASSERT(list.rear() == *pix && *pix == list(pix) &&
             pix.is_valid() && !pix.is_clear() && pix && list.owns(pix) &&
             !list.empty() && list.length() == i);

      pix.first();
      ASSERT(list.front() == *pix && *pix == list(pix) && list(pix) == i &&
             pix.is_valid() && !pix.is_clear() && pix && list.owns(pix) &&
             !list.empty() && list.length() == i);

      ASSERT(constList(pix) == i);

      ASSERT(list.remove_front() == i);
      ASSERT(list.length() == i-1 && !pix.is_valid());

      // test invalid pix errors
      TEST_ERROR("pix not in list", list.owns(pix,TRUE));
      DBGSLOW(TEST_ERROR("invalid pix", pix.next()));
      DBGSLOW(TEST_ERROR("invalid pix", pix.prev()));
      DBGSLOW(TEST_ERROR("invalid pix", ++pix));
      DBGSLOW(TEST_ERROR("invalid pix", --pix));
      DBGSLOW(TEST_ERROR("invalid pix", if (constList(pix) == 5) {}));
      DBGSLOW(TEST_ERROR("invalid pix", list(pix) = 5));
      DBGSLOW(TEST_ERROR("invalid pix", list.ins_after(pix,5)));
      DBGSLOW(TEST_ERROR("invalid pix", list.ins_before(pix,5)));

      if (list.empty()) {
         TEST_ERROR("list is empty", list.del(pix));
         TEST_ERROR("list is empty", list.del_after(pix));
      }
      else {
         DBGSLOW(TEST_ERROR("invalid pix", list.del(pix)));
         DBGSLOW(TEST_ERROR("invalid pix", list.del_after(pix)));
      }
   }
   ASSERT(list.empty() && list.length() == 0);

   Mcout << "Empty list: " << list << endl;

   // DblLinkList: append
   for (i = 1; i <= NUM; ++i) {

      if (list.length() == 1)
         ASSERT(list.front() == list.rear());

      pix = list.append(i);
      ASSERT(list.rear() == *pix && *pix == list(pix) && list(pix) == i &&
             pix.is_valid() && !pix.is_clear() && pix && list.owns(pix) &&
             !list.empty() && list.length() == i);

      pix.set_clear();
      ASSERT(pix.is_valid() && pix.is_clear() && !pix);

      pix.last();
      ASSERT(list.rear() == *pix && *pix == list(pix) && list(pix) == i &&
             pix.is_valid() && !pix.is_clear() && pix && list.owns(pix) &&
             !list.empty() && list.length() == i);

      pix.first();
      ASSERT(list.front() == *pix && *pix == list(pix) &&
             pix.is_valid() && !pix.is_clear() && pix && list.owns(pix) &&
             !list.empty() && list.length() == i);
   }

   Mcout << "Append [1," << NUM << "]: " << list << endl;

   // DblLinkList: remove_rear
   for (i = NUM; i >= 1; --i) {

      if (list.length() == 1)
         ASSERT(list.front() == list.rear());

      pix.first();
      ASSERT(list.front() == *pix && *pix == list(pix) &&
             pix.is_valid() && !pix.is_clear() && pix && list.owns(pix) &&
             !list.empty() && list.length() == i);

      pix.last();
      ASSERT(list.rear() == *pix && *pix == list(pix) && list(pix) == i &&
             pix.is_valid() && !pix.is_clear() && pix && list.owns(pix) &&
             !list.empty() && list.length() == i);

      ASSERT(list.remove_rear() == i);
      ASSERT(list.length() == i-1 && !pix.is_valid());
   }
   ASSERT(list.empty() && list.length() == 0);

   Mcout << "Empty list: " << list << endl;

   pix.set_clear();
   ASSERT(pix.is_clear() && !pix);

   DLLPix<int> pix1 = list.ins_after(pix,-12);
   ASSERT(list(pix1) == -12 &&
          *pix1 == -12 &&
          list.front() == -12 && list.rear() == -12 &&
          list.length() == 1 && !list.empty());

   list(pix1) = 139;
   ASSERT(list(pix1) == 139 &&
          *pix1 == 139 &&
          list.front() == 139 && list.rear() == 139 &&
          list.length() == 1 && !list.empty());

   DLLPix<int> pix3 = list.ins_after(pix1,-16);
   ASSERT(list(pix1) == 139 && list(pix3) == -16 &&
          *pix1 == 139 && *pix3 == -16 &&
          list.front() == 139 && list.rear() == -16 &&
          list.length() == 2 && !list.empty());

   DLLPix<int> pix2 = list.ins_after(pix1,197);
   ASSERT(list(pix1) == 139 && list(pix2) == 197 && list(pix3) == -16 &&
          *pix1 == 139 && *pix2 == 197 && *pix3 == -16 &&
          list.front() == 139 && list.rear() == -16 &&
          list.length() == 3 && !list.empty());

   DLLPix<int> pix0 = list.ins_before(pix1,-897);
   ASSERT(list(pix0) == -897 && list(pix1) == 139 && list(pix2) == 197 &&
          list(pix3) == -16 &&
          *pix0 == -897 && *pix1 == 139 && *pix2 == 197 && *pix3 == -16 &&
          list.front() == -897 && list.rear() == -16 &&
          list.length() == 4 && !list.empty());

   Mcout << "List: " << list << endl;

   // DLLPix: constructor, first, last
   DLLPix<int> headPix(list,1);
   pix.first();
   ASSERT(headPix == pix0 && pix == pix0 && *headPix == list.front());
   DLLPix<int> tailPix(list,-1);
   pix.last();
   ASSERT(tailPix == pix3 && pix == pix3 && *tailPix == list.rear());
   DLLPix<int> clearPix1(list);
   ASSERT(clearPix1.is_clear() && !clearPix1);
   DLLPix<int> clearPix2(list,0);
   ASSERT(clearPix2.is_clear() && !clearPix2);
   TEST_ERROR("illegal value for dir", DLLPix<int> badPix(list,4));

   // DLLPix: next, ++
   pix = pix0;
   ASSERT(pix == pix0 && pix);
   pix.next();
   ASSERT(pix == pix1 && pix);
   ASSERT(++pix == pix2 && pix);
   ASSERT(++pix == pix3 && pix);
   pix.next();
   ASSERT(pix.is_clear() && !pix);

   // DLLPix: prev, --
   pix = pix3;
   ASSERT(pix == pix3 && pix);
   ASSERT(--pix == pix2 && pix);
   pix.prev();
   ASSERT(pix == pix1 && pix);
   pix.prev();
   ASSERT(pix == pix0 && pix);
   --pix;
   ASSERT(pix.is_clear() && !pix);

   // deletion
   TEST_ERROR("invalid direction", list.del(pix0,0));

   list.del(pix0);
   ASSERT(pix0 == pix1 &&
          list(pix0) == 139 && list(pix1) == 139 && list(pix2) == 197 &&
          list(pix3) == -16 &&
          *pix0 == 139 && *pix1 == 139 && *pix2 == 197 && *pix3 == -16 &&
          list.front() == 139 && list.rear() == -16 &&
          list.length() == 3 && !list.empty());

   list.del(pix0,-1);
   ASSERT(pix0.is_clear() && !pix0 && !list.owns(pix1) &&
          list(pix2) == 197 && list(pix3) == -16 &&
          *pix2 == 197 && *pix3 == -16 &&
          list.front() == 197 && list.rear() == -16 &&
          list.length() == 2 && !list.empty());

   list.del(pix3,-1);
   ASSERT(pix3 == pix2 &&
          list(pix2) == 197 && list(pix3) == 197 &&
          *pix2 == 197 && *pix3 == 197 &&
          list.front() == 197 && list.rear() == 197 &&
          list.length() == 1 && !list.empty());

   TEST_ERROR("pix at end of list",list.del_after(pix2));

   list.del(pix3,1);
   ASSERT(!pix3 && !list.owns(pix2) &&
          list.length() == 0 && list.empty());

   for (i = -NUM; i < 0; ++i) // list = -1, ... -NUM
      list.prepend(i);
   ASSERT(list.length() == NUM && list.front() == -1 && list.rear() == -NUM);

   pix.first(); ASSERT(*pix == -1);
   pix.next(); ASSERT(*pix == -2);
   list.del(pix);
   ASSERT(list.length() == NUM-1 && *pix == -3);

   list.del_front();
   ASSERT(list.length() == NUM-2 && *pix == list.front() && *pix == -3);

   pix.last(); ASSERT(*pix == -NUM);
   pix.prev(); ASSERT(*pix == 1-NUM);
   list.del_rear();
   ASSERT(list.length() == NUM-3 && *pix == list.rear());

   TEST_ERROR("pix at end of list", list.del_after(pix));

   pix.prev(); ASSERT(*pix == 2-NUM && list.rear() == 1-NUM);
   list.del_after(pix);
   ASSERT(list.length() == NUM-4 && *pix == list.rear() && *pix == 2-NUM);

   ASSERT(list.front() == -3);
   pix.set_clear();
   list.del_after(pix);
   ASSERT(list.length() == NUM-5 && list.front() == -4);

   ASSERT(!list.empty());
   list.free();
   ASSERT(list.empty());

   // first and last for empty list
   ASSERT(!pix1.is_clear() && !pix1.is_valid() && !list.owns(pix1));
   pix1.first();
   ASSERT(pix1.is_clear());

   ASSERT(!pix2.is_clear() && !pix2.is_valid() && !list.owns(pix2));
   pix2.last();
   ASSERT(pix2.is_clear());

   
   return 0; // return success to shell
}
