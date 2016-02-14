R_OFFSET_GXTE := demos/gxte

gxte_objects = \
	$(R_OFFSET_GXTE)/GxteGui.$(OBJ_EXT)\
	$(R_OFFSET_GXTE)/gxte.$(OBJ_EXT)

$(R_OFFSET_GXTE)/gxte : $(gxte_objects) lib/libGx.a
	$(CXX) $(CXXFLAGS) $(gxte_objects) lib/libGx.a $(LDFLAGS) -o $(R_OFFSET_GXTE)/gxte

.PHONY : gxte_clean
gxte_clean :
	cd $(R_OFFSET_GXTE) && $(RM) $(ALL_OBJ_AND_LIB) gxte
