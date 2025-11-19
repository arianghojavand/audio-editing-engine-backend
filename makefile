CC = gcc
CFLAGS =  -Wextra  -Wvla -g -fsanitize=address -Werror -Wall # -fno-omit-frame-pointer

OBJECTS = sound_seg.o test.o

all: program

program: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o program

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f program $(OBJECTS)

