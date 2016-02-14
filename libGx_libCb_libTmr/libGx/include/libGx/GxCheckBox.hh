#ifndef GXCHECKBOX_INCLUDED
#define GXCHECKBOX_INCLUDED

#include <string>
#include <libGx/GxNoFocusButtonBase.hh>

enum GX_CHECKBOX_STATE{GX_CHECKBOX_CHECKED, GX_CHECKBOX_NOT_CHECKED,
		       GX_CHECKBOX_PARTIAL_CHECKED};

class GxCheckBox : public GxNoFocusButtonBase
{
public:
  GxCheckBox(GxRealOwner *pOwner, const char *pLabel = 0);
  virtual ~GxCheckBox(void);

  void SetLabel(const char *pLabel);

  GX_CHECKBOX_STATE State(void) const;
  void State(GX_CHECKBOX_STATE newState);

  CbVoidFO cb;

  virtual UINT GetDesiredWidth(void) const;
  virtual UINT GetDesiredHeight(void) const;

protected:
  virtual void DoAction(void);
  virtual void DrawButton(void);

  char label[GX_DEFAULT_LABEL_LEN];
  unsigned labelLen;
  unsigned labelPixLen;

  GX_CHECKBOX_STATE cbState;
};

#endif //GXCHECKBOX_INCLUDED
