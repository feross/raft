# use the C++ compiler
CC = clang++

# compiler flags:
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -Wall

# the build target executable:
TARGET = raft

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp

clean:
	$(RM) $(TARGET)
