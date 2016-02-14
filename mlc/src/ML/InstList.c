// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The InstanceList is derived from InstanceBag.
                   Lists have additional operations where an order of the
		   elements is required.
  Assumptions  : File format follows Quinlan (pp. 81-83) EXCEPT:
                     1)  , : | \ . do not appear in names
		     2) . at end of lists is optional
		     3) a word that appears before the labels are
		        enumerated, that is preceded by \ is
			interpreted as a modifier.  Currently, the
			only implemented modifier is "weighted", which
			indicates that the list will be weighted.
		 This means that labels are assumed to be nominal type
		    for read_names().
  Comments     : Line numbers given are the result of '\n', not
                    wrapping of lines.
		 "continuous" is not a legal name for the first nominal 
		    attribute value; it is reserved to indicate a
		    continuous(RealAttrInfo) attribute.
  Complexity   : InstanceList::read_names() takes time proportional 
		    to the number of characters in the file that it reads 
		    + the number of labels + the number of attributes 
		    + the number of nominal attribute values.
		 InstanceList::read_data_line() takes time 
		    proportional to the number of characters in the portion 
		    of the file that it reads + the total number of possible 
		    attribute values for the _Instance.
		 InstanceList::read_data() takes time proportional to 
		    the number of instances * the complexity of 
		    read_data_line() + complexity of
		    free_instances().
		 InstanceList(const MString&, const MString&,
		                      const MString&) takes complexity of 
		    InstanceList::read_names() + complexity of 
		    InstanceList::read_data().
		 InstanceList::split_prefix() takes O(numInSplit).
		 InstanceList::unite() takes O(N), where N is
		    the number of instances in the given list.
  Enhancements : Cause fatal_error() if read_names() is called by methods
                   other than the constructor or
		   CtrInstanceList::read_names()
                 Extend read_attributes to handle AttrInfo other
                   than NominalAttrInfo and RealAttrInfo.
                 Expand capability of input function to:
		   1) allow . in name if not followed by a space
                   2) allow , : | and \ in names if preceded by a backslash
		   (this would mimic Quinlan) 
		 Use lex to do the lexical analysis of the input file.  
		    This will be critical if the syntax becomes more 
		    complicated.
  History      : Richard Long                                       9/13/93
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <InstList.h>
#include <mlcIO.h>
#include <GetOption.h>
#include <MRandom.h>

RCSID("MLC++, $RCSfile: InstList.c,v $ $Revision: 1.41 $")

const MString defaultDataExt(".data");
const MString defaultNamesExt(".names");

const MString removeUnknownHelp = "Remove instances with unknown values "
      "when reading datafiles from disk";
const MString corruptUnknownRateHelp = "Probability of corrupting an "
    "attribute value by changing it to unknown";

/***************************************************************************
  Description : Constructor.
  Comments    : Protected method.
                This method is called by the CtrInstanceList
		  constructor that takes file names as parameters because
		  virtual methods do not work in constructors, so
		  read_data() and read_names() would not perform properly
		  for the CtrInstanceList.
***************************************************************************/
InstanceList::InstanceList() : InstanceBag()
{}


/***************************************************************************
  Description : Reads Schema from "names" file and instances from "data" file.
                The second constructor just reads the names file and
                  no instances.  Useful for assign or data generation.
		See read_names() and read_data() for file format and
		  other details.
  Comments    : Constructor.
***************************************************************************/
InstanceList::InstanceList(const MString& file,
			   const MString& namesExtension,
			   const MString& dataExtension)
  :InstanceBag()
{
   read_names(file + namesExtension);
   read_data(file + dataExtension);
}

InstanceList::InstanceList(const MString& file, ArgDummy /*dummyArg*/,
			   const MString& namesExtension)
  : InstanceBag()
{
   read_names(file + namesExtension);
}   


/***************************************************************************
  Description : Constructors.  Destructor.
  Comments    :
***************************************************************************/
InstanceList::InstanceList(const SchemaRC& schemaRC)
   : InstanceBag(schemaRC) {}


/***************************************************************************
  Description : The List has the same contents as the Bag, but there is
		  now a fixed order to the instances.
		The List has the same instance ownership status as the Bag.
  Comments    : Gets ownership of bag.
***************************************************************************/
InstanceList::InstanceList(InstanceBag*& bag)
{
   copy(bag);
   DBG(ASSERT(bag == NULL));
}


InstanceList::~InstanceList() {}


/***************************************************************************
  Description : Returns an InstanceRC correpsonding to the first instance
                   in the list.
		The instance is deleted from the list.
  Comments    : 
***************************************************************************/
InstanceRC InstanceList::remove_front()
{
   if (no_instances())
      err << "InstanceList::remove_front: list is empty"
	  << fatal_error;
   Pix pix = first();
   return InstanceBag::remove_instance(pix);   
}


