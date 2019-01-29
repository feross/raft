# use the C++ compiler
CC = clang++

# Program name
TARGET ?= raft

# Source code folder
SRC_DIRS ?= ./src

SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)
OBJS := $(addsuffix .o,$(basename $(SRCS)))
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# compiler flags:
#   -MMD        lists user header files used by the source program
#   -MP         emits dummy dependency rules (use with -MMD)
#   -Wall       turns on most, but not all, compiler warnings
#   -std=c++17  use C++17 dialect
CPPFLAGS ?= $(INC_FLAGS) -MMD -MP -Wall -std=c++17

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LOADLIBES) $(LDLIBS)

.PHONY: clean
clean:
	$(RM) $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)
