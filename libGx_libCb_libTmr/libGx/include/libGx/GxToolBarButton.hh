#ifndef GXTOOLBARBUTTON_INCLUDED
#define GXTOOLBARBUTTON_INCLUDED

#include <libGx/GxImageObject.hh>
#include <libCb/CbCallback.hh>
#include <libTmr/TmrTimer.hh>

class GxToolBar;
class GxToolBarButton : public GxImageObject
{
public:
  GxToolBarButton(GxToolBar *pTBar);
  GxToolBarButton(GxToolBar *pTBar, char *pImage);
  GxToolBarButton(GxToolBar *pTBar, char ** pImage);
  GxToolBarButton(GxToolBar *pTBar, const char *pToolText);
  GxToolBarButton(GxToolBar *pTBar, char *pImage, const char *pToolText);
  GxToolBarButton(GxToolBar *pTBar, char ** pImage, const char *pToolText);
  virtual ~GxToolBarButton(void);

  //so it knows how to draw me without my having to have an extensive
  //interface
  friend class GxToolBarButtonWin;

  void Active(bool newState);
  bool Active(void) const;

  //if true make the button depressed; if false make it raised
  //this does not call the callbacks
  void State(bool newState);
  bool State(void) const;

  void ButtonType(bool stateButton);
  bool ButtonType(void) const;

  void ToolGroupID(unsigned newGroupID);
  unsigned ToolGroupID(void) const;

  //called when depressed if we are a static button.
  CbVoidFO cb;
protected:

  GxToolBar *pOwner;

  char toolText[GX_DEFAULT_LABEL_LEN];
  bool active;
  
  unsigned toolGroupID; //defaults to 0.
  bool state;

  bool buttonType;
};

#include <libGx/GxNoFocusButtonBase.hh>
#include <libGx/GxToolTip.hh>

//created on demand by the toolbar dock to handle events
class GxToolBarButtonWin : public GxNoFocusButtonBase
{
public:
  GxToolBarButtonWin(GxRealOwner *pOwner, GxToolBarButton &rTButton);
  virtual ~GxToolBarButtonWin(void);

  virtual void Create(void);

protected:
  virtual void DoAction(void);
  virtual void DrawButton(void);

  virtual void PointerIn(void);
  virtual void PointerOut(void);

  //hackish, I would like this to be a const refrence
  GxToolBarButton &rButton;
  GxToolTip *pTTip;

  void TimerCB(void);
  TmrTimer ttTimer;
};

#endif //GXTOOLBARBUTTON_INCLUDED
