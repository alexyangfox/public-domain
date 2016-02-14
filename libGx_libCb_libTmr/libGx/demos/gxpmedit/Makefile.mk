R_OFFSET_GXPMEDIT := demos/gxpmedit

gxpmedit_objects = \
	$(R_OFFSET_GXPMEDIT)/PreviewWin.$(OBJ_EXT)\
	$(R_OFFSET_GXPMEDIT)/ColorWin.$(OBJ_EXT)\
	$(R_OFFSET_GXPMEDIT)/PixValue.$(OBJ_EXT)\
	$(R_OFFSET_GXPMEDIT)/ImgData.$(OBJ_EXT)\
	$(R_OFFSET_GXPMEDIT)/WorkWin.$(OBJ_EXT)\
	$(R_OFFSET_GXPMEDIT)/ColorDefDialog.$(OBJ_EXT)\
	$(R_OFFSET_GXPMEDIT)/GxpmCore.$(OBJ_EXT)\
	$(R_OFFSET_GXPMEDIT)/GxpmGui.$(OBJ_EXT)\
	$(R_OFFSET_GXPMEDIT)/gxpmedit.$(OBJ_EXT)

$(R_OFFSET_GXPMEDIT)/gxpmedit : $(gxpmedit_objects) lib/libGx.a
	$(CXX) $(CXXFLAGS) $(gxpmedit_objects) lib/libGx.a $(LDFLAGS) -o $(R_OFFSET_GXPMEDIT)/gxpmedit

.PHONY : gxpmedit_clean
gxpmedit_clean :
	cd $(R_OFFSET_GXPMEDIT) && $(RM) $(ALL_OBJ_AND_LIB) gxpmedit
