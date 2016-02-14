LDFLAGS=-L/usr/X11R6/lib -lglut -lGLU -lGL
CFLAGS=-O2
# if you do not want sound support using ALUT comment this out:
CPPFLAGS+=-DGLSFCAVE_SOUND 
LDFLAGS+=-lalut

glsfcave: glsfcave.o
	$(CC) $(CFLAGS) -o glsfcave glsfcave.o $(LDFLAGS) 

glsfcave.o: glsfcave.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o glsfcave.o glsfcave.c

clean:
	rm -f *.o glsfcave
