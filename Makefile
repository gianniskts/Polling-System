# make a makefile for the file poller.cpp

# Compiler
CC = g++

# Compiler flags
CFLAGS = -Wall -g

# Linker flags
LFLAGS = -Wall -g

# Libraries
LIBS = -lpthread

# Source files
SRC = poller.cpp

# Object files
OBJ = $(SRC:.cpp=.o)

# Executable
EXE = poller

# Default target
all: $(EXE)

# Executable target
$(EXE): $(OBJ)
	$(CC) $(LFLAGS) $(OBJ) -o $(EXE) $(LIBS)

# Clean target
clean:
	rm -f $(OBJ) $(EXE)

# ./poller 5634 8 16 pollLog.txt pollStats.txt
