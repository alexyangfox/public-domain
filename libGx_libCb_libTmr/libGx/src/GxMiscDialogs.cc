#include <libGx/GxMiscDialogs.hh>

#include "GxDefines.hh"

GxMessageDialog::GxMessageDialog(GxTopLevelWin *pOwner) :
  GxPopupWin(pOwner), okButton(this, "Ok"), hLine(this), textWin(this)
{
  width = 350;
  height = 350;
  SetGeomControl( GxFixed() );

  okButton.SetGeomControl( GxSIBasic(GX_WD_INT, GX_HT_INT, GX_H_CENTERED,
				     GX_FLOW_DOWN, 1,1,0,1) );

  okButton.cb.Assign( CbVoidMember<GxMessageDialog>
		      (this, &GxMessageDialog::ButtonCB) );

  hLine.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_FIXED, GX_FLOW_LEFT,
				  GX_FLOW_DOWN, 1,1,0,0) );

  textWin.Wrap(true);
  textWin.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT, 
				    GX_FLOW_UP, 1,1,1,0) );
}

GxMessageDialog::~GxMessageDialog(void)
{}

void GxMessageDialog::SetMessage(const char *pText)
{
  textWin.SetText(pText);
  if(Created())
    Place();
}

GX_STATUS GxMessageDialog::DoDialog(void)
{
  Place();
  Create();
  Display();

  EventLoop();

  XUnmapWindow(dInfo.display, xWin);
  return GX_OK;
}

void GxMessageDialog::ButtonCB(void)
{
  processEvents = false;
  resultCB(GX_OK);
}

// ****************** start GxConfirmDialog **************************

GxConfirmDialog::GxConfirmDialog(GxTopLevelWin *pOwner) :
  GxPopupWin(pOwner), buttonRow(this),
  okB(&buttonRow, "Ok"), cancelB(&buttonRow, "Cancel"),
  hLine(this), textWin(this)
{
  width = 350;
  height = 250;
  SetGeomControl( GxFixed() );

  buttonRow.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT,
				      GX_FLOW_DOWN, 1,1,0,1) );
  buttonRow.SetWidthIdentical(true);
  buttonRow.SetHeightIdentical(true);

  okB.cb.Assign( CbVoidMemberObj<GxConfirmDialog, GX_STATUS>
		 (this, &GxConfirmDialog::ButtonCB, GX_OK) );
  cancelB.cb.Assign( CbVoidMemberObj<GxConfirmDialog, GX_STATUS>
		      (this, &GxConfirmDialog::ButtonCB, GX_CANCELED) );

  hLine.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_FIXED, GX_FLOW_LEFT,
				   GX_FLOW_DOWN, 1,1,0,0) );

  textWin.Wrap(true);
  textWin.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT, 
				   GX_FLOW_UP, 1,1,1,0) );
}

GxConfirmDialog::~GxConfirmDialog(void)
{}

void GxConfirmDialog::SetMessage(const char *pText)
{
  textWin.SetText(pText);
  if( Created() )
    Place();
}

GX_STATUS GxConfirmDialog::DoDialog(void)
{
  Place();
  Create();
  Display();

  EventLoop();
  XUnmapWindow(dInfo.display, xWin);

  return rStat;
}

void GxConfirmDialog::ButtonCB(GX_STATUS bPressed)
{
  rStat = bPressed;
  processEvents = false;

  resultCB(bPressed);
}

// ****************** end GxConfirmDialog **************************

// ********************* start YesNoDialog ***********************

GxYesNoDialog::GxYesNoDialog(GxTopLevelWin *pOwner) :
  GxPopupWin(pOwner), buttonRow(this),
  yesB(&buttonRow, "Yes"), noB(&buttonRow, "No"),
  buttonLine(this), textWin(this)
{
  width = 350;
  height = 250;
  SetGeomControl( GxFixed() );

  buttonRow.SetWidthIdentical(true);
  buttonRow.SetHeightIdentical(true);
  buttonRow.SetAdditionalGap(GX_SPACE_INC);
  buttonRow.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT, 
				      GX_FLOW_DOWN,1,1,1,1) );

  yesB.cb.Assign( CbVoidMemberObj<GxYesNoDialog, GX_STATUS>
		  (this, &GxYesNoDialog::ButtonCB, GX_YES) );

  noB.cb.Assign( CbVoidMemberObj<GxYesNoDialog, GX_STATUS>
		 (this, &GxYesNoDialog::ButtonCB, GX_NO) );
  
  buttonLine.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_FIXED, GX_FLOW_LEFT,
				       GX_FLOW_DOWN, 1,1,0,0) );

  textWin.Wrap(true);
  textWin.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT, 
				    GX_FLOW_UP, 1,1,1,0) );
}

