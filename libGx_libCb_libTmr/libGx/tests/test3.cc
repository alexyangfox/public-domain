#include <iostream>
#include <fstream>

#include <libGx/GxMainInterface.hh>
#include <libGx/GxDisplay.hh>
#include <libGx/GxMainWin.hh>
#include <libGx/GxMenuBar.hh>
#include <libGx/GxMenu.hh>
#include <libGx/GxMenuItems.hh>
#include <libGx/GxToolBarButton.hh>
#include <libGx/GxToolBarSimpleDock.hh>
#include <libGx/GxToolBar.hh>
#include <libGx/GxFileSelector.hh>
#include <libGx/GxTextWin.hh>
#include <libGx/GxGeomControl.hh>

GxMainInterface *pMainInt = 0;
GxMainWin *pMain = 0;
GxTextWin *pTWin = 0;

void ExitProgram(void)
{
  pMainInt->EndEventLoop();
}

void OpenFile(void)
{
  std::cout << "In OpenFile" << std::endl;
  GxFileSelector fileSel(pMain);
  std::string fileName;
  if( fileSel.OpenFile(fileName, "*.txt") != GX_OK ) return;

  std::cout << "File is: " << fileName << std::endl;
  std::ifstream fin(fileName.c_str());

  GxTextHunk& rHunk = pTWin->GrabTextHunk();
  rHunk.SetText(fin);
  pTWin->ReleaseTextHunk();
};

void ToggleWrap(bool newWrapStat)
{
  pTWin->Wrap(newWrapStat);
  pTWin->ReleaseTextHunk();
}

int main(int argc, char **argv)
{
  GxMainInterface mainInt("Test3");
  pMainInt = &mainInt;

  if( !mainInt.Initialize(argc, argv) ) return -1;
  if( !mainInt.OpenAllocateAll() ) return -2;

  GxMainWin mainW( mainInt.dispVector[0] );
  mainW.Resize(500,450);
  mainW.SetGeomControl( GxBasic(GX_WD_FIXED, GX_HT_FIXED,
			       GX_H_FIXED, GX_V_FIXED));
  pMain = &mainW;
 
  // *********** start File Menu *************
  GxMenuBar MBar(pMain);
  GxMenu fileMenu(&MBar, "File");
  
  GxMenuOption openOption(&fileMenu, "Open...");
  openOption.cb.Assign(CbVoidPlain(OpenFile));

  GxMenuOption saveOption(&fileMenu, "Save");

  GxMenuOption saveAsOption(&fileMenu, "Save As...");

  GxMenuOption exitOption(&fileMenu, "Exit");
  exitOption.cb.Assign(CbVoidPlain(ExitProgram));

  // ***************** Start Edit Menu ******************
  GxMenu editMenu(&MBar, "Edit");

  GxMenuOption cutOption(&editMenu, "Cut");

  GxMenuOption copyOption(&editMenu, "Copy");

  GxMenuOption pasteOption(&editMenu, "Paste");

  // ****************** Start Format Menu *********************

  GxMenu formatMenu(&MBar, "Format");

  GxMenuCheckOption wrapOption(&formatMenu, "Wrap");
  wrapOption.cb.Assign( CbOnePlain<bool>( ToggleWrap ) );

  // ********************** start button bar *******************
  GxToolBarSimpleDock tbDock(&mainW);
  tbDock.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT,
				 GX_FLOW_UP));

  GxToolBar toolBar(0);
  //#include "/usr/X11R6/include/X11/pixmaps/dir_opened.xpm"
#include "/usr/X11R6/include/X11/bitmaps/boxes"
  GxToolBarButton openButton(&toolBar, boxes_bits, "Open");
  //openButton.SetImage(dir_opened);
  //openButton.SetImage(boxes_bits);
  openButton.cb.Assign(CbVoidPlain(OpenFile));

  //#include "/usr/X11R6/include/X11/bitmaps/boxes"
  GxToolBarButton otherButton(&toolBar, "Other");
  otherButton.SetImage(boxes_bits);
  tbDock.SetToolBar(&toolBar);

  GxTextWin textWin(&mainW);
  pTWin = &textWin;
  textWin.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT,
				    GX_FLOW_UP, 1,1,1,1) );

  //textWin.SetWrap(false);
  textWin.Wrap( wrapOption.Checked() );
  textWin.Editable(true);
  //  textWin.SetText("This box is currently uneditable.\n\nHowever, at some point in the future, it will be, but I never want to give this widget the ability to format text like a wordprocessor, because of course there will only be one application on the system which needs this functionality. (the wordprocessor) thisisalongwordtocheckhowtheGxTextWinHandlesClipingALineWhichHasTextLongEnoughThatItmustbeEndedAtANon-whitespace.");
  //textWin.SetText("Hello\nHello.");

  pMain->Place();
  pMain->Create();
  pMain->Display();

  mainInt.EventLoop();

  return 0;
};

