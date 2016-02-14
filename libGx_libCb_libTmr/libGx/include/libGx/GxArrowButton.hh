#ifndef GXARROWBUTTON_INCLUDED
#define GXARROWBUTTON_INCLUDED

#include <libGx/GxInc.hh>
#include <libGx/GxNoFocusButtonBase.hh>

class GxArrowButton : public GxNoFocusButtonBase
{
public:
  GxArrowButton(GxRealOwner *pOwner);
  virtual ~GxArrowButton(void);

  void SetDirection(GX_DIRECTION newDir);

  CbVoidFO cb;

protected:
  virtual void DrawButton(void);
  virtual void DoAction(void);

  GX_DIRECTION bDir; //the direction the button points
};

#endif //GXARROWBUTTON_INCLUDED
