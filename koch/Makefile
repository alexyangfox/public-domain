## Uncomment following line for X11
#LDFLAGS=-lglut -lGLU -lGL -lm
## Uncomment following line for MacOS X
LDFLAGS=-framework OpenGL -framework GLUT -framework Foundation
CFLAGS=-O2 -Wall -g
koch: koch.c
	$(CC) $(CFLAGS) -o koch koch.c $(LDFLAGS)

clean:
	rm -fr koch
