CC = gcc
FC = gfortran
CFLAGS = -O2 -Wall `pkg-config --cflags x11 gl`
LDFLAGS = -lX11 -lGL -lGLU

SRCS = cad_gui.c
OBJS = $(SRCS:.c=.o)
FSRC = cad_fortran.f
FOBJ = cad_fortran.o
TARGET = cad_spline

all: $(TARGET)

$(TARGET): $(OBJS) $(FOBJ)
	$(FC) -o $(TARGET) $(FOBJ) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.f
	$(FC) -c $< -o $@

clean:
	rm -f *.o $(TARGET)
