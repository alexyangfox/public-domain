#include <fnmatch.h>
#include <sstream>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include <libGx/GxFileSelector.hh>

using namespace std;

GxFileSelector::GxFileSelector(GxTopLevelWin *pOwner) :
  GxPopupWin(pOwner),
  outerGhost(this),
  filterLabel(&outerGhost, "Filter:"),
  filterWin(&outerGhost),
  bRow(&outerGhost),
  Ok(&bRow, "OK"), Filter(&bRow, "Filter"), Cancel(&bRow, "Cancel"),
  bLine(&outerGhost),
  selectionWin(&outerGhost),
  selLabel(&outerGhost, "Selection:"),
  centerRow(&outerGhost),
  dirHolder(&centerRow), fileHolder(&centerRow),
  dirLabel( &dirHolder, "Directories"),
  fileLabel( &fileHolder, "Files"),
  dirList( &dirHolder ), fileList( &fileHolder )
{
  width = 500;
  height = 400;
  SetGeomControl(GxBasic(GX_WD_FIXED, GX_HT_FIXED, GX_H_FIXED, GX_V_FIXED));

  char cPath[1024];
  getcwd(cPath, 1024);
  //make totally sure the last char in path is a '/'
  int lastSpot = strlen(cPath);
  //  cout << "path 1: " << path << endl;
  if(cPath[lastSpot-1] != '/')
    {
      cPath[lastSpot] = '/';
      cPath[lastSpot+1] = '\0';
    };
  //  cout << "path 2: " << path << endl;

  path = cPath;

  //the ghost just provides borders for everything
  outerGhost.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_FILL,
				       GX_FLOW_LEFT, GX_FLOW_UP, 1,1,1,1) );

  filterLabel.SetGeomControl( GxBasic(GX_WD_INT, GX_HT_INT, GX_FLOW_LEFT,
				      GX_FLOW_UP, false, true) );

  filterWin.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT,
				    GX_FLOW_UP) );
  filterWin.enterCB.Assign( CbVoidMember<GxFileSelector>
			    (this, &GxFileSelector::FilterCallback) );

  //the buttons across the bottom
  bRow.SetWidthIdentical(true);
  bRow.SetHeightIdentical(true);
  bRow.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT,
			       GX_FLOW_DOWN) );

  Ok.cb.Assign( CbVoidMember<GxFileSelector>
		(this, &GxFileSelector::OkCallback));

  Filter.cb.Assign( CbVoidMember<GxFileSelector>
		    (this, &GxFileSelector::FilterCallback));
  Cancel.cb.Assign( CbVoidMember<GxFileSelector>
		    (this, &GxFileSelector::CancelCallback));
  //end the buttons across the bottom

  //the line just above the buttons
  bLine.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_FIXED, GX_FLOW_LEFT,
				GX_FLOW_DOWN) );

  //the selection edit window just above the bottom
  selectionWin.enterCB.Assign( CbVoidMember<GxFileSelector>
			       (this, &GxFileSelector::OkCallback) );
  selectionWin.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT,
				       GX_FLOW_DOWN) );

  //the label for the selection edit window
  selLabel.SetGeomControl( GxBasic(GX_WD_INT, GX_HT_INT, GX_FLOW_LEFT,
				   GX_FLOW_DOWN, false, true) );


  centerRow.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT,
				       GX_FLOW_UP, 0,0, 1,1));

  dirHolder.SetGeomControl( GxBasic(GX_WD_INT, GX_HT_FILL, GX_FLOW_LEFT, GX_FLOW_UP) );
  fileHolder.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT, GX_FLOW_UP) );

  dirLabel.SetGeomControl( GxBasic(GX_WD_INT, GX_HT_INT, GX_FLOW_LEFT,
				   GX_FLOW_UP, false, true));

  dirList.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_FILL,
				  GX_FLOW_LEFT, GX_FLOW_UP) );
  dirList.SetDesiredWidth(200);


  fileLabel.SetGeomControl( GxBasic(GX_WD_INT, GX_HT_INT, GX_FLOW_LEFT,
				    GX_FLOW_UP, false, true) );

  fileList.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_FILL,
				   GX_FLOW_LEFT, GX_FLOW_UP) );

  AddFocusObject(&filterWin);
  AddFocusObject(&selectionWin);
  AddFocusObject(&Ok);
  AddFocusObject(&Filter);
  AddFocusObject(&Cancel);
}

GxFileSelector::~GxFileSelector(void)
{}

