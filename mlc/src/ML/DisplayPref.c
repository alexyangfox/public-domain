// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Provides preferences for graphical & non-graphical output.
                 DisplayOptions is a class containing one element:
		   type, which is enumerated currently to contain one of:
		   1. ASCIIDisplay
		   2. DotPostscriptDisplay
		   3. DotGraphDisplay
		 A class is derived from DisplayOptions for each and
		   the printing routine casts to the appropriate type
		   and uses whatever methods it needs.  The advantage of
		   subclassing over unions is that we can share information
		   cleanly.  For example, DotPostscriptDisplay and
		   DotGraphDisplay can both inherit from DotDisplay which
		   contains information common to both.
                 Most combinations are valid, but not all.  FileStream and
                   PrinterStream are valid for all.  XStream is valid only with
                   DotGraphDisplay.  This is almost an orthogonal
		   separation.  We could force it to be orthogonal by
		   defining some behavior to other combinations (e.g. XStream
		   on ASCII will show an xterm window with the information in
		   ASCII, and DotPostscriptDisplay will display the
		   information using ghostview so you know how the
		   printer output will actually look), but there
		   does not seem to be much motivation for forcing this.
  Assumptions  : 
  Comments     : Every output routine with more than one DisplayPref
                   should expect a DisplayPref.
                 This should not be confused with the "device" type
		   information contained in MLCStream.
  Complexity   : Users cannot change their PreferenceType.  They
                   can change various preferences, but are not allowed
		   to change the general category.
  Enhancements : PrinterStream is not currently implemented.  This
                   should be done by sending the file to the printer when
		   the destructor is invoked.
		 Eventually should add in more preferences.
  History      : Dave Manley                                       9/23/93
                   Initial revision
***************************************************************************/
#include <basics.h>  
#include <DisplayPref.h>

RCSID("MLC++, $RCSfile: DisplayPref.c,v $ $Revision: 1.9 $")

// default is ASCII for backwards compatibility with display.
ASCIIPref defaultDisplayPref;


/***************************************************************************
  Description : Handy function to get the string name from the preference.
  Comments    : 
***************************************************************************/
static MString GetDisplayTypeMString(const DisplayPref::DisplayType dt)
{
   switch (dt) {
   case DisplayPref::DotGraphDisplay: return("DotGraphDisplay");
   case DisplayPref::DotPostscriptDisplay: return("DotPostscriptDisplay");
   case DisplayPref::ASCIIDisplay: return("ASCIIDisplay");
   default:    err << "Unknown DisplayType " << dt << fatal_error;
   }
   // dummy return value to avoid compiler warnings.
   return("DUMMY");
}


/***************************************************************************
  Description : Cast to specific preferences from base DisplayPref class.
  Comments    : Uses "fat interface."
                Cast is by default illegal.  Can be overridden by
                  derived class
***************************************************************************/
const DotPostscriptPref& DisplayPref::typecast_to_DotPostscript() const
{
   err << "Attempt to cast DisplayPref to DotPostscriptPref illegally"
       " from " << GetDisplayTypeMString(preference_type())<< fatal_error;
   // dummy return value, keeps your compiler nice & happy
   return  *(DotPostscriptPref *)NULL_REF;
}

const DotGraphPref& DisplayPref::typecast_to_DotGraph() const
{
   err << "Attempt to cast DisplayPref to DotGraphPref illegally"
      " from " << GetDisplayTypeMString(preference_type())<< fatal_error;
   // dummy return value, keeps your compiler nice & happy
   return  *(DotGraphPref *)NULL_REF;
}

const ASCIIPref& DisplayPref::typecast_to_ASCII() const
{
   err << "Attempt to cast DisplayPref to ASCIIPref illegally"
       " from " << GetDisplayTypeMString(preference_type())<< fatal_error;
   // dummy return value, keeps your compiler nice & happy
   return  *(ASCIIPref *)NULL_REF;
}

const TreeVizPref& DisplayPref::typecast_to_treeViz() const
{
   err << "Attempt to cast DisplayPref to TreeVizPref illegally"
       " from " << GetDisplayTypeMString(preference_type())<< fatal_error;
   // dummy return value, keeps your compiler nice & happy
   return  *(TreeVizPref *)NULL_REF;
}


/***************************************************************************
  Description : Constructor that assigns default preferences.
  Comments    : Preferences are then specified by set routines.
***************************************************************************/
DotPostscriptPref::DotPostscriptPref()
                 : DisplayPref(DotPostscriptDisplay)
{
   pageSize = FloatPair(defaultPageX,defaultPageY);
   graphSize = FloatPair(defaultGraphX, defaultGraphY);
   orientation = DisplayLandscape;
   ratio = RatioFill;
}


/***************************************************************************
  Description : Methods that change options.
  Comments    : 
***************************************************************************/
void DotPostscriptPref::set_page_size(const FloatPair& size) 
{   
   DBG(if (size.x < 0 || size.y < 0)
       err << "DotPostscriptPref::set_page_size: illegal page size "
       << "x,y: " << size.x << "," << size.y << fatal_error);
   pageSize = size;
}


void DotPostscriptPref::set_graph_size(const FloatPair& size)
{
   DBG(if (size.x < 0 || size.y < 0)
       err<<"DotPostscriptPref::set_graph_size: illegal graph size "
       << "x,y: " << size.x << "," << size.y
       << fatal_error); 
   graphSize = size;
}


void DotPostscriptPref::set_orientation(const DisplayOrientation
					new_orientation)
{
   orientation = new_orientation;
}


void DotPostscriptPref::set_ratio(const DotRatioType new_ratio)
{
   ratio = new_ratio;
}
