CC = gcc
CFLAGS =  -Wextra  -Wvla -g -fsanitize=address # -Werror -fno-omit-frame-pointer -Wall

OBJECTS = sound_seg.o #test.o

all: program

program: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o program

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f program $(OBJECTS)

