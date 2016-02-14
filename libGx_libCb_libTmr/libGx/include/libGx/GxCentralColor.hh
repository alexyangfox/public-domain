#ifndef GXCENTRALCOLOR_INCLUDED
#define GXCENTRALCOLOR_INCLUDED

#include <list>

#include <libGx/GxInc.hh>
#include <libGx/GxErrorSink.hh>

//the GxCentralColor class represents a color which must be allocated
//for the application to function correctly.  All Aplications have four
//GxCentralColors allocated by default.
//(the GxDisplay allocates them internally) the background color, the raised
//color, the dark border color and the light border color.  This class
//just retains all of the alternative names for a color before allocation.
class GxDisplay;

class GxCentralColor
{
public:
  GxCentralColor(void);
  //GxCentralColor(UINT redC, UINT greenC, UINT blueC);
  //GxCentralColor(void);//need constructors for hex and floating point names
  //GxCentralColor(void);//need constructors for hex and floating point names
  virtual ~GxCentralColor(void);

  virtual GxCentralColor* Clone(void) const;

  void ClearColorList(void);
  //adds a color to the end of the desired color list
  void AddColor(const char *pAltName);
  //void AddAlt();
  //void AddAlt();

  //returns false if all of the alternative names fail or no colors were given
  bool Allocate(GxErrorSink &rErrorSink, GxDisplay &rDisplay, Colormap tCMap);

  //after it has been alocated, this will return the pix allocated
  const XColor& GetColor(void);
protected:

  class GxColorHolder
  {
  public:
    GxColorHolder(void);
    virtual ~GxColorHolder(void);

    virtual GxColorHolder *Clone(void) const = 0;
    virtual bool AllocColor(GxErrorSink &rErrorSink, GxDisplay &rDisplay,
			    Colormap tCMap, XColor &rColor) = 0;
  };

  class GxNamedColorHolder : public GxColorHolder
  {
  public:
    GxNamedColorHolder(const char *pName);
    virtual ~GxNamedColorHolder(void);

    virtual GxColorHolder *Clone(void) const;
    virtual bool AllocColor(GxErrorSink &rErrorSink, GxDisplay &rDisplay,
			    Colormap tCMap, XColor &rColor);
  private:
    char colorName[GX_DEFAULT_LABEL_LEN];
  };

  bool foundColor;
  XColor myColor;
  //the first element in color list is the one we *really* want
  std::list<GxColorHolder*> colorList; //?slist?
};

#endif //GXCENTRALCOLOR_INCLUDED
