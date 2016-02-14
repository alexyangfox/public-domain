#ifndef GXTEGUI_INCLUDED
#define GXTEGUI_INCLUDED

#include <string>

#include <libGx/GxDisplay.hh>
#include <libGx/GxMainWin.hh>
#include <libGx/GxMenuBar.hh>
#include <libGx/GxMenu.hh>
#include <libGx/GxMenuItems.hh>
#include <libGx/GxToolBarButton.hh>
#include <libGx/GxToolBarSimpleDock.hh>
#include <libGx/GxToolBar.hh>
#include <libGx/GxFileSelector.hh>
#include <libGx/GxMiscDialogs.hh>
#include <libGx/GxTextWin.hh>
#include <libGx/GxGeomControl.hh>

class GxteGui : public GxMainWin 
{
public:
  GxteGui(GxDisplay *pDisp);
  virtual ~GxteGui(void);

protected:
  void NewFile(void);
  void OpenFile(void);
  void Save(void);
  void SaveAs(void);
  void Exit(void);

  void ToggleWrap(bool newWrap);

  //GxTopLevelWin overloads
  virtual void DeleteWindow(void);
  virtual void GetWMProperties(XTextProperty &winName, XTextProperty &iconName,
			       XSizeHints &rSizeHints, XWMHints &rWMHints,
			       std::list<Atom> &rWMProtocolList);

  std::string cFileName;

  GxMenuBar MBar;

  // *********** start File Menu *************
  GxMenu fileMenu;
  GxMenuOption newOption;
  GxMenuOption openOption;
  GxMenuOption saveOption;
  GxMenuOption saveAsOption;
  GxMenuOption exitOption;

  // ***************** Start Edit Menu ******************
  GxMenu editMenu;
  GxMenuOption cutOption;
  GxMenuOption copyOption;
  GxMenuOption pasteOption;

  // ****************** Start Format Menu *********************
  GxMenu formatMenu;
  GxMenuCheckOption wrapOption;

  // ********************** start button bar *******************
  GxToolBarSimpleDock tbDock;
  GxToolBar toolBar;
  GxToolBarButton openButton;
  GxToolBarButton saveButton;

  GxTextWin textWin;
};

#endif //GXTEGUI_INCLUDED
