#include "GxpmGui.hh"

#include <sstream>

GxpmGui::GxpmGui(GxDisplay *pDisp, GxpmCore *pTCore) :
  GxMainWin(pDisp), 
  pCore(pTCore),
  cState(DRAW_POINTS),

  menuBar(this),
  fileMenu(&menuBar, "File"),
  newOption(&fileMenu, "New"),
  openOption(&fileMenu, "Open..."),
  saveOption(&fileMenu, "Save"),
  saveAsOption(&fileMenu, "SaveAs..."),
  exitOption(&fileMenu, "Exit"),

  optionsMenu(&menuBar, "Options"),
  sizeOption(&optionsMenu, "Size..."),
  programOption(&optionsMenu, "Program Options..."),

  toolsMenu(&menuBar, "Tools"),
  refreshOption(&toolsMenu, "Refresh"),
  drawContOption(&toolsMenu, "Draw Continuous"),
  moveOption(&toolsMenu, "Move Block"),
  lineOption(&toolsMenu, "Draw Line"),
  fillOption(&toolsMenu, "Fill Block"),

  statusBar(this),
  centerGhost(this),
  sideColumn(&centerGhost),

  colorFrame(&sideColumn),
  colorsColumn(&colorFrame),

  defaultColorsBorder(&colorsColumn, "Default Colors"),
  defaultColorWin(&defaultColorsBorder),

  userColorsBorder(&colorsColumn, "User Colors"),
  userColorWin(&userColorsBorder, NUM_DEFAULT_COLORS, 24),

  currColorRow(&colorsColumn),
  currColorLabel(&currColorRow, "Current Color: "),
  colorBox(&currColorRow),

  previewWin(&sideColumn),

  workArea(&centerGhost),
  workWin(&workArea, pCore->GetPixWidth(), pCore->GetPixHeight(), DEFAULT_WORK_WIN_SCALE)
{
  Resize(500,450);
  SetGeomControl( GxFixed() );

  newOption.cb.Assign(CbVoidMember<GxpmGui>(this, &GxpmGui::NewCallback));
  openOption.cb.Assign(CbVoidMember<GxpmGui>(this, &GxpmGui::OpenCallback));
  saveOption.cb.Assign(CbVoidMember<GxpmGui>(this, &GxpmGui::SaveCallback));
  saveAsOption.cb.Assign(CbVoidMember<GxpmGui>(this, &GxpmGui::SaveAsCallback));
  exitOption.cb.Assign(CbVoidMember<GxpmGui>(this, &GxpmGui::ExitCallback));

  sizeOption.cb.Assign(CbVoidMember<GxpmGui>(this, &GxpmGui::ResizeCallback));
  programOption.cb.Assign(CbVoidMember<GxpmGui>(this, &GxpmGui::ProgramOptionsCallback));

  refreshOption.cb.Assign(CbVoidMember<GxpmGui>(this, &GxpmGui::RefreshCallback));
  drawContOption.Checked(false);
  drawContOption.cb.Assign( CbOneMember<bool, GxpmGui>(this, &GxpmGui::DrawContinuous) );
  moveOption.cb.Assign( CbVoidMember<GxpmGui>(this, &GxpmGui::MoveBlockCB) );
  lineOption.cb.Assign( CbVoidMember<GxpmGui>(this, &GxpmGui::DrawLineCB) );
  fillOption.cb.Assign( CbVoidMember<GxpmGui>(this, &GxpmGui::FillAreaCB) );

  statusBar.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT, GX_FLOW_DOWN) );

  centerGhost.SetGeomControl(GxBasic(GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT, GX_FLOW_UP) );

  sideColumn.SetGeomControl( GxBasic(GX_WD_INT, GX_HT_FILL, GX_FLOW_LEFT, GX_FLOW_UP) );

  colorFrame.SetGeomControl(GxBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT, GX_FLOW_DOWN) );

  defaultColorsBorder.SetGeomControl( GxBasic( GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT, GX_FLOW_UP) );
  defaultColorWin.SetGeomControl( GxBasic( GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT, GX_FLOW_UP) );
  defaultColorWin.colorChangeCB.Assign(CbOneMember<const ColorDef&, GxpmGui>(this, &GxpmGui::HandleCurrColorChange));

  userColorsBorder.SetGeomControl( GxBasic( GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT, GX_FLOW_UP) );
  userColorWin.SetGeomControl( GxBasic( GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT, GX_FLOW_UP) );
  userColorWin.colorChangeCB.Assign(CbOneMember<const ColorDef&, GxpmGui>(this, &GxpmGui::HandleCurrColorChange));
  userColorWin.colorDefineCB.Assign(CbOneMember<ColorDef&, GxpmGui>(this, &GxpmGui::HandleEditColor));
  userColorWin.colorEnterCB.Assign( CbVoidMember<GxpmGui>(this, &GxpmGui::HandleUserColorEnter) );
  userColorWin.colorLeaveCB.Assign( CbVoidMember<GxpmGui>(this, &GxpmGui::HandleUserColorLeave) );

  currColorRow.SetGeomControl( GxBasic( GX_WD_INT, GX_HT_INT, GX_H_CENTERED, GX_FLOW_UP) );
  currColorLabel.SetGeomControl( GxBasic( GX_WD_INT, GX_HT_INT, GX_FLOW_LEFT, GX_V_CENTERED) );
  colorBox.SetGeomControl( GxBasic( GX_WD_INT, GX_HT_INT, GX_FLOW_LEFT, GX_V_CENTERED) );
  colorBox.SetCurrentColor( defaultColorWin.GetDefaultColor() );

  previewWin.SizePixmap( pCore->GetPixWidth(), pCore->GetPixHeight() );
  previewWin.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT, GX_FLOW_UP) );
  
  workWin.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT, GX_FLOW_UP, 1,1,1,1) );
  workWin.redrawCB.Assign( CbVoidMember<GxpmGui>(this, &GxpmGui::RefreshCallback) );
  workWin.dataEventCB.Assign( CbOneMember<const DrawData&, GxpmGui>(this, &GxpmGui::DataEventCB) );

  workArea.SetClipWindow(&workWin);

  Place();
  Create();

  ConfigWorkWin();
  RefreshCallback();

  Display();
}

