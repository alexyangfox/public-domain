#include <libGx/GxArrowButton.hh>

GxArrowButton::GxArrowButton(GxRealOwner *pOwner) :
  GxNoFocusButtonBase(pOwner), bDir(GX_UP)
{}

GxArrowButton::~GxArrowButton(void)
{}

void GxArrowButton::SetDirection(GX_DIRECTION newDir)
{
  if(newDir != bDir)
    {
      bDir = newDir;
      if(Created())
	DrawButton();
    };
}

void GxArrowButton::DrawButton(void)
{
  if(width < height)
    Draw3dArrow(0,0, width, bDir, !pressed);
  else
    Draw3dArrow(0,0, height, bDir, !pressed);
}

void GxArrowButton::DoAction(void)
{
  DrawButton();
  cb();
}

