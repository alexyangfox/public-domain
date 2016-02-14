// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/***************************************************************************
  Description  : Generate template files
***************************************************************************/

#include <basics.h>
#include <Array.h>
#include <Array2.h>
#include <DynamicArray.h>
#include <DblLinkList.h>
#include <BoolArray.h>
#include <MEnum.h>
#include <StatData.h>

RCSID("MLC++, $RCSfile: MCoreTemplates.c,v $ $Revision: 1.4 $")

// Display using the call and then using "<<" so both will be generated
//   immediately.  If we just us "<<" we get a second phase of display()
//   template instantiations
#define DISP(str, var) \
   Mcout << str << " "; var.display(Mcout); \
   Mcout << endl << var << endl;


static void intArray()
{
   Array<int> intArray1(0, 10, 3);
   intArray1.sort();
   int i, j;
   i = intArray1.max(j);
   i = intArray1.min(j);   
   DISP("intArray", intArray1);

   Array2<int> intArray2(0, 10, 3);
   DISP("intArray2", intArray2)

   ASSERT(intArray2(2,2) != intArray2(3,3));
   ASSERT(intArray1 == intArray1);
   ASSERT(!(intArray1 != intArray1));
}   

// float is rare in MLC++, but used in the C4.5 interface
static void floatArray() 
{
   Array<float> intArray1(3);
   intArray1.read_bin(Mcin);
}   

static void real2Array()
{
   Array2<Real> realArray1(0,10, 1.2);
   Array2<Real> realArray2(0, 10, 1.2);
   Array2<Real> realArray3(0, 0, 5, 5, 1.2);
   DISP("real2Array", realArray1);

   const Array2<Real>& realArray4 = realArray1;
   ASSERT(realArray2(2,2) != realArray4(3,3));

}   

static void realDArray()
{
   DynamicArray<Real> realArray1(10, 1.2);
   DynamicArray<Real> realArray1B(2);
   realArray1.append(realArray1B);
   realArray1B.truncate(1);
   realArray1[2] = 3;
   Array<Real> realArray2(0, 5, 1.1);
   realArray1.sort();
   Real i; 
   int j;
   i = realArray1.max(j);
   i = realArray1.min(j);
   realArray2 -= realArray1;
   ASSERT(!(realArray2 == realArray1));
   ASSERT(realArray2 != realArray1);
   realArray2 += realArray1;
   DISP("realDArray", realArray1);
}   

static void MStringDArray()
{
   DynamicArray<MString> MStringArray1(10, "foo");
   DynamicArray<MString> MStringArray1B(2);
   MStringArray1.append(MStringArray1B);
   MStringArray1B.truncate(1);
   MStringArray1[2] = 3;
   Array<MString> MStringArray2(0, 5);
   MStringArray1.sort();
   MString i; 
   int j;
   i = MStringArray1.max(j);
   i = MStringArray1.min(j);
   ASSERT(!(MStringArray2 == MStringArray1));
   ASSERT(MStringArray2 != MStringArray1);
   MStringArray2 += MStringArray1;
   DynamicArray<MString> MStringArray3(-1, 5, "foo");
   DISP("MStringDArray", MStringArray1);
}   

static void boolArray()
{
   BoolArray ba(3);
   DISP("bool array", ba);
}   

// For LEDA stuff that is really all voids.
static void voidArray()
{
   Array<void*> voidArray1(0, 10, NULL);
   DISP("void*Array", voidArray1);
}   


static void ptrArray()
{
   PtrArray<Array<int>*> ptrArray1(0, 10);
   DISP("ptrArray1", ptrArray1);

   PtrArray<Array2<int>*> ptrArray2(0,10);
   DISP("ptrArray2", ptrArray2);

   Array<Array<int>*> ptrArray3(0,10);
   DISP("ptrArray3", ptrArray3);

   Array<Array2<int>*> ptrArray4(0,10);
   DISP("ptrArray3", ptrArray3);
}


// For hash tables.
static void unsignedDArray()
{
   DynamicArray<unsigned char> charArray1(0, 10);
   DynamicArray<unsigned char> charArray2(10);
   DISP("DcharArray", charArray1);
}   


static void mstrings()
{
   MString foo("ronny");
   Mcout << foo << endl;
}

static void menum_dyn_array()
{
   DynamicArray<MEnumField> fields(2);
}   

static void array_char()
{
   Array<char> ac(2);
   Array<char> ac1(2);   

   Array<char> ac2(ac1,ctorDummy);
   Array<char> ac3(2,2,'a');
   Array<char> ac4(2,2);
   ASSERT(ac == ac1);
}


TEMPL_GENERATOR(MCoreTemplates) 
{
    Array<StatData> a(2,3);
    Array<char*>    ac(2);


    Mcout << "Generating templates for MCore" << endl;

    intArray();
    floatArray();
    realDArray();
    MStringDArray();
    real2Array();
    boolArray();
    voidArray();
    ptrArray();
    unsignedDArray();
    mstrings();
    menum_dyn_array();
    array_char();
   return 0; // return success to shell
}   