/***************************************************************************
  Description : Reads the label names from the file.  Initializes
                   the labelInfo of the Schema.
		The list of labels consists of a comma-separated
		  "words" (as described in read_word())
  Comments    : Assumes labels are nominal attributes.
                AttrInfo is given name "Label" and num 0.
		Takes time proportional to the number of characters in the 
		   file it reads + the  number of labels.
***************************************************************************/
static AttrInfo* read_labels(MLCIStream& namesFile, int& line)
{
   DblLinkList<MString>* labelVals = new DblLinkList<MString>;
   skip_white_comments(namesFile, line);
   while (namesFile.peek() != '.' && namesFile.peek() != '|' 
	  && namesFile.peek() != '\n') {
      if (namesFile.peek() == ':')
	 err << "InstList.c::read_labels: unexpected ':' in list of "
	        "labels line " << line << " of "
	     << namesFile.description() << fatal_error;
      skip_white_comments(namesFile, line);
      if (namesFile.peek() == ',')
	 namesFile.get();               // skip over comma
      skip_white_comments(namesFile, line);
      MStringRC label = read_word(namesFile, line, FALSE);
      labelVals->append(*label.read_rep());
   }
   AttrInfo* labels = new NominalAttrInfo("Label", labelVals);
   if (namesFile.peek() == '.')
      namesFile.get();                // skip over period
   
   return labels;
}


/***************************************************************************
  Description : Reads a list of "words" (as described in read_word()) that 
                  correspond to nominal values.
                Returns a pointer to the newly created NominalAttrInfo,
		  whose values were read.
  Comments    : The attrName, and first attrValue must be read first to
		  determine the type of the AttrInfo (e.g. continuous), 
		  so they are passed as parameters.  
		Takes time proportional to the number of characters in the 
		  portion of the file that it reads + the number of 
		  attribute values.
		Gets ownership of attrVal1.
		Temp is necessary because GNU's append requires a
		  non-const argument.
***************************************************************************/
static NominalAttrInfo* read_nominal_values(MLCIStream& namesFile, 
					    const MString& attrName,
					    const MString& attrVal1,
					    int& line)
{      
   DblLinkList<MString>* attrVals = new DblLinkList<MString>;
   MString temp(attrVal1);
   attrVals->append(temp);
   while (namesFile.peek() != '.' && namesFile.peek() != '\n'
	  && namesFile.peek() != '|' && namesFile.peek() != EOF) {
      if (namesFile.peek() == ',')
	 namesFile.get(); // skip over comma
      MStringRC attrValue = read_word(namesFile, line, FALSE);
      attrVals->append(*attrValue.read_rep());
   }
   NominalAttrInfo* nai = new NominalAttrInfo(attrName, attrVals);

   if (namesFile.peek() == '.')
      namesFile.get();  // skip over '.'
   return nai;
}


/***************************************************************************
  Description : Reads the information about the attributes that the
                  instances in the bag can have and adds appropriate
		  AttrInfo to the AttrInfo* list.
		File should have format described in read_names().
  Comments    : Currently causes fatal_error if continuous is encountered
		  as attribute type.
		Takes time proportional to the number of characters in the 
		  file it reads + the number of attributes 
		  + the number of nominal attribute values.
***************************************************************************/
static DblLinkList<AttrInfoPtr>* read_attributes(MLCIStream& namesFile,
					       int& line)
{
   MStringRC attrName;
   DblLinkList<AttrInfoPtr>* attrInfos = new DblLinkList<AttrInfoPtr>;
  
   skip_white_comments(namesFile, line);
   while (namesFile.peek() != EOF) {
      attrName = read_word(namesFile, line, FALSE);
      if (namesFile.get() != ':')
	 err << "InstList.c::read_attributes: expecting a ':' following "
	        "attribute label " << attrName << " on line " 
	     << line << " of " << namesFile.description() << fatal_error;
    
      // read first attrValue to determine attrType
      MStringRC attrValue = read_word(namesFile, line, FALSE);
    
      AttrInfo* ai;
      if (attrValue == "continuous") {
	 ai = new RealAttrInfo(*attrName.read_rep());
	 if (namesFile.peek() == '.')
	    namesFile.get();        // skip over period
      } else 
	 ai = read_nominal_values(namesFile, *attrName.read_rep(),
				  *attrValue.read_rep(), line);
      AttrInfoPtr aip(ai);
      attrInfos->append(aip);
      skip_white_comments(namesFile, line);
   }
   return attrInfos;
}


