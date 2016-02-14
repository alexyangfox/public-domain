// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _MOption_h
#define _MOption_h 1

#include <basics.h>
#include <MEnum.h>


class MOption {
 protected:
   MString name;
   MString defValue;
   MString help;
   Bool nuisance;
   
   static MEnum promptLevelEnum;
   static MString optionDump;
   static MLCOStream *optionDumpOut;

   virtual Bool result_OK(const MString&) const = 0;
   virtual void prompt_user(const MString& value, Bool full=FALSE) const;
   
   MOption(const MString& name, const MString& defVal,
	   const MString& help, Bool nuisance);

 public:
   enum PromptLevel { requiredOnly, promptBasic, promptAll };
   static MEnum boolEnum;
   virtual void OK() const;
   static PromptLevel get_prompt_level() { return promptLevel; }
   static void set_prompt_level(PromptLevel pl) { promptLevel = pl; }
   static MString get_option_dump() { return optionDump; }
   static void set_option_dump(const MString& str) { optionDump = str; }
   virtual MString get(Bool emptyOK = FALSE) const;
   static void initialize();
   static void finish();

 protected:
   static PromptLevel promptLevel;
};


class StringOption : public MOption {
 protected:
   virtual void prompt_user(const MString& value, Bool full = FALSE) const;
   virtual Bool result_OK(const MString&) const { return TRUE; }

 public:
   StringOption(const MString& name, const MString& defVal,
		const MString& help="", Bool nuisance = FALSE);
};


class IntOption : public MOption {
 protected:
   virtual void prompt_user(const MString& value, Bool full = FALSE) const;
   virtual Bool result_OK(const MString& result) const;
   
 public:
   IntOption(const MString& name, const MString& defVal,
	     const MString& help="", Bool nuisance = FALSE);
};

class IntRangeOption : public IntOption {
 protected:
   int lowerBound;
   int upperBound;
   virtual Bool result_OK(const MString& result) const;
   virtual void prompt_user(const MString& value, Bool full = FALSE) const;

 public:
   virtual void OK() const;
   IntRangeOption(const MString& name, const MString& defVal,
		  int lower, int upper,
		  const MString& help="", Bool nuisance = FALSE);
};

class RealOption : public MOption {
 protected:
   virtual Bool result_OK(const MString& result) const;
   virtual void prompt_user(const MString& value, Bool full = FALSE) const;
   
 public:
   RealOption(const MString& name, const MString& defVal,
	     const MString& help, Bool nuisance = FALSE);
};

class RealRangeOption : public RealOption {
 protected:
   Real lowerBound;
   Real upperBound;
   virtual Bool result_OK(const MString& result) const;
   virtual void prompt_user(const MString& value, Bool full = FALSE) const;

 public:
   virtual void OK() const;
   RealRangeOption(const MString& name, const MString& def,
		   Real lower, Real upper,
		   const MString& hlp="", Bool nuisance = FALSE);
};

class MEnumOption : public MOption {
 protected:
   const MEnum& enumSpec;
   virtual Bool result_OK(const MString& result) const;
   virtual void prompt_user(const MString& value, Bool full = FALSE) const;

 public:
   MEnumOption(const MString& name, const MEnum& mEnum, const MString& defVal,
	       const MString& help="", Bool nuisance=FALSE);
};


#endif




   
