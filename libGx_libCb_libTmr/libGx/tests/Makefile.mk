R_OFFSET_TESTS := tests

test_bins = \
	$(R_OFFSET_TESTS)/test1\
	$(R_OFFSET_TESTS)/test2\
	$(R_OFFSET_TESTS)/test3\
	$(R_OFFSET_TESTS)/test4\
	$(R_OFFSET_TESTS)/test5\
	$(R_OFFSET_TESTS)/test6\
	$(R_OFFSET_TESTS)/test7\
	$(R_OFFSET_TESTS)/test8\
	$(R_OFFSET_TESTS)/maptest

$(R_OFFSET_TESTS)/% :: $(R_OFFSET_TESTS)/%.cc lib/libGx.a
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS) -o $@

.PHONY : tests_clean
tests_clean :
	$(RM) $(test_bins)
	cd tests && $(RM) $(ALL_OBJ_AND_LIB) core
