# Define compiler and flags
CC = gcc
CFLAGS = -Wall -std=c99 -DDEBUGMODE=1

# Define target executable
TARGET = rudra

# Define source files
SRC = rudra.c usbComm.c

# Define object files (automatically replaces .c with .o)
OBJ = $(SRC:.c=.o)

# Define header dependencies
DEPS = protocol.h rudra.h usbComm.h

# Define libraries
LIBS = -lusb-1.0  

# Build the target
all: $(TARGET)

# Link object files to create the final executable
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LIBS)

# Compile each .c file into an object file
%.o: %.c $(DEPS)
	$(CC) -c $(CFLAGS) $< -o $@

test: $(TARGET)
	sudo ./$(TARGET) -t ./8809_00400000_usb.fp

run: $(TARGET)
	sudo ./$(TARGET)

# Clean up object files and executable
clean:
	rm -f $(OBJ) $(TARGET)
