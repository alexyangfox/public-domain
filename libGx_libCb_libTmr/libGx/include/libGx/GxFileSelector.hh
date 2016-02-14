#ifndef GXFILESELECTOR_INCLUDED
#define GXFILESELECTOR_INCLUDED

#include <string>

#include <libGx/GxPopupWin.hh>
#include <libGx/GxHLine.hh>
#include <libGx/GxButton.hh>
#include <libGx/GxList.hh>
#include <libGx/GxLabel.hh>
#include <libGx/GxEditWin.hh>
#include <libGx/GxRow.hh>
#include <libGx/GxColumn.hh>
#include <libGx/GxGeomControl.hh>
//#include <libGx/GxVDivider.hh>

class GxFileSelector : public GxPopupWin
{
public:
  GxFileSelector(GxTopLevelWin *pOwner);
  virtual ~GxFileSelector(void);

  GX_STATUS OpenFile(std::string &rSelFileName, const std::string &rFileMask);
  GX_STATUS SaveFile(std::string &rSelFileName, const std::string &rFileMask,
		     bool confermOverwrite = true);

protected:
  virtual void GetWMProperties(XTextProperty &winName, XTextProperty &iconName,
			       XSizeHints &rSizeHints, XWMHints &rWMHints);
private:

  void DirectoryCallback(const char *pDir);
  void FileCallback(const char *pFile);

  //the enterCB of the pSelectionWin call's this too.
  void OkCallback(void);
  void FilterCallback(void);
  void CancelCallback(void);

  //this returns a bool which should be honored.  this prevents cd'ing into
  //a directory with no read access and then not being able to .. back out
  //because the directory couldn't be read.
  bool FillLists(void);

  //checks the file against the mask to see if it should be included
  //in the file list. returns TRUE if it should FALSE otherwise
  bool FitsMask(const char *pName);

  class GxFSListItem : public GxListItem
  {
  public:
    GxFSListItem(const char *pLabel, const CbOneBase<const char*>& tCB);
    virtual ~GxFSListItem(void);

    virtual void Size(GxListData &rData);
    virtual void SelectCallback(void);

    virtual void Draw(int x, int y, const GxListData &rData);

    //kind of ugly that this is public; but it is done so access can be made
    //to it in the GxFileSelector without using an access function
    //It is important that reading the label be fast because when alphabetizing
    //a long list of files will require many checks of the label
    char label[GX_DEFAULT_LABEL_LEN];
  private:
    CbOneFO<const char*> cb;
  };

  void AddItem(GxList *pList, GxFSListItem *pNewItem);

  GxGhost outerGhost;
  GxLabel filterLabel;
  GxEditWin filterWin;
  //the buttons across the bottom
  GxRow bRow;
  GxButton Ok, Filter, Cancel;
  GxHLine bLine;
  GxEditWin selectionWin;
  GxLabel selLabel;

  GxThinRow centerRow;
  GxThinColumn dirHolder;
  GxThinColumn fileHolder;

  GxLabel dirLabel, fileLabel;
  GxList dirList, fileList;

  //true if opening a file; false if saving a file
  bool openingFile;
  //if true conferm overwrite; only makes a difference if saving a file
  bool cOvrwrite;

  std::string path;
  std::string ext;

  GX_STATUS returnStat;
};

#endif //GXFILESELECTOR_INCLUDED
