include ../Make.cfg

objects=\
	TmrTimer.$(OBJ_EXT)\
	TmrTimerMaster.$(OBJ_EXT)\
	TmrUtil.$(OBJ_EXT)

libTmr.$(LIB_EXT) : $(objects)
	$(LDCOMBINE) $(objects) $(LDCOMBINE_OUTPUT_FLAG)libTmr.$(LIB_EXT)

.PHONY : clean
clean :
	$(RM) $(ALL_OBJ_AND_LIB)