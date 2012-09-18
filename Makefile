CC := gcc
CFLAGS := -Wall -Werror
LDFLAGS := 


OBJECTS := shell.o

all: w4118_sh


w4118_sh: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) -g

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -g

clean:
	rm -f w4118_sh
	rm -f shell.o

.PHONY: clean
