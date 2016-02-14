#include <libGx/GxToolBarButton.hh>
#include <libGx/GxMainInterface.hh>

#include <libGx/GxToolBar.hh>

#include "GxDefines.hh"

// ***************************** start GxToolBarButton ****************
GxToolBarButton::GxToolBarButton(GxToolBar *pTBar) :
  pOwner(pTBar), active(true), toolGroupID(0), state(false), buttonType(false)
{
  toolText[0] = '\0';
  pTBar->AddButton(this);
}

GxToolBarButton::GxToolBarButton(GxToolBar *pTBar, char *pImage) :
  pOwner(pTBar), active(true), toolGroupID(0), state(false), buttonType(false)
{
  toolText[0] = '\0';
  SetImage(pImage);
  pTBar->AddButton(this);
}

GxToolBarButton::GxToolBarButton(GxToolBar *pTBar, char **pImage) :
  pOwner(pTBar), active(true), toolGroupID(0), state(false), buttonType(false)
{
  toolText[0] = '\0';
  SetImage(pImage);
  pTBar->AddButton(this);
}

GxToolBarButton::GxToolBarButton(GxToolBar *pTBar, const char *pToolText) :
  pOwner(pTBar), active(true), toolGroupID(0), state(false), buttonType(false)
{
  unsigned junkLength = 0;
  GxSetLabel(toolText, GX_DEFAULT_LABEL_LEN, pToolText, junkLength);
  pTBar->AddButton(this);
}

GxToolBarButton::GxToolBarButton(GxToolBar *pTBar, char *pImage,
				 const char *pToolText) :
  pOwner(pTBar), active(true), toolGroupID(0), state(false), buttonType(false)
{
  SetImage(pImage);
  unsigned junkLength = 0;
  GxSetLabel(toolText, GX_DEFAULT_LABEL_LEN, pToolText, junkLength);
  pTBar->AddButton(this);
}


GxToolBarButton::GxToolBarButton(GxToolBar *pTBar, char ** pImage,
				 const char *pToolText) :
  pOwner(pTBar), active(true), toolGroupID(0), state(false), buttonType(false)
{
  SetImage(pImage);
  unsigned junkLength = 0;
  GxSetLabel(toolText, GX_DEFAULT_LABEL_LEN, pToolText, junkLength);
  pTBar->AddButton(this);
}

GxToolBarButton::~GxToolBarButton(void)
{}

void GxToolBarButton::Active(bool newState)
{
  active = newState;
}

bool GxToolBarButton::Active(void) const
{
  return active;
}

void GxToolBarButton::State(bool newState)
{
  state = newState;
}

bool GxToolBarButton::State(void) const
{
  return state;
}

void GxToolBarButton::ButtonType(bool stateButton)
{
  buttonType = stateButton;
}

bool GxToolBarButton::ButtonType(void) const
{
  return buttonType;
}

void GxToolBarButton::ToolGroupID(unsigned newGroupID)
{
  toolGroupID = newGroupID;
}

unsigned GxToolBarButton::ToolGroupID(void) const
{
  return toolGroupID;
}

// ******************** end GxToolBarButton ***********************

// ******************** start GxToolBarButtonWin ********************

GxToolBarButtonWin::GxToolBarButtonWin(GxRealOwner *pOwner,
				       GxToolBarButton &rTButton) :
  GxNoFocusButtonBase(pOwner), rButton(rTButton), pTTip(0),
  ttTimer(50, CbVoidMember<GxToolBarButtonWin>(this, &GxToolBarButtonWin::TimerCB) )
{
  width = GX_TOOLBAR_BUTTON_SIZE;
  height = GX_TOOLBAR_BUTTON_SIZE;
  //std::cout << "GxToolBarButtonWin::GxToolBarButtonWin" << std::endl;
}

GxToolBarButtonWin::~GxToolBarButtonWin(void)
{
  //cout << "GxToolBarButtonWin::~GxToolBarButtonWin" << endl;
  delete pTTip;
  pTTip = 0;
}

void GxToolBarButtonWin::Create(void)
{
  /*
  std::cout << "GxToolBarButtonWin::Create" << std::endl;
  std::cout << "width: " << width << " height: " << height << endl;
  */
  GxNoFocusButtonBase::Create();
  if( !rButton.ImageCreated() )
    rButton.CreateImage(dInfo, xWin);
}

void GxToolBarButtonWin::DoAction(void)
{
  //destroy the tooltip
  PointerOut();
  //pressed has been set to false in the button base
  XClearWindow(dInfo.display, xWin);
  DrawButton();

  rButton.cb();
}

void GxToolBarButtonWin::DrawButton(void)
{
  Draw3dBorder(0,0, width, height, !pressed);
  int ix = ( ((int)width) - ((int)rButton.ImageWidth()) )/2;
  int iy = ( ((int)height) - ((int)rButton.ImageHeight()) )/2;
  rButton.DrawImage(dInfo, vData, xWin, ix, iy);
}

void GxToolBarButtonWin::PointerIn(void)
{
  //activate the timer to raise the tool tip
  dInfo.rMainInterface.ActivateTimer(ttTimer);
}

void GxToolBarButtonWin::PointerOut(void)
{
  ttTimer.DeActivate(); //either this will be active or pTTip will be valid
  delete pTTip;
  pTTip = 0;
}

void GxToolBarButtonWin::TimerCB(void)
{
  if(pTTip || (rButton.toolText[0] == '\0')) return;

  pTTip = new GxToolTip(this, rButton.toolText);
  
  pTTip->Place();
  pTTip->Create();
  int xRoot, yRoot;
  Window junkWin;
  XTranslateCoordinates(dInfo.display, xWin, dInfo.rootWin, width/2, height,
			&xRoot, &yRoot, &junkWin);
  pTTip->Display(xRoot,yRoot);
}

// ******************** end GxToolBarButtonWin ********************
