CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99

SRC = src/main.c src/editor.c src/terminal.c src/input.c
OBJ = $(SRC:.c=.o)
TARGET = micro

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

%.o: %.c editor.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean