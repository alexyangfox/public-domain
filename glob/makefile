CFLAGS=-O

GOBJ = glob.o
TOBJ = glot.o

all: $(GOBJ)

glot: $(GOBJ) $(TOBJ)
	$(CC) -o glot $(GOBJ) $(TOBJ)

test: glot glot.dat
	glot glot.dat

clean:
	rm -f *.o core	
