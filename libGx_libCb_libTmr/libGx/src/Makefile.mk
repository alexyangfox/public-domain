R_OFFSET_SRC := src

src_objects = \
	$(R_OFFSET_SRC)/GxFraction.o\
	$(R_OFFSET_SRC)/GxWinArea.o\
	$(R_OFFSET_SRC)/GxOwner.o\
	$(R_OFFSET_SRC)/GxFocusMaster.o\
	$(R_OFFSET_SRC)/GxCentralColor.o\
	$(R_OFFSET_SRC)/GxCoreWinMap.o\
	$(R_OFFSET_SRC)/GxRealOwner.o\
	$(R_OFFSET_SRC)/GxMapHolder.o\
	$(R_OFFSET_SRC)/GxSubMapHolder.o\
	$(R_OFFSET_SRC)/GxErrorSink.o\
	$(R_OFFSET_SRC)/GxMainInterface.o\
	$(R_OFFSET_SRC)/GxDisplay.o\
	$(R_OFFSET_SRC)/GxTrueColorMapInfo.o\
	$(R_OFFSET_SRC)/GxDisplayInfo.o\
	$(R_OFFSET_SRC)/GxVolatileData.o\
	$(R_OFFSET_SRC)/GxImageObject.o\
	$(R_OFFSET_SRC)/GxCoreWin.o\
	$(R_OFFSET_SRC)/GxCoreOwnerWin.o\
	$(R_OFFSET_SRC)/GxWin.o\
	$(R_OFFSET_SRC)/GxOwnerWin.o\
	$(R_OFFSET_SRC)/GxGeomControl.o\
	$(R_OFFSET_SRC)/GxGhost.o\
	$(R_OFFSET_SRC)/GxRootTransient.o\
	$(R_OFFSET_SRC)/GxTopLevelWin.o\
	$(R_OFFSET_SRC)/GxMainWin.o\
	$(R_OFFSET_SRC)/GxPopupWin.o\
	$(R_OFFSET_SRC)/GxColumn.o\
	$(R_OFFSET_SRC)/GxRow.o\
	$(R_OFFSET_SRC)/GxFrame.o\
	$(R_OFFSET_SRC)/GxArguments.o\
	$(R_OFFSET_SRC)/GxVLine.o\
	$(R_OFFSET_SRC)/GxHLine.o\
	$(R_OFFSET_SRC)/GxCBList.o\
	$(R_OFFSET_SRC)/GxMenuBar.o\
	$(R_OFFSET_SRC)/GxMenuItemOwner.o\
	$(R_OFFSET_SRC)/GxMenu.o\
	$(R_OFFSET_SRC)/GxPopupMenu.o\
	$(R_OFFSET_SRC)/GxMenuItems.o\
	$(R_OFFSET_SRC)/GxNoFocusButtonBase.o\
	$(R_OFFSET_SRC)/GxFocusButtonBase.o\
	$(R_OFFSET_SRC)/GxButton.o\
	$(R_OFFSET_SRC)/GxCheckBox.o\
	$(R_OFFSET_SRC)/GxStateButton.o\
	$(R_OFFSET_SRC)/GxToggleButton.o\
	$(R_OFFSET_SRC)/GxToolTip.o\
	$(R_OFFSET_SRC)/GxPulldownMenu.o\
	$(R_OFFSET_SRC)/GxToolBarButton.o\
	$(R_OFFSET_SRC)/GxToolBar.o\
	$(R_OFFSET_SRC)/GxToolBarSimpleDock.o\
	$(R_OFFSET_SRC)/GxToolBarCplxDockCore.o\
	$(R_OFFSET_SRC)/GxToolBarDock.o\
	$(R_OFFSET_SRC)/GxToolBarManager.o\
	$(R_OFFSET_SRC)/GxToolBarFloatingDock.o\
	$(R_OFFSET_SRC)/GxKeyHandler.o\
	$(R_OFFSET_SRC)/GxCursor.o\
	$(R_OFFSET_SRC)/GxEditWin.o\
	$(R_OFFSET_SRC)/GxTextWin.o\
	$(R_OFFSET_SRC)/GxNumberBox.o\
	$(R_OFFSET_SRC)/GxSlideGrip.o\
	$(R_OFFSET_SRC)/GxSlider.o\
	$(R_OFFSET_SRC)/GxScrollBar.o\
	$(R_OFFSET_SRC)/GxArrowButton.o\
	$(R_OFFSET_SRC)/GxFilledArrowButton.o\
	$(R_OFFSET_SRC)/GxRadioBox.o\
	$(R_OFFSET_SRC)/GxList.o\
	$(R_OFFSET_SRC)/GxListTree.o\
	$(R_OFFSET_SRC)/GxNewList.o\
	$(R_OFFSET_SRC)/GxVDivider.o\
	$(R_OFFSET_SRC)/GxFileSelector.o\
	$(R_OFFSET_SRC)/GxMiscDialogs.o\
	$(R_OFFSET_SRC)/GxLabel.o\
	$(R_OFFSET_SRC)/GxStatusBar.o\
	$(R_OFFSET_SRC)/GxScrolledWin.o\
	$(R_OFFSET_SRC)/GxPercentBar.o\
	$(R_OFFSET_SRC)/GxLabeledBorder.o\
	$(R_OFFSET_SRC)/GxTable.o\
	$(R_OFFSET_SRC)/GxTabManager.o\
	$(R_OFFSET_SRC)/GxTabPane.o

#include dependacy information
#hack. I don't want to include this (and cause dependancy info to be rebuilt)
#on a make clean.
-include $(src_objects:.o=.d)

#generate dependancy information
%.d : %.cc
	$(DEPEND) -MT $(<:.cc=.o) $< > $@

src/libGx.a : $(src_objects)
	ar -rc  src/libGx.a $(src_objects)
	ranlib src/libGx.a

.PHONY : src_clean
src_clean :
	cd src && $(RM) $(ALL_OBJ_AND_LIB) sta*