GxpmGui::~GxpmGui(void)
{}

void GxpmGui::NewCallback(void)
{
  if( pCore->UnSavedData() )
    {
      if( !DoSaveWithDialog() )
	return;
      //else continue below
    };

  pCore->CreateNewXPM();
  workWin.SizePixmap( pCore->GetPixWidth(), pCore->GetPixHeight() );
  previewWin.SizePixmap( pCore->GetPixWidth(), pCore->GetPixHeight() );
  centerGhost.PlaceChildren();
  RefreshCallback();
  cState = DRAW_POINTS;
  ConfigWorkWin();
}

void GxpmGui::OpenCallback(void)
{
  if( pCore->UnSavedData() )
    {
      GxYesNoCancelDialog yncDialog(this);
      yncDialog.SetMessage("Warning: Unsaved Changes.  Save Them?");
      GX_STATUS rStat = yncDialog.DoDialog();
      XUnmapWindow(dInfo.display, yncDialog.GetWindow());

      switch(rStat)
	{
	case GX_YES:
	  SaveCallback();
	case GX_NO:
	  break; //continue below
	default: //GX_CANCELED
	  return;
	};
    };

  while(1)
    {
      GxFileSelector *pSelector = new GxFileSelector(this);
      std::string fileName;
      GX_STATUS stat = pSelector->OpenFile(fileName, "*.xpm");
      delete pSelector;

      if(stat == GX_CANCELED)
	return;

      if( pCore->ReadFile(fileName) )
	{
	  unsigned nWidth = pCore->GetPixWidth();
	  unsigned nHeight = pCore->GetPixHeight();

	  previewWin.SizePixmap(nWidth, nHeight);
	  workWin.SizePixmap(nWidth, nHeight);
	  centerGhost.PlaceChildren();
	  RefreshCallback();
	  cState = DRAW_POINTS;
	  ConfigWorkWin();
	  return;
	};
    };
}

void GxpmGui::SaveCallback(void)
{
  //we could check wether or not we have any unsaved changes
  DoSaveWithDialog(); //we don't care (here) whether or not this succeeds
}

void GxpmGui::SaveAsCallback(void)
{
  //don't care about success.
  DoSaveAsWithDialog();
}

