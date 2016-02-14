#ifndef GXTOGGLEBUTTON_INCLUDED
#define GXTOGGLEBUTTON_INCLUDED

#include <string.h>

#include <libGx/GxNoFocusButtonBase.hh>

const unsigned GX_TOGGLE_BUTTON_LABEL_LEN = 256;
class GxToggleButton : public GxNoFocusButtonBase
{
public:
  GxToggleButton(GxRealOwner *pOwner, const char *pLabel = NULL);
  virtual ~GxToggleButton(void);

  void SetLabel(const char *pLabel);

  bool State(void) const;
  virtual void State(bool newState);

  //this is called on every toggle. (set and unset)
  //why did I program this origionally to only be called on set (and not reset)
  CbVoidFO cb;

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

protected:
  virtual void DoAction(void);
  virtual void DrawButton(void);

  bool state;
  char label[GX_TOGGLE_BUTTON_LABEL_LEN];
};

#endif //GXTOGGLEBUTTON_INCLUDED
