#include "ColorDefDialog.hh"

ColorDefDialog::ColorDefDialog(GxTopLevelWin *pOwner) :
  GxPopupWin(pOwner), 
  mainColumn(this),
  buttonRow(&mainColumn),
  ok(&buttonRow, "Ok"), cancel(&buttonRow, "Cancel"),
  hLine(&mainColumn),
  nameWin(&mainColumn)
{
  mainColumn.SetGeomControl( GxSIBasic(GX_WD_INT, GX_HT_INT, GX_FLOW_LEFT, GX_FLOW_UP, 1,1,1,1) );

  buttonRow.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT, GX_FLOW_DOWN) );
  buttonRow.SetWidthIdentical(true);
  buttonRow.SetHeightIdentical(true);

  ok.cb.Assign( CbVoidMemberObj<ColorDefDialog, bool>(this, &ColorDefDialog::ExitCB, true) );
  cancel.cb.Assign( CbVoidMemberObj<ColorDefDialog, bool>(this, &ColorDefDialog::ExitCB, false) );

  hLine.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_FIXED, GX_FLOW_LEFT, GX_FLOW_DOWN) );

  nameWin.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT, GX_V_CENTERED) );

  Place();
  Create();
}

ColorDefDialog::~ColorDefDialog(void)
{}

bool ColorDefDialog::DoDialog(std::string &rName)
{
  Display();
  nameWin.SetText( rName.c_str() );

  EventLoop();

  rName = nameWin.GetText();

  return exitStat;
}

void ColorDefDialog::ExitCB(bool tExitStat)
{
  exitStat = tExitStat;
  processEvents = false;
}