void GxpmGui::ExitCallback(void)
{
  if( pCore->UnSavedData() )
    {
      
      GxYesNoCancelDialog yncDialog(this);
      yncDialog.SetMessage("Warning: Unsaved Changes.  Save Them?");
      GX_STATUS rStat = yncDialog.DoDialog();
      XUnmapWindow(dInfo.display, yncDialog.GetWindow());

      switch(rStat)
	{
	case GX_YES:
	  if( !DoSaveWithDialog() ) return; //don't exit. save/saveas failed
	  //fall down
	case GX_NO:
	  break; //continue below
	default: //GX_CANCELED
	  return;
	};
    };

  //above can fall down to here
  dInfo.rMainInterface.EndEventLoop();
}

void GxpmGui::ResizeCallback(void)
{
  SizeDialog *pSize = new SizeDialog(this);
  UINT oldWidth = pCore->GetPixWidth();
  UINT oldHeight = pCore->GetPixHeight();
  UINT nWidth = oldWidth;
  UINT nHeight = oldHeight;
  GX_STATUS rStat = pSize->GetNewSize(nWidth,nHeight);
  if(rStat == GX_OK)
    if( (nWidth != oldWidth) || (nHeight != oldHeight) )
      if( pCore->SizePixmap(nWidth, nHeight) )
	{
	  previewWin.SizePixmap(nWidth, nHeight);
	  workWin.SizePixmap(nWidth, nHeight);
	  centerGhost.PlaceChildren();
	  RefreshCallback();
	};

  delete pSize;
}

void GxpmGui::ProgramOptionsCallback(void)
{
  OptionsDialog optDialog(this);

  ProgOptions progOptions;
  progOptions = pCore->GetOptions();

  if( optDialog.DoDialog(progOptions) == GX_OK )
    {
      pCore->SetOptions(progOptions);
      //rebuild the work window.
      workWin.Scale(progOptions.pixelScale);
      centerGhost.PlaceChildren();
      RefreshCallback();
    };
}

void GxpmGui::RefreshCallback(void)
{
  /*
  cout << "in GxpmGui::RefreshCallback" << endl;
  */
  ImgData & rImg = pCore->GetImgData();

  previewWin.UpdatePixmap(rImg);
  workWin.Update(rImg);
}

void GxpmGui::DrawContinuous(bool newState)
{
  //we might not be able to change the state of the work window now.
  if(cState == DRAW_POINTS)
    workWin.SetState(WW_DRAW_CONT);
}

void GxpmGui::MoveBlockCB(void)
{
  statusBar.SetMessage("Select area to move");
  cState = MOVE_AREA_SEL_AREA; //, MOVE_AREA_SEL_DEST
  workWin.SetState(WW_SEL_AREA);
}

void GxpmGui::DrawLineCB(void)
{
  statusBar.SetMessage("Select start of line");
  cState = DRAW_LINE;
  workWin.SetState(WW_DRAW_LINE);
}

void GxpmGui::FillAreaCB(void)
{
  statusBar.SetMessage("Select area");
  cState = FILL_AREA_SEL;
  ConfigWorkWin();
}

bool GxpmGui::DoSaveWithDialog(void)
{
  if( !pCore->PixmapFileNamed() )
    return DoSaveAsWithDialog();

  //hack. check for and return false if error
  pCore->WriteFile();
  return true;
}

bool GxpmGui::DoSaveAsWithDialog(void)
{
  std::string fileName;
  if( pCore->PixmapFileNamed() )
    fileName = pCore->GetFileName();

  GxFileSelector fileSelector(this);
  GX_STATUS stat = fileSelector.SaveFile(fileName, "*.xpm");

  if(stat != GX_OK) return false;

  //hack. check for and return false if error
  pCore->SetFilename(fileName);
  pCore->WriteFile();
  return true;
}