GX_STATUS GxFileSelector::OpenFile(std::string &rSelFileName, const std::string &rFileMask)
{
  openingFile = true;
  ext = rFileMask;

  filterWin.SetText( (path + ext).c_str() );

  FillLists();

  this->Place(); //this is the GxMainWin::Place function
  Create();
  Display();
  EventLoop();

  ostringstream filePName;
  filePName << path << selectionWin.GetText();
  rSelFileName = filePName.str();

  XUnmapWindow(dInfo.display, xWin);
  return returnStat;
}

GX_STATUS GxFileSelector::SaveFile(std::string &rSelFileName, const std::string &rFileMask,
				   bool confermOverwrite)
{
  openingFile = false;
  cOvrwrite = confermOverwrite;
  ext = rFileMask;

  filterWin.SetText( (path + ext).c_str() );

  FillLists();

  this->Place(); //this is the GxMainWin::Place function
  Create();
  Display();
  EventLoop();

  ostringstream filePName;
  filePName << path << selectionWin.GetText() << '\0';
  rSelFileName = filePName.str();

  //ostrstream filePName(pBuffer,buffLen);
  //filePName << path << selectionWin.GetText() << '\0';
  XUnmapWindow(dInfo.display, xWin);
  return returnStat;
}

void GxFileSelector::GetWMProperties(XTextProperty &winName,
				     XTextProperty &iconName,
				     XSizeHints &rSizeHints,
				     XWMHints &rWMHints)
{
  const char * pWinString = "File Selector";
  XStringListToTextProperty(const_cast<char**>(&pWinString), 1, &winName);
  XStringListToTextProperty(const_cast<char**>(&pWinString), 1, &iconName);
  XSetTransientForHint(dInfo.display, xWin, pWinAreaOwner->GetClosestXWin());

  rWMHints.flags = InputHint;
  rWMHints.input = true;
}

void GxFileSelector::DirectoryCallback(const char *pDir)
{
  string prevPath = path;

  //cout << "GxFileSelector::DirectoryCallback dir: " << pDir << endl;
  if( !strcmp(pDir, ".") )
    {
      FillLists();
      return;
    };

  if( !strcmp(pDir, "..") )
    {
      int firstSlash = path.find("/");
      //the first instance of / had better be the first one in the
      //string. if it's not there is a problem and we'd better make it so
      if(firstSlash != 0)
	{
	  //cerr << "first char in path is not a /" << endl;
	  path = "/";

	  FillLists();
	  filterWin.SetText( (path+ext).c_str() );
	  return;
	};

      //search from the end of the string for '/'
      int endS = path.rfind("/");

      if(endS == -1)
	{
#ifdef LIBGX_DEBUG_BUILD
	  cerr << "error 1 in GxFileSelector::DirectoryCallback" << endl;
#endif //LIBGX_DEBUG_BUILD
	  return;
	};

      if(endS == firstSlash)
	return;

      int nextS = path.rfind("/", endS - 1);

      if(nextS == -1)
	{
#ifdef LIBGX_DEBUG_BUILD
	  cerr << "error 2 in GxFileSelector::DirectoryCallback" << endl;
#endif //LIBGX_DEBUG_BUILD
	  return;
	};

      //now end the string one after the next to last /
      path = path.substr(0, nextS+1);
      if( !FillLists() )
	{
	  path = prevPath;
	  if(!FillLists() )
	    {
	      path = "/"; //something we know should be good
	      FillLists();
	    };
	};
      filterWin.SetText( (path+ext).c_str() );
      //cout << "path: " << path << endl;
      return;
    };

  //append the new directory name to the end of the path
  path += string(pDir) + string("/");
  //cout << "path: " << path << endl;
  if( !FillLists() )
    {
      path = prevPath;
      if(!FillLists() )
	{
	  path = "/"; //something we know should be good
	  FillLists();
	};
    };

  filterWin.SetText( (path+ext).c_str() );
  FillLists();
}

void GxFileSelector::FileCallback(const char *pFile)
{
  selectionWin.SetText(pFile);
}

//this is called by the OK button and the callback of the selection win
void GxFileSelector::OkCallback(void)
{
  returnStat = GX_OK;
  processEvents = false;
}