/***************************************************************************
  Description : Reads modifiers that appear before the labels.
  Comments    : The modifiers are \ preceded "words" (as described in
		  mlcIO.c::read_word()).
		Modifiers are separated by white space.
		All modifiers must appear before the first label.
		See apply_modifiers() for more details.
***************************************************************************/
DblLinkList<MString>* InstanceList::read_modifiers(MLCIStream& namesFile,
						   int& line)
{
   DblLinkList<MString>* modifiers = new DblLinkList<MString>;
   skip_white_comments(namesFile, line);
   while (namesFile.peek() == '\\') {
      namesFile.ignore();
      MStringRC modifier = read_word(namesFile, line, TRUE);
      modifiers->append(*modifier.read_rep());
   }
   return modifiers;
}


/***************************************************************************
  Description : Applys the given modifier to the instance list.
  Comments    : "weighted" sets the list to contain weighted instances.
                This means that the data file should have weights.
***************************************************************************/
void InstanceList::apply_modifiers(const DblLinkList<MString>& modifiers)
{
   for (DLLPix<MString> pix(modifiers, 1); pix; pix.next()) {
      if (modifiers(pix) == "weighted")
	 set_weighted(TRUE);
      else
	 err << "InstanceList::apply_modifiers: Unknown modifier "
	     << modifiers(pix) << fatal_error;
   }
}

   
/***************************************************************************
  Description : Reading the names file is split into three phases:
                  1. Reading and applying the modifiers. (see read_modifiers())
                  2. Reading the label names
		  3. Reading the attribute information.
		The label names are comma separated words.
		The attribute information begins with a colon terminated 
		word, and a list of comma separated words.
		Lists of labels or attribute values end with a period 
		  or the first EOLN that is not preceded by a comma.
		The information that is read is stored in a
		   Schema to which the private instance variable schema.
  Comments    : Protected method.
                The line counter is used to give more informative error
		   messages.
***************************************************************************/
void InstanceList::read_names(const MString& file) 
{
   MLCIStream namesFile(file);
   int line = 1;    // keeps track of current line number
   DblLinkList<MString>* modifiers = read_modifiers(namesFile, line);
   AttrInfo* labelInfo = read_labels(namesFile, line);
   apply_modifiers(*modifiers);
   DblLinkList<AttrInfoPtr>* attrInfos = read_attributes(namesFile, line);
   SchemaRC schemaRC(attrInfos, labelInfo);
   ASSERT(attrInfos == NULL);
   ASSERT(labelInfo == NULL);
   
   set_schema(schemaRC);
   delete modifiers;
}


/***************************************************************************
  Description : Reads a line of data from the data file.
                A line of data consists of a list of comma separated
		  attributes in the order matching the Schema
		  followed by a comma, and then the label followed 
		  by an optional period.  All of this information must 
		  appear on one newline terminated line.
		The nominal attributes are represented by words (as 
		  described in read_word()).  Continuous attributes
		  are floats.
                Creates a new _Instance to store the information.  For each
                  attribute, checks the attribute type, reads in the
		  appropriate type, and stores the data in the _Instance.
		Adds _Instance to the Bag.
  Comments    : Protected method.
                This method should be called only after the 
		  Schema has been set (such as by read_names()).
		A | will cause the rest of a line to be ignored.
***************************************************************************/
void InstanceList::read_data_line(MLCIStream& dataFile, int& line)
{
  skip_white_comments(dataFile, line);
  const SchemaRC& instSchema = get_schema();
  InstanceRC instance(instSchema);
  if (get_weighted()) {
     Real wt;
     dataFile >> wt;
     if (skip_white_comments_same_line(dataFile, line) == FALSE)
	err << "InstanceList::read_data_line: Unexpected end of "
	       "line following weight " << wt << " on line " << line
	    << " of " << dataFile.description() << fatal_error;
     if (dataFile.peek() != ',')
	err << "InstanceList::read_data_line: Expecting ',' "
	       " instead of '" << dataFile.peek() << "' following weight "
	    << wt << " on line " << line << " of " << dataFile.description()
	    << fatal_error;
     dataFile.get();  // ignore comma
     instance.set_weight(wt);
  }
  AttrValue_ value;
  for (int i = 0; i < num_attr(); i++) {
    const AttrInfo& ai = instSchema.attr_info(i);
    value = ai.read_attr_value(dataFile, line);
    GLOBLOG(7, "Attribute " << i << " (" << ai.name() << ")=" <<
          ai.attrValue_to_string(value) << endl);
    instance[i] = value;
    if (dataFile.peek() ==  ',')
      dataFile.get();        // skip over comma
  }
  
  const AttrInfo& labelInfo = label_info();
  value = labelInfo.read_attr_value(dataFile, line);
  instance.set_label(value);
  
  if (dataFile.peek() == '.') {
    dataFile.get();        // skip over period; should be end of line
    if (skip_white_comments_same_line(dataFile, line))
      err << "InstanceList::read_data_line: illegal file format line "
	  << line << " of " << dataFile.description() << ".  Only comments "
	     "& whitespace can follow a '.' on a line in the data file"
	  << fatal_error;
  } else if (dataFile.peek() != '\n' && dataFile.peek() != '|')
    err << "InstanceList::read_data_line: illegal file format line "
	<< line << " of " << dataFile.description() << ".  Only comments & "
	   "whitespace or a '.' can follow the label value on a line"
	<< fatal_error;
  else
    skip_white_comments(dataFile, line);
  add_instance(instance);
}