void GxpmGui::DataEventCB(const DrawData &rData)
{
  if(cState == DRAW_POINTS)
    {
      assert(rData.dType == DrawData::DATA_POINT);

      PixValue cColor(dInfo.blackPix);
      colorBox.GetCurrentColor(cColor);
      ImgData & rImg = pCore->GetImgData();
      rImg.SetValue(rData.pointX, rData.pointY, cColor, (PIX_COLOR | PIX_MASKED) );
      RefreshCallback();
      return;
    };

  if(cState == DRAW_LINE)
    {
      ImgData & rImg = pCore->GetImgData();
      if(rData.dType == DrawData::DATA_LINE)
	{
	  //erase the xor
	  rImg.ClearSelected();
	  if( rData.valid ) //else do NOT draw the pixels
	    {
	      //draw in real pixels
	      PixValue pixValue;
	      colorBox.GetCurrentColor(pixValue);
	      rImg.DrawLine(rData.line_x1, rData.line_y1, rData.line_x2, rData.line_y2, pixValue,
			    (PIX_COLOR | PIX_MASKED) );
	    };
	  //refresh the window
	  RefreshCallback();
	  cState = DRAW_POINTS;
	  ConfigWorkWin();
	  return;
	};
      assert(rData.dType == DrawData::DATA_LINE_RUBBER);
      //assuming we are rubbering a line
      //erase the xor
      rImg.ClearSelected();
      //redraw the new xor.
      PixValue pixValue;
      pixValue.selected = true;
      rImg.DrawLine(rData.line_x1, rData.line_y1, rData.line_x2, rData.line_y2, pixValue, PIX_SELECTED);
      //refresh the window
      RefreshCallback();
      return;
    };

  if(cState == FILL_AREA_SEL)
    {
      ImgData & rImg = pCore->GetImgData();
      if(rData.dType == DrawData::DATA_AREA)
	{
	  //erase the xor
	  rImg.ClearSelected();
	  if( rData.valid ) //else do NOT draw the pixels
	    {
	      //draw in real pixels
	      PixValue pixValue;
	      colorBox.GetCurrentColor(pixValue);
	      rImg.FillArea(rData.area_x, rData.area_y, rData.width, rData.height, pixValue,
			    (PIX_COLOR | PIX_MASKED) );
	    };
	  //refresh the window
	  RefreshCallback();
	  cState = DRAW_POINTS;
	  ConfigWorkWin();
	  return;
	};

      assert(rData.dType == DrawData::DATA_AREA_RUBBER);
      //assuming we are rubbering an area to fill.
      //erase the xor
      rImg.ClearSelected();
      //redraw the new xor.
      PixValue pixValue;
      pixValue.selected = true;
      rImg.FillArea(rData.area_x, rData.area_y, rData.width, rData.height, pixValue, PIX_SELECTED);
      //refresh the window
      RefreshCallback();
      return;
    }; 

  if(cState == MOVE_AREA_SEL_AREA)
    {
      ImgData & rImg = pCore->GetImgData();
      //assert();
      //assuming we are rubbering an area to fill.
      //erase the xor
      rImg.ClearSelected();
      //redraw the new xor.
      PixValue pixValue;
      pixValue.selected = true;
      rImg.FillArea(rData.area_x, rData.area_y, rData.width, rData.height, pixValue, PIX_SELECTED);
      //refresh the window
      RefreshCallback();
      if(rData.dType == DrawData::DATA_AREA)
	{
	  cState = MOVE_AREA_SEL_DEST;
	  ConfigWorkWin();
	};
      return;
    };

  if(cState == MOVE_AREA_SEL_DEST)
    {
      ImgData & rImg = pCore->GetImgData();
      rImg.ClearSelected();
      if( rData.valid ) //else do NOT draw the pixels
	{
	  //draw in real pixels
	  PixValue pixValue;
	  colorBox.GetCurrentColor(pixValue);
	  rImg.FillArea(rData.area_x, rData.area_y, rData.width, rData.height, pixValue,
			(PIX_COLOR | PIX_MASKED) );
	};
      //refresh the window
      RefreshCallback();
      cState = DRAW_POINTS;
      ConfigWorkWin();
    };

  assert(false);
}

void GxpmGui::DoMoveBlock(void)
{
  /*
  UINT blockWidth;
  if(coordMatrix[0].x > coordMatrix[1].x)
    blockWidth = coordMatrix[0].x - coordMatrix[1].x;
  else
    blockWidth = coordMatrix[1].x - coordMatrix[0].x;

  UINT blockHeight;
  if(coordMatrix[0].y > coordMatrix[1].y)
    blockHeight = coordMatrix[0].y - coordMatrix[1].y;
  else
    blockHeight = coordMatrix[1].y - coordMatrix[0].y;

  pCore->MoveBlock(blockWidth+1, blockHeight+1, coordMatrix[0],
		   coordMatrix[2]);

  workWin.UpdatePixmap( pCore->GetImageXImage(), pCore->GetShapeXImage() );
  previewWin.UpdatePixmap( pCore->GetImageXImage(), pCore->GetShapeXImage() );
  */
}

