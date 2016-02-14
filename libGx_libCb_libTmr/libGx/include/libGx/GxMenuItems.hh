#ifndef GXMENUITEMS_INCLUDED
#define GXMENUITEMS_INCLUDED

#include <libCb/CbCallback.hh>

#include <libGx/GxInc.hh>
#include <libGx/GxMenuItemOwner.hh>
#include <libGx/GxWin.hh>

class GxMenuItem : public GxWin
{
public:
  virtual ~GxMenuItem(void);

  virtual UINT GetMinimumWidth(void) = 0;

protected:
  GxMenuItemOwner *pItemOwner; //not necessarily the same as pWinAreaOwner
  GxMenuItem(GxMenuItemOwner *pMenu);
};

class GxBaseMenuOption : public GxMenuItem
{
public:
  GxBaseMenuOption(GxMenuItemOwner *pOwner, const char * pLabel = NULL);
  virtual ~GxBaseMenuOption(void);

  void SetLabel(const char* pLabel);
  void SetActive(bool newStatus);

  virtual void Draw(void) = 0;
  void GetWindowData(XSetWindowAttributes &winAttributes, ULINT &valueMask);
  void HandleEvent(const XEvent &rEvent);

  //returns the length for just the text string
  virtual UINT GetMinimumWidth(void);

protected:
  virtual void DoCallback(void) = 0;

  char label[GX_DEFAULT_LABEL_LEN];
  bool active; //if the item can be selected
  //the mouse pointer is over the item & we are in the right mode
  bool selected;
};

class GxMenuOption : public GxBaseMenuOption
{
public:
  GxMenuOption(GxMenuItemOwner *pOwner, const char * pLabel = NULL);
  virtual ~GxMenuOption(void);

  virtual void Draw(void);

  CbVoidFO cb;
protected:
  virtual void DoCallback(void);
};

class GxMenuCheckOption : public GxBaseMenuOption
{
public:
  GxMenuCheckOption(GxMenuItemOwner *pOwner, const char * pLabel = NULL);
  virtual ~GxMenuCheckOption(void);

  bool Checked(void) const;
  void Checked(bool newCheck);

  virtual void Draw(void);
  virtual UINT GetMinimumWidth(void);

  CbOneFO<bool> cb;
protected:
  virtual void DoCallback(void);
  bool checked; //not checked by default
};

class GxMenuDivider : public GxMenuItem
{
public:
  GxMenuDivider(GxMenuItemOwner *pOwner);
  virtual ~GxMenuDivider(void);

  void Draw(void);
  void GetWindowData(XSetWindowAttributes &winAttributes, ULINT &valueMask);
  void HandleEvent(const XEvent &rEvent);
  UINT GetMinimumWidth(void);
};

class GxSubMenu : public GxMenuItem
{
public:
  GxSubMenu(GxMenuItemOwner *pOwner);
  virtual ~GxSubMenu(void);

  void SetLabel(const char* pLabel);
  void SetActive(bool newStatus);

  void Draw(void);
  void GetWindowData(XSetWindowAttributes &winAttributes, ULINT &valueMask);
  void HandleEvent(const XEvent &rEvent);
  UINT GetMinimumWidth(void);

private:
  char label[GX_DEFAULT_LABEL_LEN];
  bool active;
  //the mouse pointer is over the item & we are in the right mode
  bool selected;
};

#endif //GXMENUITEMS_INCLUDED
