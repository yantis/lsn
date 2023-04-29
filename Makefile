DEBUG =

# Compiler to use
CC = gcc

# Compiler flags for added warnings, checks, and optimizations
CFLAGS = -Wall -Wextra -pedantic -O3 $(DEBUG)

# Name of the executable to generate
TARGET = lsn

# Source file(s) for the project
SRC = lsn.c

# Default target executed when running 'make'
all: $(TARGET)

# Target to compile the source files into an executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Target to remove the generated executable
clean:
	rm -f $(TARGET)

# List of phony targets that don't represent actual files
.PHONY: all clean
