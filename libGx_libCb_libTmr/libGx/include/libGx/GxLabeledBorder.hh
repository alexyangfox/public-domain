#ifndef GXLABELEDBORDER_INCLUDED
#define GXLABELEDBORDER_INCLUDED

#include <libGx/GxInc.hh>
#include <libGx/GxOwnerWin.hh>

class GxLabeledBorder : public GxOwnerWin
{
public:
  GxLabeledBorder(GxRealOwner *pOwner, const char *pLabel = NULL);
  virtual ~GxLabeledBorder(void);

  void SetLabel(const char *pLabel);

  //desired size is to size around desired size of first child, if it exists
  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

  virtual void HandleEvent(const XEvent &rEvent);
  //we overload this to place children inside of where border is drawn
  virtual void PlaceChildren(void);

protected:
  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);
  char label[GX_DEFAULT_LABEL_LEN];
  unsigned labelLen; //the length of the string in label
  unsigned labelPixLen; //the length of the string in pixels
};

#endif //GXLABELEDBORDER_INCLUDED
