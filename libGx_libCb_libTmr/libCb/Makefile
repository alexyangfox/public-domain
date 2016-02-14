include Make.cfg

.PHONY : lib
lib :
	$(MAKE) -C src
	$(CP) src$(DIR_CHAR)$(LIB_TARGET) lib$(DIR_CHAR)$(LIB_TARGET)

.PHONY : clean
clean :
	$(MAKE) -C src clean
	$(RM) lib$(DIR_CHAR)$(DEL_ALL_EXT)

.PHONY : real_clean
real_clean : clean
	$(RM) Make.cfg