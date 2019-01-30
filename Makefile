# use the C++ compiler
CC = clang++

# Program name
TARGET ?= raft

# Source code folder
SRC_DIR ?= ./src

SRCS := $(shell find $(SRC_DIR) -name *.cc -or -name *.c -or -name *.s)
OBJS := $(addsuffix .o,$(basename $(SRCS)))
DEPS := $(OBJS:.o=.d)
PROTOS := $(shell find $(SRC_DIR) -name *.proto)

INC_DIRS := $(shell find $(SRC_DIR) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# compiler flags:
#   -MMD        lists user header files used by the source program
#   -MP         emits dummy dependency rules (use with -MMD)
#   -Wall       turns on most, but not all, compiler warnings
#   -std=c++17  use C++17 dialect
CPPFLAGS ?= $(INC_FLAGS) -MMD -MP -Wall -std=c++17

LDLIBS ?= $(shell pkg-config --cflags --libs protobuf)

$(TARGET): proto $(OBJS)
	pkg-config --cflags protobuf  # fails if protobuf is not installed
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LOADLIBES) $(LDLIBS)

proto: $(PROTOS)
	protoc -I=$(SRC_DIR) --cpp_out=$(SRC_DIR) $(PROTOS)

install-deps:
	brew install pkg-config protobuf

.PHONY: clean
clean:
	$(RM) $(TARGET) $(OBJS) $(DEPS)
	$(RM) $(SRC_DIR)/*.pb.h $(SRC_DIR)/*.pb.cc

-include $(DEPS)
