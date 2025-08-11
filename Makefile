CC = gcc -g
CFLAGS = -Wall -O2 -I/usr/include/X11 -I/usr/include/Motif -I/home/jack/MyProg/CADGui
LIBS = -lXm -lXt -lX11 -lGL -lGLU -lXext -lm

all: motif_glx_spline

motif_glx_spline: motif_glx_spline.o pslib.o tinyspline.o parson.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

parson.o: parson.c parson.h
	$(CC) $(CFLAGS) -c parson.c

motif_glx_spline.o: motif_glx_spline.c pslib.h
	$(CC) $(CFLAGS) -c motif_glx_spline.c

pslib.o: pslib.c pslib.h
	$(CC) $(CFLAGS) -c pslib.c

tinyspline.o: tinyspline.c tinyspline.h
	$(CC) $(CFLAGS) -c tinyspline.c

clean:
	rm -f motif_glx_spline motif_glx_spline.o pslib.o tinyspline.o
