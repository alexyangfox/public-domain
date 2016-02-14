#include <iostream>
#include <fstream>
//#include <sstream>

#include "open.xpm"
#include "save.xpm"

#include "GxteGui.hh"

using namespace std;

// hackish -> used for exit
extern GxMainInterface *pMainInt;

GxteGui::GxteGui(GxDisplay *pDisp) :
  GxMainWin(pDisp),
  // *********** File Menu *************
  MBar(this),
  fileMenu(&MBar, "File"),
  newOption(&fileMenu, "New"),
  openOption(&fileMenu, "Open..."),
  saveOption(&fileMenu, "Save"),
  saveAsOption(&fileMenu, "Save As..."),
  exitOption(&fileMenu, "Exit"),

  // ***************** Edit Menu ******************
  editMenu(&MBar, "Edit"),
  cutOption(&editMenu, "Cut"),
  copyOption(&editMenu, "Copy"),
  pasteOption(&editMenu, "Paste"),

  // ****************** Format Menu *********************
  formatMenu(&MBar, "Format"),
  wrapOption(&formatMenu, "Wrap"),

  // ********************** button bar *******************
  tbDock(this),
  toolBar(0),
  openButton(&toolBar, open_xpm, "Open"),
  saveButton(&toolBar, save_xpm, "Save"),

  textWin(this)
{

  Resize(500,450);
  SetGeomControl( GxBasic(GX_WD_FIXED, GX_HT_FIXED, GX_H_FIXED, GX_V_FIXED));

  textWin.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT, GX_FLOW_UP, 1,1,1,1) );

  //textWin.SetWrap(false);
  textWin.Wrap( wrapOption.Checked() );
  textWin.Editable(true);

  tbDock.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT, GX_FLOW_UP));

  tbDock.SetToolBar(&toolBar);

  // assign menu callbacks
  newOption.cb.Assign(    CbVoidMember<GxteGui>(this, &GxteGui::NewFile) );
  openOption.cb.Assign(   CbVoidMember<GxteGui>(this, &GxteGui::OpenFile) );
  saveOption.cb.Assign(   CbVoidMember<GxteGui>(this, &GxteGui::Save) );
  saveAsOption.cb.Assign( CbVoidMember<GxteGui>(this, &GxteGui::SaveAs) );
  exitOption.cb.Assign( CbVoidMember<GxteGui>(this, &GxteGui::Exit) );

  wrapOption.cb.Assign( CbOneMember<bool, GxteGui>(this, &GxteGui::ToggleWrap) );

  // assign button bar callbacks
  openButton.cb.Assign( CbVoidMember<GxteGui>(this, &GxteGui::OpenFile) );  
  saveButton.cb.Assign( CbVoidMember<GxteGui>(this, &GxteGui::Save) );  
}

GxteGui::~GxteGui(void)
{}


void GxteGui::NewFile(void)
{
  cFileName.clear();
  textWin.SetText("");
}

void GxteGui::OpenFile(void)
{
  GxFileSelector fileSel(this);
  string fileName;
  if( fileSel.OpenFile(fileName, "*.txt") != GX_OK ) return;
  
  ifstream fin(fileName.c_str());
  if( !fin )
    {
      GxMessageDialog errDialog(this);
      errDialog.SetMessage( (string("error opening file: ") + fileName).c_str() );
      errDialog.DoDialog();
      return;
    };

  cFileName = fileName;
  GxTextHunk& rHunk = textWin.GrabTextHunk();
  rHunk.SetText(fin);
  textWin.ReleaseTextHunk();
}

void GxteGui::Save(void)
{
  if(cFileName.empty())
    {
      SaveAs();
      return;
    };

  ofstream fout( cFileName.c_str());
  if( !fout )
    {
      GxMessageDialog errDialog(this);
      errDialog.SetMessage( (string("Error opening file: ") + cFileName).c_str() );
      errDialog.DoDialog();
      return;
    };

  GxTextHunk::TextPlace cursorPlace;
  unsigned topLine;

  GxTextHunk& rText = textWin.GrabTextHunk(cursorPlace, topLine);
  rText.GetText( fout );
  textWin.ReleaseTextHunk(cursorPlace, topLine);

  fout.close();
}

void GxteGui::SaveAs(void)
{
  GxFileSelector fileSel(this);
  string fileName;
  if( fileSel.OpenFile(fileName, "*.txt") != GX_OK ) return;
  
  cFileName = fileName;
  Save();
}

void GxteGui::Exit(void)
{
  //hack. ask about unsaved changes.
  pMainInt->EndEventLoop();
}

void GxteGui::ToggleWrap(bool newWrapStat)
{
  textWin.Wrap(newWrapStat);
  textWin.ReleaseTextHunk(); //hackish.
}


void GxteGui::DeleteWindow(void)
{
  Exit();
}

void GxteGui::GetWMProperties(XTextProperty &winName, XTextProperty &iconName,
			      XSizeHints &rSizeHints, XWMHints &rWMHints,
			      std::list<Atom> &rWMProtocolList)
{
  GxTopLevelWin::GetWMProperties(winName, iconName, rSizeHints, rWMHints, rWMProtocolList);
  rWMProtocolList.push_back( dInfo.deleteWindow );
}
