#include <libGx/GxCheckBox.hh>

#include "GxDefines.hh"
const unsigned checkBoxSize = 15;

GxCheckBox::GxCheckBox(GxRealOwner *pOwner, const char *pLabel) :
  GxNoFocusButtonBase(pOwner),
  labelLen(0), labelPixLen(0), cbState(GX_CHECKBOX_NOT_CHECKED)
{
  SetLabel(pLabel);
}

GxCheckBox::~GxCheckBox(void)
{}

void GxCheckBox::SetLabel(const char *pLabel)
{
  GxSetLabel(label, GX_DEFAULT_LABEL_LEN, pLabel, labelLen);

  if(labelLen)
    labelPixLen = XTextWidth(dInfo.pDefaultFont, label, labelLen);
  else
    labelPixLen = GX_BORDER_WD;
}

GX_CHECKBOX_STATE GxCheckBox::State(void) const
{
  return cbState;
}

void GxCheckBox::State(GX_CHECKBOX_STATE newState)
{
  cbState = newState;
  if( Created() )
    {
      if(newState == GX_CHECKBOX_NOT_CHECKED)
	XClearWindow(dInfo.display, xWin);
      DrawButton();
    };
}

UINT GxCheckBox::GetDesiredWidth(void) const
{
  return labelPixLen + checkBoxSize + 10;
}

UINT GxCheckBox::GetDesiredHeight(void) const
{
  return dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent + 5;
}

void GxCheckBox::DoAction(void)
{
  switch(cbState)
    {
    case GX_CHECKBOX_CHECKED:
      cbState = GX_CHECKBOX_NOT_CHECKED;
      //when we change state we need to clear the window to erase the check
      if(Created())
	XClearWindow(dInfo.display, xWin);
      break;
    case GX_CHECKBOX_NOT_CHECKED:
      cbState = GX_CHECKBOX_CHECKED;
      break;
    case GX_CHECKBOX_PARTIAL_CHECKED:
      cbState = GX_CHECKBOX_CHECKED;
      break;
    default:
      return; //unknown state. major error
    };

  DrawButton();
  cb();
}

void GxCheckBox::DrawButton(void)
{
  if( !Created() ) return;

  int boxXStart = 5;
  int boxYStart = (height-checkBoxSize)/2;
  Draw3dBorder(boxXStart, boxYStart, checkBoxSize, checkBoxSize, false);

  XSetForeground(dInfo.display, vData.borderGC, dInfo.whitePix);
  XFillRectangle(dInfo.display, xWin, vData.borderGC,
		 boxXStart+GX_BORDER_WD, boxYStart+GX_BORDER_WD,
		 checkBoxSize-2*GX_BORDER_WD, checkBoxSize-2*GX_BORDER_WD);

  //cout << label << " " << labelLen << endl;
  int xTextStart = 10+checkBoxSize;
  int yTextStart = 2+dInfo.pDefaultFont->ascent;

  if(active)
    XSetForeground(dInfo.display, vData.textGC, dInfo.blackPix);
  else
    XSetForeground(dInfo.display, vData.textGC, dInfo.darkBorderPix);


  XDrawString(dInfo.display, xWin, vData.textGC, xTextStart, yTextStart,
	      label, labelLen);

  switch(cbState)
    {
    case GX_CHECKBOX_CHECKED:
      DrawCheck(boxXStart, boxYStart-8, checkBoxSize+checkBoxSize/2,
		dInfo.blackPix);
      break;
    case GX_CHECKBOX_PARTIAL_CHECKED:
      DrawCheck(boxXStart, boxYStart-8, checkBoxSize+checkBoxSize/2,
		dInfo.darkBorderPix); //hack in terms of color
      break;
    default: //GX_CHECKBOX_NOT_CHECKED
      break;
    }; 
}
