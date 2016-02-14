// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef DisplayPref_H
#define DisplayPref_H



struct FloatPair {
   float x;
   float y;
   FloatPair(const float x1, const float y1) { x = x1; y = y1; }
   FloatPair() { x = 0; y = 0; }
};

class DotPostscriptPref;
class DotGraphPref;
class ASCIIPref;
class TreeVizPref;

class DisplayPref { 
public:
   // enumerated type used by all display types
   enum DisplayType { DotPostscriptDisplay, DotGraphDisplay, ASCIIDisplay,
		      TreeVizDisplay};
protected:
   const DisplayType pref;  
   // allow for default copy constructor--judged useful for this case
public:
   DisplayPref(DisplayType preference) : pref(preference) {}
   // not virtual -- just an "accessor method"
   DisplayType preference_type() const { return pref; } 
   // implement typecast conversions with error checks for all derived classes.
   virtual const DotPostscriptPref &typecast_to_DotPostscript() const;
   virtual const DotGraphPref &typecast_to_DotGraph() const;
   virtual const ASCIIPref &typecast_to_ASCII() const;
   virtual const TreeVizPref& typecast_to_treeViz() const;
};


class DotPostscriptPref : public DisplayPref {
public:
   // enumerated types specifc to DotPostscriptPref
   enum DisplayOrientation { DisplayLandscape, DisplayPortrait }; 
   enum DotRatioType { RatioFill, RatioDefault };
protected:
   FloatPair pageSize;   // in inches
   FloatPair graphSize;  // in inches
   DisplayOrientation orientation;  // an enumerated type
   DotRatioType ratio;
   // allow for default copy constructor--judged useful for this case
public:
   DotPostscriptPref();
   virtual const DotPostscriptPref &typecast_to_DotPostscript() const
                                                             { return *this; }
   const FloatPair &get_page_size() const { return pageSize; }           
   const FloatPair &get_graph_size() const { return graphSize; }
   DisplayOrientation get_orientation() const { return orientation; }
   DotRatioType get_ratio() const  { return ratio; } 
   void set_page_size(const FloatPair& size);
   void set_graph_size(const FloatPair& size);
   void set_orientation(const DisplayOrientation orientation);
   void set_ratio(const DotRatioType ratio);
};


class DotGraphPref : public DisplayPref {
public:
   DotGraphPref() : DisplayPref(DotGraphDisplay) {}
   virtual const DotGraphPref &typecast_to_DotGraph() const
                                                   { return *this; }
};


class ASCIIPref : public DisplayPref {
public:
   ASCIIPref() : DisplayPref(ASCIIDisplay) {}
   virtual const ASCIIPref &typecast_to_ASCII() const
                                             { return *this; }
};

class TreeVizPref : public DisplayPref {
public:
   TreeVizPref() : DisplayPref(TreeVizDisplay) {}
   virtual const TreeVizPref& typecast_to_treeViz() const
                                                   { return *this; }
};


extern ASCIIPref defaultDisplayPref;

#endif





