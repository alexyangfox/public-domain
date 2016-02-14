
SW_LOC=/home/john/sw

CXXFLAGS=-I$(SW_LOC)/libCb/include -I$(SW_LOC)/libTmr/include -I$(SW_LOC)/libGx/include
LDFLAGS=$(SW_LOC)/libGx/lib/libGx.a $(SW_LOC)/libCb/lib/libCb.a $(SW_LOC)/libTmr/lib/libTmr.a -lX11

objects=\
	dtmain.o\
	MyDialog.o

dtest : $(objects)
	$(CXX) $(objects) $(LDFLAGS) -o dtest

.PHONY : clean
clean :
	$(RM) *.o dtest