/***************************************************************************
  Description : This function reads in the data (Instances) for the bag.
                The data consists of lines described above in
		   read_data_line().
		Data is read until the end of the file is reached.
		Deletes any old instances in the list.
  Comments    : Line counter is used to give more informative error 
                   messages.
***************************************************************************/
void InstanceList::read_data(const MString& file) 
{
   static Bool removeUnknownInstances = get_option_bool("REMOVE_UNKNOWN_INST",
	  FALSE, removeUnknownHelp, TRUE); 

   static Real corruptToUnknownRate =  get_option_real_range(
      "CORRUPT_UNKNOWN_RATE", 0.0, 0.0, 1.0, corruptUnknownRateHelp, TRUE);

   remove_all_instances();
   MLCIStream dataFile(file);
   int line = 1;

   GLOBLOG(1, "Reading " << file);
   // Must skip over white comments in file, because there may be
   //   zero instances after comment, in which case the peek must see EOF.
   skip_white_comments(dataFile, line);
   while (dataFile.peek() != EOF) {
      if (line % 100 == 1)
	 GLOBLOG(1, '.' << flush);
      read_data_line(dataFile, line);
   }
   if (!removeUnknownInstances)
      GLOBLOG(1, " done." << endl);
   else {
      int num = num_instances();
      GLOBLOG(1, ' '); // show we finished reading
      remove_inst_with_unknown_attr();
      int newNum = num_instances();
      if (newNum < num)
	 GLOBLOG(1, "Removed " << num - newNum << " instances." << endl);
      else
         GLOBLOG(1, "done." << endl);
   }
   if (no_instances())
      Mcerr << "Warning: no instances in file " << file << endl;

   static int unknownSeed = -1;
   static MRandom mrandomForUnknowns;
   if (corruptToUnknownRate > 0) {
      if (unknownSeed == -1) { // get seed first time
	 unknownSeed = get_option_int("UNKNOWN_RATE_SEED", 4975497,
			      "Seed for unknown value corruption", TRUE);
	 mrandomForUnknowns.init(unknownSeed);
      }
      corrupt_values_to_unknown(corruptToUnknownRate, mrandomForUnknowns);
   }
}


/***************************************************************************
  Description : Returns a list with the first "numInst" instances
                   removed from this list.
  Comments    : 
***************************************************************************/
InstanceList* InstanceList::split_prefix(int numInSplit)
{
   if (numInSplit < 0 || numInSplit > num_instances())
      err << "InstanceList::split_prefix: Cannot remove "
	  << numInSplit << " instances from a list with "
	  << num_instances() << " instances" << fatal_error;
   InstanceList* bag;
   bag = &create_my_type(get_schema())->cast_to_instance_list();

   for (int i = 0; i < numInSplit; i++) {
      InstanceRC instance = remove_front();
      bag->add_instance(instance);
   }
   ASSERT(bag->num_instances() == numInSplit);
   return bag;
}


/***************************************************************************
  Description : Appends the instances from the given list to this list.
  Comments    : Gets ownership of and deletes the given list.
***************************************************************************/
void InstanceList::unite(InstanceList*& bag)
{
   if (bag == this)
      err << "InstanceList::unite: Cannot unite bag with itself"
	  << fatal_error;
   while (!bag->no_instances()) {
      InstanceRC instance = bag->remove_front();
      add_instance(instance);
   }
   delete bag;
   bag = NULL;
}


/***************************************************************************
  Description : Returns a reference to a InstanceList.
  Comments    :
***************************************************************************/
InstanceList& InstanceList::cast_to_instance_list()
{
   return *this;
}

const InstanceList& InstanceList::cast_to_instance_list() const
{
   return *this;
}


/***************************************************************************
  Description : Returns a pointer to a new InstanceList with the
                   given instance info.
  Comments    : The new list will own its instances and its instance info.
***************************************************************************/
InstanceBag* InstanceList::create_my_type(const SchemaRC& schemaRC) const
{
   return new InstanceList(schemaRC);
}


/***************************************************************************
  Description : Returns a pointer to a list that has the same contents
                   as this list, with a random ordering of the instances.
  Comments    : mrandom and index default to NULL.
                The MRandom parameter allows for duplication of results.
***************************************************************************/
InstanceList* InstanceList::shuffle(MRandom* mrandom,
				    InstanceBagIndex* index) const
{
   return &shuffle_(mrandom, index)->cast_to_instance_list();
}