void GxFileSelector::FilterCallback(void)
{
  //find the last / in the filterWin text.
  //the extention string starts after this. Update the ext string with the
  //new extension, and call FillLists to update the file lists with the new
  //extention.
  //because the user may have deleted part of the current string before
  //he hit enter or clicked the filer button, re-update the contents of the
  //filterWin with the path+ext string
  string filterWinText = filterWin.GetText();
  int endS = filterWinText.rfind("/");
  string extString;
  if(endS == -1)
    {
      extString = filterWinText;
    }else
      {
	//the ext string should be the part of the list after the /
	extString = filterWinText.substr(endS+1, extString.length() - endS-1);
      };

  //cout << "extString: " << extString << endl;

  if(extString.length() == 0)
    extString = "*";

  ext = extString;
  filterWin.SetText( (path+ext).c_str() );
  FillLists();
}

void GxFileSelector::CancelCallback(void)
{
  returnStat = GX_CANCELED;
  processEvents = false;
}

bool GxFileSelector::FillLists(void)
{
  dirList.Clear();
  fileList.Clear();

  DIR *pDir = opendir( path.c_str() );
  if(!pDir)
    {
#ifdef LIBGX_DEBUG_BUILD
      cerr << "Error opening directory" << endl;
#endif //LIBGX_DEBUG_BUILD
      return false;
    };

  struct stat cFileStat; //holds info on the file
  dirent *pEntry = readdir(pDir);
  while(pEntry)
    {
      string fName = path + pEntry->d_name;
      if( 0 == stat( fName.c_str() , &cFileStat) ) {
	if(S_ISDIR(cFileStat.st_mode)) //we are adding a directory
	  {
	    AddItem(&dirList,  new GxFSListItem(pEntry->d_name,
				     CbOneMember<const char*, GxFileSelector>
						(this, &GxFileSelector::DirectoryCallback)) );
	  }else //we are adding a file
	    {
	      //cout << "adding file" << endl;
	      if( FitsMask(pEntry->d_name) )
		AddItem( &fileList, new GxFSListItem(pEntry->d_name,
				     CbOneMember<const char*, GxFileSelector>
					    (this, &GxFileSelector::FileCallback)) );
	    };
      };

      pEntry = readdir(pDir);
    };

  dirList.SetTopItem();
  fileList.SetTopItem();

  if( Created() )
    {
      dirList.ClearWindow();
      dirList.SizeAll();
      dirList.DrawList();

      fileList.ClearWindow();
      fileList.SizeAll();
      fileList.DrawList();
    };

  closedir(pDir);
  return true;
}

bool GxFileSelector::FitsMask(const char *pName)
{
  if(!pName) return false;
  //would like this for below (FNM_PERIOD | FNM_CASEFOLD)
  return !fnmatch(ext.c_str(), pName, (FNM_PERIOD));
}

void GxFileSelector::AddItem(GxList *pList, GxFSListItem *pNewItem)
{
  GxFSListItem *pCItem = (GxFSListItem*)pList->GetFirstListItem();
  if(!pCItem)
    {
      pList->AddListItemStart(pNewItem);
      return;
    };

  while(1)
    {
      //new item goes before current one
      if(strcmp(pCItem->label, pNewItem->label) > 0)
	{
	  pList->AddListItemBefore(pCItem, pNewItem);
	  return;
	};

      GxFSListItem *pNItem = (GxFSListItem*)pCItem->GetNextItem();
      if(pNItem)
	pCItem = pNItem;
      else
	{
	  pList->AddListItemAfter(pCItem, pNewItem);
	  return;
	};
    };
}

GxFileSelector::GxFSListItem::GxFSListItem(const char *pLabel,
					   const CbOneBase<const char*>& tCB) :
  GxListItem(), cb(tCB)
{
  unsigned junkLen = 0;
  GxSetLabel(label, GX_DEFAULT_LABEL_LEN, pLabel, junkLen);
}

GxFileSelector::GxFSListItem::~GxFSListItem(void)
{}

void GxFileSelector::GxFSListItem::Size(GxListData &rData)
{
  itemWidth = (UINT)XTextWidth(rData.dInfo.pDefaultFont, label,
			       strlen(label)) + 2;

  itemHeight = rData.dInfo.pDefaultFont->ascent +
    rData.dInfo.pDefaultFont->descent + 2; //one pixel aditional top&bottom
}

void GxFileSelector::GxFSListItem::SelectCallback(void)
{
  cb(label);
}

void GxFileSelector::GxFSListItem::Draw(int x, int y, const GxListData &lData)
{
  //hack; using menuGC
  XDrawString(lData.dInfo.display, lData.win, lData.vData.menuGC, x+1,
	    y+1+lData.dInfo.pDefaultFont->ascent, label, strlen(label));
}
