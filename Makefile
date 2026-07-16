# Makefile for Escape Game
# COMP2002 Unix Systems Programming

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -g
TARGET = escape

SOURCES = main.c game.c logic.c player.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = game.h logic.h player.h

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)

run: $(TARGET)
	./$(TARGET) map.txt

valgrind: $(TARGET)
	valgrind --trace-children=yes ./$(TARGET) map.txt

.PHONY: all clean run valgrind
