# Makefile

# Compiler
CC = gcc

SERIAL = 1

# Compiler flags
CFLAGS = -DUSE_SERIAL=$(SERIAL)
LIBS = 

# Source files
SRC = *.c


# Object files
OBJ = $(SRC:.c=.o)

# Executable name
TARGET = serial_comm

# Default target: compile the program
all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

# Rule to compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up the build (object files and executable)
clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	sudo ./serial_comm

# Phony targets
.PHONY: all clean


