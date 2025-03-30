CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wvla -fPIC -fsanitize=address -g

OBJECTS = sound_seg.o #test.o

all: program

program: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o program

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f program $(OBJECTS)

