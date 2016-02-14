#ifndef GXBUTTON_INCLUDED
#define GXBUTTON_INCLUDED

#include <libGx/GxFocusButtonBase.hh>

class GxButton : public GxFocusButtonBase
{
public:
  GxButton(GxRealOwner *pOwner, const char *pLabel = NULL);
  GxButton(GxRealOwner *pOwner, const char *pLabel, const GxGeomControl &rGCont);
  ~GxButton(void);

  void SetLabel(const char *pLabel);

  CbVoidFO cb;

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

protected:
  virtual void DrawButton(void);
  virtual void DoAction(void);

  char label[GX_DEFAULT_LABEL_LEN];
  unsigned labelLen;
  //hack; do we really need this?
  //the coordinates used for drawing the text
  int textX, textY;
};

#endif //GXBUTTON_INCLUDED
