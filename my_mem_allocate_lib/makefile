
CFLAGS = -c -g -Wall -std=gnu99
SOURCES = malloc_l.c malloc_l.h malloc_test.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = test

all: $(SOURCES) $(HEADERS) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	gcc $(OBJECTS) -o $@

.c.o:
	gcc $(CFLAGS) $< -o $@

clean:
	rm -rf *.o *~ $(EXECUTABLE)