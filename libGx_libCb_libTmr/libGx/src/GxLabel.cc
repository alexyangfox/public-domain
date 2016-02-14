#include <libGx/GxLabel.hh>

GxLabel::GxLabel(GxRealOwner *pOwner, const char *pLabel) :
  GxWin(pOwner)
{
  GxSetLabel(labelText, GX_LONG_LABEL_LEN, pLabel, strLength);
}

GxLabel::~GxLabel(void)
{}

void GxLabel::SetLabel(const char *pNewLabel)
{
  GxSetLabel(labelText, GX_LONG_LABEL_LEN, pNewLabel, strLength);
}

UINT GxLabel::GetDesiredWidth(void) const
{
  return (UINT)XTextWidth(dInfo.pDefaultFont, labelText, strLength) + 2;
}

UINT GxLabel::GetDesiredHeight(void) const
{
  return (dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent + 2);
}

void GxLabel::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      DrawLabel();
}

void GxLabel::GetWindowData(XSetWindowAttributes &winAttributes,
			    ULINT &valueMask)
{
  winAttributes.event_mask = ExposureMask;
  valueMask |= CWEventMask;
}

void GxLabel::DrawLabel(void)
{
  XSetForeground(dInfo.display, vData.textGC, dInfo.labelTextPix);
  XSetFont(dInfo.display, vData.textGC, dInfo.pDefaultFont->fid);
  XDrawString(dInfo.display, xWin, vData.textGC, 0, dInfo.pDefaultFont->ascent,
	      labelText, strLength);
}
