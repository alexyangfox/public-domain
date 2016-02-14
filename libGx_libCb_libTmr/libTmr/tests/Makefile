include ../Make.cfg

objects=\
	main.$(OBJ_EXT)

ttest : $(objects)
	$(CXX) $(objects) ../lib/libTmr.$(LIB_EXT) $(LIBCB_INCLUDE_DIR)/../lib/libCb.a -o ttest

.PHONY : clean
clean :
	$(RM) $(ALL_OBJ_AND_LIB) ttest