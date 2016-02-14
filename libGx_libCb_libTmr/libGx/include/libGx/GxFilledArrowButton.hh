#ifndef GXFILLEDARROWBUTTON_INCLUDED
#define GXFILLEDARROWBUTTON_INCLUDED

#include <libGx/GxArrowButton.hh>

class GxFilledArrowButton : public GxArrowButton
{
public:
  GxFilledArrowButton(GxRealOwner *pOwner);
  virtual ~GxFilledArrowButton(void);

protected:
  void DrawButton(void);

};

#endif //GXFILLEDARROWBUTTON_INCLUDED