GxYesNoDialog::~GxYesNoDialog(void)
{}

void GxYesNoDialog::SetMessage(const char *pMessage)
{
  textWin.SetText(pMessage);
}

GX_STATUS GxYesNoDialog::DoDialog(void)
{
  Place();
  Create();
  Display();

  EventLoop();
  XUnmapWindow(dInfo.display, xWin);

  return retStat;
}

void GxYesNoDialog::ButtonCB(GX_STATUS tStat)
{
  retStat = tStat;
  processEvents = false;
}

// ************************** end YesNoDialog **************************


// ********************** start GxYesNoCancelDialog **********************

GxYesNoCancelDialog::GxYesNoCancelDialog(GxTopLevelWin *pOwner) :
  GxPopupWin(pOwner), buttonRow(this),
  yesB(&buttonRow, "Yes"), noB(&buttonRow, "No"),
  cancelB(&buttonRow, "Cancel"),
  buttonLine(this), textWin(this)
{
  width = 350;
  height = 250;
  SetGeomControl( GxFixed() );

  buttonRow.SetWidthIdentical(true);
  buttonRow.SetHeightIdentical(true);
  buttonRow.SetAdditionalGap(GX_SPACE_INC);
  buttonRow.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT, 
				      GX_FLOW_DOWN,1,1,1,1) );

  yesB.cb.Assign( CbVoidMemberObj<GxYesNoCancelDialog, GX_STATUS>
		  (this, &GxYesNoCancelDialog::ButtonCB, GX_YES) );

  noB.cb.Assign( CbVoidMemberObj<GxYesNoCancelDialog, GX_STATUS>
		 (this, &GxYesNoCancelDialog::ButtonCB, GX_NO) );

  cancelB.cb.Assign( CbVoidMemberObj<GxYesNoCancelDialog, GX_STATUS>
		     (this, &GxYesNoCancelDialog::ButtonCB, GX_CANCELED) );
  
  buttonLine.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_FIXED, GX_FLOW_LEFT,
				       GX_FLOW_DOWN, 1,1,0,0) );

  textWin.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT, 
				    GX_FLOW_UP, 1,1,1,0) );
}

GxYesNoCancelDialog::~GxYesNoCancelDialog(void)
{}

void GxYesNoCancelDialog::SetMessage(const char *pMessage)
{
  textWin.SetText(pMessage);
}

GX_STATUS GxYesNoCancelDialog::DoDialog(void)
{
  Place();
  Create();
  Display();

  EventLoop();
  XUnmapWindow(dInfo.display, xWin);

  return retStat;
}

void GxYesNoCancelDialog::ButtonCB(GX_STATUS tStat)
{
  retStat = tStat;
  processEvents = false;
}
// ********************* end GxYesNoCancelDialog *********************

// ************************** GxStatusDialog **********************
GxStatusDialog::GxStatusDialog(GxTopLevelWin *pOwner) :
  GxPopupWin(pOwner), cancelAction(false), cancelButton(this, "Cancel"),
  percentBar(this)
{
  width = 250;
  height = 75;
  SetGeomControl(GxBasic(GX_WD_FIXED, GX_HT_FIXED, GX_H_FIXED, GX_V_FIXED) );

  cancelButton.cb.Assign( CbVoidMember<GxStatusDialog>(this, &GxStatusDialog::CancelCB));
  cancelButton.SetGeomControl( GxSIBasic(GX_WD_INT, GX_HT_INT, GX_H_CENTERED,
					 GX_FLOW_DOWN, 2,2,1,2) );
  
  percentBar.SetGeomControl( GxSIBasic(GX_WD_INT, GX_HT_INT, GX_H_CENTERED,
					 GX_V_CENTERED, 2,2,2,1) );
}

GxStatusDialog::~GxStatusDialog(void)
{}

void GxStatusDialog::SetPercent(UINT percent)
{
  if(percent > 100)
    percent = 100;

  GxFraction pFraction(percent, 100);
  percentBar.SetPercent(pFraction); 
}

void GxStatusDialog::CancelCB(void)
{
  if(eventHandlerID != NULL_EVENT_HANDLER_ID)
    EndNonblockingDialog();
  else
    processEvents = false;

  cancelAction = true;
}
