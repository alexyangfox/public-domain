#include <libGx/GxStateButton.hh>

GxStateButton::GxStateButton(GxRealOwner *pOwner, const char *pLabel) :
  GxNoFocusButtonBase(pOwner), state(false)
{
  GxSetLabel(label, GX_DEFAULT_LABEL_LEN, pLabel, labelLen);
}

GxStateButton::~GxStateButton(void)
{}

bool GxStateButton::State(void)
{
  return state;
}

void GxStateButton::State(bool newState)
{
  state = newState;
  if(Created())
    {
      XClearWindow(dInfo.display, xWin);
      DrawButton();
    };
}

void GxStateButton::SetLabel(const char * pLabel)
{
  GxSetLabel(label, GX_DEFAULT_LABEL_LEN, pLabel, labelLen);
}

UINT GxStateButton::GetDesiredWidth(void) const
{
  UINT boxSize = dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent;
  return (UINT)XTextWidth(dInfo.pDefaultFont, label, labelLen) + 6
    + boxSize + 1;
}

UINT GxStateButton::GetDesiredHeight(void) const
{
  //boxsize + 6
  return dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent + 6;
}

void GxStateButton::DoAction(void)
{
  state = !state;
  DrawButton();
  cb(state);
}

void GxStateButton::DrawButton(void)
{
  Draw3dBorder(0,0, width,height, !pressed);

  XSetFont(dInfo.display, vData.borderGC, dInfo.pDefaultFont->fid);
  int fontHeight = dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent;
  int drawY = ((height - fontHeight)/2) + dInfo.pDefaultFont->ascent;

  if(active)
    XSetForeground(dInfo.display, vData.borderGC, dInfo.labelTextPix);
  else
    XSetForeground(dInfo.display, vData.borderGC, dInfo.unActiveLabelTextPix);

  XDrawString(dInfo.display, xWin, vData.borderGC, 3,drawY,
	      label, labelLen);

  Draw3dBorder(width - fontHeight - 3, drawY - dInfo.pDefaultFont->ascent,
	       fontHeight, fontHeight, !state);
}
