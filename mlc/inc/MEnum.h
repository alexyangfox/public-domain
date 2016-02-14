// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _MEnum_h
#define _MEnum_h 1

#include <basics.h>
#include <DynamicArray.h>

struct MEnumField {
   MString name;
   int value;
   MEnumField() : name(), value(-1) {}
};

class MEnum {   
   DynamicArray<MEnumField> fields;

 public:
   void OK() const;
   MEnum() : fields(0) { }
   MEnum(const MString& name, int value);
   MEnum(const MEnum& other);
   ~MEnum() { OK(); };

   MEnum& operator<<(const MEnum& other);
   MEnum& operator=(const MEnum src) {fields = src.fields; return *this;}

   void display(MLCOStream& out, Bool full=FALSE) const;
   int value_from_name(const MString& name) const;
   MString name_from_value(int value) const;
   Bool check_value(int value) const { return name_from_value(value) != ""; }
};
   
DECLARE_DISPLAY(MEnum);

#endif