void GxpmGui::HandleCurrColorChange(const ColorDef& rColor)
{
  colorBox.SetCurrentColor(rColor);

  std::ostringstream oStr;
  oStr << "Set New Color: " << rColor.colorName;
  statusBar.SetMessage( oStr.str().c_str() );
}

void GxpmGui::HandleUserColorEnter(void)
{
  std::ostringstream oStr;
  oStr << "Right Click on user color to redefine.";
  statusBar.SetMessage( oStr.str().c_str() );  
}

void GxpmGui::HandleUserColorLeave(void)
{

}

void GxpmGui::HandleEditColor(ColorDef &rDef)
{
  ColorDefDialog defDialog(this);

  std::string colorName = rDef.colorName;
  if( defDialog.DoDialog(colorName) )
    {
      if(colorName.empty() ) return;

      XColor screen, exact;
      if( !XAllocNamedColor(dInfo.display, dInfo.cMap, colorName.c_str(), &screen, &exact) )
	{
	  std::cout << "color alloc failed" << std::endl;
	  //do an ok dialog.
	  return;
	};

      //should free existing cell if it defines
      if( rDef.defined )
	{
	  XFreeColors(dInfo.display, dInfo.cMap, &rDef.xcolor, 1, dInfo.cVisualInfo.depth); 
	  rDef.defined = false;
	};

      rDef.defined = true;
      rDef.xcolor  = screen.pixel;
      rDef.colorName = colorName;
      rDef.transparent = false;

      userColorWin.Refresh();
    };
}

void GxpmGui::ConfigWorkWin(void)
{
  switch(cState)
    {
    case DRAW_POINTS:
      if( drawContOption.Checked() )
	workWin.SetState(WW_DRAW_CONT);
      else
	workWin.SetState(WW_DRAW);
      break;
    case DRAW_LINE:
      workWin.SetState(WW_DRAW_LINE);
      break;
    case MOVE_AREA_SEL_AREA:
      workWin.SetState(WW_SEL_AREA);
      break;
    case MOVE_AREA_SEL_DEST:
      workWin.SetState(WW_DRAW);
      break;
    case FILL_AREA_SEL:
      workWin.SetState(WW_SEL_AREA);
      break;
    default:
      assert(false);
    };
}

// ************************* start dialogs **************************

SizeDialog::SizeDialog(GxTopLevelWin *pOwner) :
  GxPopupWin(pOwner), mainColumn(this),
  buttonRow(&mainColumn), hLine(&mainColumn), mainTable(&mainColumn),
  widthRowLabel(&mainTable, "Width:"), widthNumberBox(&mainTable),
  heightRowLabel(&mainTable, "Height:"), heightNumberBox(&mainTable),
  ok(&buttonRow, "Ok"), cancel(&buttonRow, "Cancel")
{
  mainColumn.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT,
				       GX_FLOW_UP,1,1,1,1) );
  widthNumberBox.SetIncrement(1);
  widthNumberBox.Minimum(1);
  widthNumberBox.Maximum(1000);

  heightNumberBox.SetIncrement(1);
  heightNumberBox.Minimum(1);
  heightNumberBox.Maximum(1000);

  mainTable.SetGeomControl( GxSIBasic(GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT,
				      GX_FLOW_UP,1,1,1,1) );

  hLine.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_FIXED, GX_FLOW_LEFT,
				GX_FLOW_DOWN) );
  buttonRow.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT,
				    GX_FLOW_DOWN) );
  buttonRow.SetWidthIdentical(true);
  buttonRow.SetHeightIdentical(true);
  ok.cb.Assign(CbVoidMember<SizeDialog>(this, &SizeDialog::OkCallback));
  cancel.cb.Assign(CbVoidMember<SizeDialog>(this,
					      &SizeDialog::CancelCallback));
}

SizeDialog::~SizeDialog(void)
{}

