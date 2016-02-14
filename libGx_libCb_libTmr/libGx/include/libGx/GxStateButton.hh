#ifndef GXSTATEBUTTON_INCLUDED
#define GXSTATEBUTTON_INCLUDED

#include <libCb/CbCallback.hh>

#include <libGx/GxRealOwner.hh>
#include <libGx/GxNoFocusButtonBase.hh>

class GxStateButton : public GxNoFocusButtonBase
{
public:
  GxStateButton(GxRealOwner *pOwner, const char *pLabel = NULL);
  virtual ~GxStateButton(void);

  bool State(void);
  void State(bool newState);

  void SetLabel(const char * pLabel);

  CbOneFO<bool> cb;

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

protected:
  virtual void DoAction(void);
  virtual void DrawButton(void);

  char label[GX_DEFAULT_LABEL_LEN];
  unsigned labelLen;
  bool state;
};

#endif //GXSTATEBUTTON_INCLUDED
