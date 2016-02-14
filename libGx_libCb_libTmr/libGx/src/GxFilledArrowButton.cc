#include <libGx/GxFilledArrowButton.hh>

GxFilledArrowButton::GxFilledArrowButton(GxRealOwner *pOwner) :
  GxArrowButton(pOwner)
{}

GxFilledArrowButton::~GxFilledArrowButton(void)
{}

void GxFilledArrowButton::DrawButton(void)
{
  Draw3dBorder(0,0,width,height, !pressed);
  DrawFilledArrow(3,3, width-6, bDir);
}
