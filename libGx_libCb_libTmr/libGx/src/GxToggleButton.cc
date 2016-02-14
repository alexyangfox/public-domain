#include <libGx/GxToggleButton.hh>

GxToggleButton::GxToggleButton(GxRealOwner *pOwner, const char *pLabel) :
  GxNoFocusButtonBase(pOwner), state(false)
{
  unsigned junkLength = 0;
  GxSetLabel(label, GX_TOGGLE_BUTTON_LABEL_LEN, pLabel, junkLength);
}

GxToggleButton::~GxToggleButton(void)
{}

void GxToggleButton::SetLabel(const char *pLabel)
{
  unsigned junkLength = 0;
  GxSetLabel(label, GX_TOGGLE_BUTTON_LABEL_LEN, pLabel, junkLength);
}

bool GxToggleButton::State(void) const
{
  return state;
}

void GxToggleButton::State(bool newState)
{
  if(state == newState)
    return;

  state = newState;
  
  if(Created())
    {
      //XClearWindow?
      DrawButton();
    };
}

UINT GxToggleButton::GetDesiredWidth(void) const
{
  return dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent + 6 +
    XTextWidth(dInfo.pDefaultFont, label, strlen(label));
}

UINT GxToggleButton::GetDesiredHeight(void) const
{
  //two pixel border
  return dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent + 4;
}

void GxToggleButton::DoAction(void)
{
  if(state)
    state = false;
  else
    state = true;

  DrawButton();
  cb();
}

void GxToggleButton::DrawButton(void)
{
  int fontHeight = dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent;
  int drawY = ((height - fontHeight)/2) + dInfo.pDefaultFont->ascent;

  if(active)
    XSetForeground(dInfo.display, vData.textGC, dInfo.labelTextPix);
  else
    XSetForeground(dInfo.display, vData.textGC, dInfo.unActiveLabelTextPix);
  //leaves a 2 pixel border to the right of the state box
  XDrawString(dInfo.display, xWin, vData.textGC, 4+fontHeight,drawY,
	      label, strlen(label));

  //now draw the state box
  if(pressed)//the user is currnetly interacting with the widget
    Draw3dBorder(2, drawY - dInfo.pDefaultFont->ascent,
		 fontHeight, fontHeight, state);
  else
    Draw3dBorder(2, drawY - dInfo.pDefaultFont->ascent,
		 fontHeight, fontHeight, !state);
}
