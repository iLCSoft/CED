CC=gcc
LD=gcc
CFLAGS=-g -Wall -I/usr/X11R6/include -I.
LDFLAGS=-g -Wall
LDLIBS= -lglut

all:	glced

glced:	glced.o ced_srv.o ced.o glut_socks.o

#ced_test: ced_test.o ced_cli.o ced.o

glced.o ced_cli.o ced_srv.o ced.o:	ced.h
glced.o ced_cli.o ced_srv.o:		ced_cli.h




gl1:	gl1.o glut_socks.o 

gl2:	gl2.o
	$(LD) $(LDFLAGS) $< -o $@ $(LDLIBS)

gl3:	gl3.o
	$(LD) $(LDFLAGS) $< -o $@ $(LDLIBS)

clean:
	rm -rf *.o gl1 gl2 gl3
