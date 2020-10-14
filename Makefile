NAME=bibi-tetris
CC=gcc
CFLAGS=-g -ggdb -Wall -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-variable -o $(NAME)
GTKFLAGS=-export-dynamic `pkg-config --cflags --libs gtk+-2.0 cairo libcanberra`

# Top-level rule to create the program
all: tetris

# Compiling the source file
tetris: tetris.c
	$(CC) $(CFLAGS) $^ $(GTKFLAGS)

# Cleaning everything that can be automatically recreated with "make"
clean:
	@/bin/rm -f $(NAME)

new: clean all