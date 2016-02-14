// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _CN2Inducer_h
#define _CN2Inducer_h 1

#include <BaseInducer.h>
#include <Inducer.h>
#include <MLCStream.h>
#include <LogOptions.h>

#define CN2_INDUCER 25

class CN2Inducer : public BaseInducer {
private:   
   MString pgmName;
   MString redirectString;

public:
   static MString defaultPgmName;
   static MString defaultRedirectString;

   CN2Inducer(const MString& description, 
	      const MString& thePgmName = defaultPgmName);

   virtual ~CN2Inducer();
   virtual int class_id() const { return CN2_INDUCER; }
   virtual Real train_and_test(InstanceBag* trainingSet,
			       const InstanceBag& testList);
   virtual void set_pgm_name(const MString& name) { pgmName = name; }
   virtual MString get_pgm_name() const { return pgmName; }
   virtual void set_redirect_string(const MString& val);
   virtual MString get_redirect_string() const
   { return redirectString; }
   
   virtual void set_user_options(const MString& prefix);
};
#endif





