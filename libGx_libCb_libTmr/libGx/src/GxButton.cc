#include <libGx/GxButton.hh>

#include "GxDefines.hh"

GxButton::GxButton(GxRealOwner *pOwner, const char *pLabel) :
  GxFocusButtonBase(pOwner)
{
  GxSetLabel(label, GX_DEFAULT_LABEL_LEN, pLabel, labelLen);
}

GxButton::GxButton(GxRealOwner *pOwner, const char *pLabel,
		   const GxGeomControl &rGCont) :
  GxFocusButtonBase(pOwner)
{
  GxSetLabel(label, GX_DEFAULT_LABEL_LEN, pLabel, labelLen);
  SetGeomControl(rGCont);
}

GxButton::~GxButton(void)
{}

void GxButton::SetLabel(const char *pLabel)
{
  GxSetLabel(label, GX_DEFAULT_LABEL_LEN, pLabel, labelLen);

  if( Created() )
    {
      XClearWindow(dInfo.display, xWin);
      DrawButton();
    };
}

UINT GxButton::GetDesiredWidth(void) const
{
  //button is drawn border border_width_gap, focus line, border_width_gap text
  //the +1 is for line line drawn around the text when active
  return XTextWidth(dInfo.pDefaultFont, label, labelLen) + GX_BORDER_WD*6 + 1;
}

UINT GxButton::GetDesiredHeight(void) const
{
  //the +1 is for line line drawn around the text when active
  return (dInfo.pDefaultFont->ascent) + (dInfo.pDefaultFont->descent)
    + GX_BORDER_WD*6 + 1;
}

void GxButton::DrawButton(void)
{
  if(pressed)
    {
      //we want a slightly darker background;
      XSetForeground(dInfo.display, vData.borderGC, dInfo.recessedPix);
      XFillRectangle(dInfo.display, xWin, vData.borderGC, 0,0, width,height);
    };

  Draw3dBorder(0,0, width,height, !pressed);

  if(haveFocus)
    {
      XSetForeground(dInfo.display, vData.borderGC, dInfo.blackPix);
      //draw a border around the text area of the button
      XDrawRectangle(dInfo.display, xWin, vData.borderGC,
		     GX_BORDER_WD*2, GX_BORDER_WD*2,
		     width - 1 - GX_BORDER_WD*4, height - 1 - GX_BORDER_WD*4);
    };

  //hack; I really don't want to re-calculate textX and textY and textWidth
  //every time I go through here
  if(labelLen != 0)
    {
      int textWidth = XTextWidth(dInfo.pDefaultFont, label, labelLen);
      textX = (width - textWidth)/2;
      textY = ((height - ((dInfo.pDefaultFont->max_bounds).ascent +
			  (dInfo.pDefaultFont->max_bounds).descent))/2)
	+ (dInfo.pDefaultFont->max_bounds).ascent;
 
      if(active)
	XSetForeground(dInfo.display, vData.textGC, dInfo.labelTextPix);
      else
	XSetForeground(dInfo.display, vData.textGC, dInfo.unActiveLabelTextPix);
   
      XDrawString(dInfo.display, xWin, vData.textGC, textX, textY, label,
		  labelLen);
    };
}

void GxButton::DoAction(void)
{
  XClearWindow(dInfo.display, xWin);
  DrawButton();
  cb();
}
