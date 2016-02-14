#include <string.h>

#include <libGx/GxMenuItems.hh>

#include <libGx/GxMenuBar.hh>
#include "GxDefines.hh"

// *************** Start GxMenuItem ****************
GxMenuItem::~GxMenuItem(void)
{}

GxMenuItem::GxMenuItem(GxMenuItemOwner *pMenu) :
  GxWin( &pMenu->GetRealOwnerObject() ), pItemOwner(pMenu)
{}
// *************** End GxMenuItem ****************

// *************** Start GxBaseMenuOption ****************
GxBaseMenuOption::GxBaseMenuOption(GxMenuItemOwner *pOwner, const char * pLabel) :
  GxMenuItem(pOwner), active(true), selected(false)
{
  height = dInfo.pMenuFont->ascent + dInfo.pMenuFont->descent
    + GX_BORDER_WD*2 + 2;

  unsigned junkLen = 0;
  GxSetLabel(label, GX_DEFAULT_LABEL_LEN, pLabel, junkLen);
}

GxBaseMenuOption::~GxBaseMenuOption(void)
{}

void GxBaseMenuOption::SetLabel(const char* pLabel)
{
  unsigned junkLen = 0;
  GxSetLabel(label, GX_DEFAULT_LABEL_LEN, pLabel, junkLen);
}

void GxBaseMenuOption::SetActive(bool newStatus)
{
  active = newStatus;
  if(Created())
    Draw();
}

void GxBaseMenuOption::GetWindowData(XSetWindowAttributes &winAttributes,
				 ULINT &valueMask)
{
  valueMask |= CWEventMask;
  winAttributes.event_mask = ExposureMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | LeaveWindowMask;
}

void GxBaseMenuOption::HandleEvent(const XEvent &rEvent)
{
  switch(rEvent.type)
    {
    case Expose:
      if(rEvent.xexpose.count == 0)
	Draw();
      break;

    case EnterNotify:
      if( pItemOwner->GetButtonHeldMode() )
	{
	  selected = true;
	  Draw();
	};
      break;

    case ButtonPress:
      selected = true;
      Draw();
      pItemOwner->SetButtonHeldMode(true);
      break;

    case LeaveNotify:
      if(selected)
	{
	  selected = false;
	  XClearWindow(dInfo.display, xWin);
	  Draw();
	};
      break;

    case ButtonRelease:
      selected = false;
      pItemOwner->EndMenuEventGrab();
      DoCallback();
      break;

    default:
      //do nothing
      break;
    };
}

UINT GxBaseMenuOption::GetMinimumWidth(void)
{
  return GX_BORDER_WD*2 + 2 +
    (UINT)XTextWidth(dInfo.pMenuFont, label, strlen(label));
}

// *************** End GxBaseMenuOption ****************

// *************** Start GxMenuOption ****************
GxMenuOption::GxMenuOption(GxMenuItemOwner *pOwner, const char * pLabel) :
  GxBaseMenuOption(pOwner, pLabel)
{}

GxMenuOption::~GxMenuOption(void)
{}

void GxMenuOption::Draw(void)
{
  if(active && selected)
    Draw3dBorder(0,0, width,height, true);

  if(!active)
    XSetForeground(dInfo.display, vData.menuGC, dInfo.unActiveLabelTextPix);

  XDrawString(dInfo.display, xWin, vData.menuGC, GX_BORDER_WD+1,
	      GX_BORDER_WD+1+dInfo.pMenuFont->ascent, label, strlen(label));

  if(!active)
    XSetForeground(dInfo.display, vData.menuGC, dInfo.labelTextPix);
}


void GxMenuOption::DoCallback(void)
{
  cb();
}
// *************** End GxMenuOption ****************

// *************** Start GxMenuCheckOption ****************
GxMenuCheckOption::GxMenuCheckOption(GxMenuItemOwner *pOwner, const char * pLabel) :
  GxBaseMenuOption(pOwner, pLabel), checked(false)
{}

GxMenuCheckOption::~GxMenuCheckOption(void)
{}

bool GxMenuCheckOption::Checked(void) const
{
  return checked;
}

