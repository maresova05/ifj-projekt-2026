CC = gcc
CFLAGS = -Wall -Wextra
DEPFLAGS = -MMD -MP
TARGET = main
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
DEPS = $(OBJ:.o=.d)

.PHONY: build clean

build: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(OBJ) $(DEPS) $(TARGET)

