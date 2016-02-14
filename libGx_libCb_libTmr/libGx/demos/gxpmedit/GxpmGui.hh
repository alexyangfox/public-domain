#ifndef GXPMGUI_INCLUDED
#define GXPMGUI_INCLUDED

#include <libCb/CbCallback.hh>

#include <libGx/GxInc.hh>
#include <libGx/GxGhost.hh>
#include <libGx/GxMainWin.hh>
#include <libGx/GxMenuBar.hh>
#include <libGx/GxMenu.hh>
#include <libGx/GxMenuItems.hh>
#include <libGx/GxLabel.hh>
#include <libGx/GxTable.hh>
#include <libGx/GxNumberBox.hh>
#include <libGx/GxFileSelector.hh>
#include <libGx/GxRow.hh>
#include <libGx/GxColumn.hh>
#include <libGx/GxHLine.hh>
#include <libGx/GxStatusBar.hh>
#include <libGx/GxMiscDialogs.hh>
#include <libGx/GxFrame.hh>
#include <libGx/GxCheckBox.hh>
#include <libGx/GxLabeledBorder.hh>
#include <libGx/GxScrolledWin.hh>

#include "GxpmCore.hh"
#include "PreviewWin.hh"
#include "WorkWin.hh"
#include "ColorWin.hh"
#include "ColorDefDialog.hh"

enum PROG_STATE{DRAW_POINTS, DRAW_LINE, MOVE_AREA_SEL_AREA, MOVE_AREA_SEL_DEST, FILL_AREA_SEL};

class GxpmGui : public GxMainWin
{
public:
  GxpmGui(GxDisplay *pDisp, GxpmCore *pTCore);
  ~GxpmGui(void);

  //bool Create(void);
private:
  void NewCallback(void);
  void OpenCallback(void);
  void SaveCallback(void);
  void SaveAsCallback(void);
  void ExitCallback(void);
  //options menu
  void ResizeCallback(void);
  void ProgramOptionsCallback(void);
  //tools menu
  void RefreshCallback(void);
  void DrawContinuous(bool newState);
  void MoveBlockCB(void);
  void DrawLineCB(void);
  void FillAreaCB(void);

  //this saves the current file, returning false if the file is not saved
  //(this could happen if the user clicks cancel in the SaveAs Dialog)
  bool DoSaveWithDialog(void);
  //similar to above. returns false if the file is not saved.  this always
  //pops up a saveas dialog box to allow the user to rename/select a filename
  bool DoSaveAsWithDialog(void);

  //the callback from the WorkWin
  void DataEventCB(const DrawData &rData);
  //internal function which does the block move based on coordMatrix
  void DoMoveBlock(void);

  void HandleCurrColorChange(const ColorDef&);
  void HandleUserColorEnter(void);
  void HandleUserColorLeave(void);
  void HandleEditColor(ColorDef& rDef);
  void ConfigWorkWin(void);

  GxpmCore *pCore;
  PROG_STATE cState;

  // *********** start gui objects ****************

  GxMenuBar menuBar;
  GxMenu fileMenu;
  GxMenuOption newOption, openOption, saveOption, saveAsOption, exitOption;

  GxMenu optionsMenu;
  GxMenuOption sizeOption, programOption;

  GxMenu toolsMenu;
  GxMenuOption refreshOption;
  GxMenuCheckOption drawContOption;
  GxMenuOption moveOption;
  GxMenuOption lineOption;
  GxMenuOption fillOption;

  GxStatusBar statusBar;

  GxGhost centerGhost;
  GxThinColumn sideColumn;
  GxFrame colorFrame;
  GxThinColumn colorsColumn;

  GxLabeledBorder defaultColorsBorder;
  ColorWin defaultColorWin;

  GxLabeledBorder userColorsBorder;
  ColorWin userColorWin;

  GxThinRow currColorRow; //on a row together
  GxLabel currColorLabel;
  ColorBox colorBox;

  //expands to fill unused space
  PreviewWin previewWin;

  GxAppScrolledWin workArea;
  WorkWin workWin;
};

class SizeDialog : public GxPopupWin
{
public:
  SizeDialog(GxTopLevelWin *pOwner);
  virtual ~SizeDialog(void);

  GX_STATUS GetNewSize(UINT &rNewWidth, UINT &rNewHeight);

private:
  void OkCallback(void);
  void CancelCallback(void);

  GxThinColumn mainColumn;

  GxRow buttonRow;
  GxHLine hLine;
  GxTable mainTable;

  GxLabel widthRowLabel;
  GxNumberBox widthNumberBox;

  GxLabel heightRowLabel;
  GxNumberBox heightNumberBox;

  GxButton ok, cancel;

  GX_STATUS exitStat;
};

class OptionsDialog : public GxPopupWin
{
public:
  OptionsDialog(GxTopLevelWin *pOwner);
  virtual ~OptionsDialog(void);

  GX_STATUS DoDialog(ProgOptions &rOptions);

private:
  void SelFileCallback(void);
  void OkCallback(void);
  void CancelCallback(void);

  GxThinColumn mainColumn;

  GxRow rgbRow;
  GxLabel rgbLabel;
  GxEditWin rgbFileName;
  GxButton rgbSelFile;

  GxCheckBox saveColorsAsNames;

  GxRow scaleRow;
  GxLabel scaleLabel;
  GxNumberBox scaleNumberBox;

  GxHLine hLine;

  GxRow buttonRow;
  GxButton ok, cancel;

  GX_STATUS exitStat;
};

#endif //GXPMGUI_INCLUDED
