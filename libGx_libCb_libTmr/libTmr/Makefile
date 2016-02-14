include Make.cfg

.PHONY : libTmr
libTmr :
	$(MAKE) -C src
	$(CP) src/libTmr.$(LIB_EXT) lib/libTmr.$(LIB_EXT)

.PHONY : clean
clean :
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
	$(RM) lib/*.o lib/*.a

.PHONY : real_clean
real_clean : clean
	$(RM) Make.cfg