GX_STATUS SizeDialog::GetNewSize(UINT &rNewWidth, UINT &rNewHeight)
{
  widthNumberBox.Value((int)rNewWidth);
  heightNumberBox.Value((int)rNewHeight);

  Place();
  Create();
  Display();

  EventLoop();

  rNewWidth = (int)widthNumberBox.Value();
  rNewHeight = (int)heightNumberBox.Value();
  //std::cout << "SizeDialog::GetNewSize got new width: " << rNewWidth << " got new height: " << rNewHeight << std::endl;
  return exitStat;
}

void SizeDialog::OkCallback(void)
{
  exitStat = GX_OK;
  processEvents = false;
}

void SizeDialog::CancelCallback(void)
{
  exitStat = GX_CANCELED;
  processEvents = false;
}

// ********************** end SizeDialog **************************

// ********************** start OptionsDialog **************************

OptionsDialog::OptionsDialog(GxTopLevelWin *pOwner) :
  GxPopupWin(pOwner), mainColumn(this),

  rgbRow(&mainColumn),
  rgbLabel(&rgbRow),
  rgbFileName(&rgbRow),
  rgbSelFile(&rgbRow, "Select rgb file..."),

  saveColorsAsNames(&mainColumn, "Save Colors As Names"),
  
  scaleRow(&mainColumn),
  scaleLabel(&scaleRow, "Work Window Scale:"),
  scaleNumberBox(&scaleRow),

  hLine(&mainColumn),

  buttonRow(&mainColumn),
  ok(&buttonRow, "Ok"), cancel(&buttonRow, "Cancel")
{
  mainColumn.SetGeomControl( GxSIBasic(GX_WD_INT, GX_HT_INT, GX_FLOW_LEFT, GX_FLOW_UP, 1,1, 1,1) );

  rgbRow.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT, GX_FLOW_UP) );
  rgbSelFile.cb.Assign( CbVoidMember<OptionsDialog>(this, &OptionsDialog::SelFileCallback) );

  saveColorsAsNames.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT, GX_FLOW_UP) );

  scaleRow.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT, GX_FLOW_UP) );
  scaleNumberBox.SetIncrement(2);
  scaleNumberBox.Minimum(2);
  scaleNumberBox.Maximum(10);

  hLine.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_FIXED, GX_FLOW_LEFT, GX_FLOW_UP) );

  buttonRow.SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT, GX_FLOW_DOWN) );
  buttonRow.SetWidthIdentical(true);
  buttonRow.SetHeightIdentical(true);

  ok.cb.Assign(CbVoidMember<OptionsDialog>(this, &OptionsDialog::OkCallback));
  cancel.cb.Assign(CbVoidMember<OptionsDialog>(this, &OptionsDialog::CancelCallback));

  Place();
  Create();
}
  
OptionsDialog::~OptionsDialog(void)
{}

GX_STATUS OptionsDialog::DoDialog(ProgOptions &rOptions)
{
  rgbFileName.SetText( rOptions.rgbFileName.c_str() );
  if(rOptions.saveColorsAsNames)
    saveColorsAsNames.State(GX_CHECKBOX_CHECKED);
  else
    saveColorsAsNames.State(GX_CHECKBOX_NOT_CHECKED);

  scaleNumberBox.Value( rOptions.pixelScale );

  Display();
  EventLoop();

  rOptions.rgbFileName = rgbFileName.GetText();
  if( saveColorsAsNames.State() == GX_CHECKBOX_CHECKED )
    rOptions.saveColorsAsNames = true;
  else
    rOptions.saveColorsAsNames = false;

  rOptions.pixelScale = scaleNumberBox.Value();

  return exitStat;
}

void OptionsDialog::SelFileCallback(void)
{
  GxFileSelector fileSel(this);

  std::string fileName;
  GX_STATUS rStat = fileSel.OpenFile(fileName, "*.txt");
  if(rStat == GX_OK)
    rgbFileName.SetText( fileName.c_str() );
}

void OptionsDialog::OkCallback(void)
{
  exitStat = GX_OK;
  processEvents = false;
}

void OptionsDialog::CancelCallback(void)
{
  exitStat = GX_CANCELED;
  processEvents = false;
}

// ********************** end OptionsDialog **************************
