CC = gcc
CFLAGS = -std=c11 -I.
OBJECTS = fdutil.o simplefs.o w25q64fv.o
TARGET = fdutil

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

fdutil.o: fdutil.c defs.h
	$(CC) $(CFLAGS) -g -c fdutil.c

simplefs.o: simplefs.c defs.h
	$(CC) $(CFLAGS) -g -c simplefs.c

w25q64fv.o: w25q64fv.c defs.h
	$(CC) $(CFLAGS) -g -c w25q64fv.c

clean:
	rm -f $(OBJECTS) $(TARGET)

