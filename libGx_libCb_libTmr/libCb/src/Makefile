include ../Make.cfg

objects =\
	CbCallback.$(OBJ_EXT)

libCb.$(LIB_EXT) : $(objects)
	$(LD_COMBINE) $(objects) $(LD_COMBINE_OUTPUT_FLAG)libCb.$(LIB_EXT)

.phony : clean
clean :
	$(RM) $(ALL_OBJ_AND_LIB)