void GxMenuCheckOption::Checked(bool newCheck)
{
  if(newCheck == checked)
    return;

  checked = newCheck;
  if(Created())
    Draw();
}

void GxMenuCheckOption::Draw(void)
{
  if(active && selected)
    Draw3dBorder(0,0, width,height, true);

  if(!active)
    XSetForeground(dInfo.display, vData.menuGC, dInfo.unActiveLabelTextPix);

  if(checked)
    DrawCheck(GX_BORDER_WD+1, GX_BORDER_WD+1, 
	      dInfo.pMenuFont->ascent + dInfo.pMenuFont->descent,
	      dInfo.blackPix);

  XDrawString(dInfo.display, xWin, vData.menuGC,
	      (GX_BORDER_WD + 1 + dInfo.pMenuFont->ascent +
	       dInfo.pMenuFont->descent + 1),
	      GX_BORDER_WD+1+dInfo.pMenuFont->ascent, label, strlen(label));


  if(!active)
    XSetForeground(dInfo.display, vData.menuGC, dInfo.labelTextPix);
}

UINT GxMenuCheckOption::GetMinimumWidth(void)
{
  return GX_BORDER_WD*2 + 2 + 
    (UINT)XTextWidth(dInfo.pMenuFont, label, strlen(label)) +
    dInfo.pMenuFont->ascent + dInfo.pMenuFont->descent;
}

void GxMenuCheckOption::DoCallback(void)
{
  if(checked)
    checked = false;
  else
    checked = true;

  cb(checked);
}
// *************** End GxMenuCheckOption ****************

// *************** Start GxMenuDivider ****************
GxMenuDivider::GxMenuDivider(GxMenuItemOwner *pOwner) :
  GxMenuItem(pOwner)
{
  height = GX_SPACE_INC;
}

GxMenuDivider::~GxMenuDivider(void)
{}

void GxMenuDivider::Draw(void)
{
  unsigned sideOffset = GX_SPACE_INC;
  if(width < sideOffset*2)
    sideOffset = 0;

  //we just draw two horizontal lines
  XSetForeground(dInfo.display, vData.borderGC, dInfo.darkBorderPix);
  int cY = ((int)(height/2)) - 1;
  XDrawLine(dInfo.display, xWin, vData.borderGC, sideOffset,cY, width-sideOffset,cY);
  
  cY++;
  XSetForeground(dInfo.display, vData.borderGC, dInfo.lightBorderPix);
  XDrawLine(dInfo.display, xWin, vData.borderGC, sideOffset,cY, width-sideOffset,cY);
  XSetForeground(dInfo.display, vData.borderGC, dInfo.lightBorderPix);
}

void GxMenuDivider::GetWindowData(XSetWindowAttributes &winAttributes,
				 ULINT &valueMask)
{
  valueMask |= CWEventMask;
  winAttributes.event_mask = ExposureMask;
}

void GxMenuDivider::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      Draw();
}

UINT GxMenuDivider::GetMinimumWidth(void)
{
  return 1; //we can be as small as needed
}

// *************** End GxMenuDivider ****************

// *************** Start GxSubMenu ****************
GxSubMenu::GxSubMenu(GxMenuItemOwner *pOwner) :
  GxMenuItem(pOwner), active(true), selected(false)
{
  label[0] = '\0';
}

GxSubMenu::~GxSubMenu(void)
{}

void GxSubMenu::SetLabel(const char* pLabel)
{
  unsigned junkLen = 0;
  GxSetLabel(label, GX_DEFAULT_LABEL_LEN, pLabel, junkLen);
}

void GxSubMenu::Draw(void)
{
  XDrawString(dInfo.display, xWin, vData.menuGC, GX_BORDER_WD,
	      GX_BORDER_WD+dInfo.pMenuFont->ascent, label, strlen(label));
}


void GxSubMenu::GetWindowData(XSetWindowAttributes &winAttributes,
			      ULINT &valueMask)
{
  valueMask |= CWEventMask;
  winAttributes.event_mask = ExposureMask | ButtonPressMask |
    ButtonReleaseMask;
}

void GxSubMenu::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      Draw();
}

UINT GxSubMenu::GetMinimumWidth(void)
{
  //hack;
  return 1;
}
// *************** End GxSubMenu ****************

