CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g
LDFLAGS = -lrt
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
TARGET = kernel_sim

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $(OBJ) -lrt


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) $(TEST)

clean:
	rm -f $(OBJ) $(TARGET)

re: clean all
