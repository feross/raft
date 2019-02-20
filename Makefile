# use the C++ compiler
CC = clang++

# Program names
RAFT_TARGET ?= raft
CLIENT_TARGET ?= client

# Source code folder
SRC_DIR ?= ./src

RAFT_SRCS := $(shell find $(SRC_DIR) \( -name *.cc -or -name *.c \) -and -not -name "$(CLIENT_TARGET).cc")
RAFT_OBJS := $(addsuffix .o,$(basename $(RAFT_SRCS)))
RAFT_DEPS := $(RAFT_OBJS:.o=.d)

CLIENT_SRCS := $(shell find $(SRC_DIR) \( -name *.cc -or -name *.c \) -and -not -name "$(RAFT_TARGET).cc")
CLIENT_OBJS := $(addsuffix .o,$(basename $(CLIENT_SRCS)))
CLIENT_DEPS := $(CLIENT_OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIR) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# compiler flags:
#   -MMD        lists user header files used by the source program
#   -MP         emits dummy dependency rules (use with -MMD)
#   -Wall       turns on most, but not all, compiler warnings
#   -std=c++17  use C++17 dialect
CPPFLAGS ?= $(INC_FLAGS) -MMD -MP -Wall -std=c++17

LDLIBS ?= $(shell pkg-config --cflags --libs protobuf)

all: $(RAFT_TARGET) $(CLIENT_TARGET)

$(RAFT_TARGET): proto $(RAFT_OBJS)
	$(CC) $(LDFLAGS) $(RAFT_OBJS) -o $@ $(LOADLIBES) $(LDLIBS)

$(CLIENT_TARGET): proto $(CLIENT_OBJS)
	$(CC) $(LDFLAGS) $(CLIENT_OBJS) -o $@ $(LOADLIBES) $(LDLIBS)

PROTOS := $(shell find $(SRC_DIR) -name *.proto)
PROTOS_H := $(addsuffix .pb.h,$(basename $(PROTOS)))
PROTOS_CC := $(addsuffix .pb.cc,$(basename $(PROTOS)))

proto: $(PROTOS_H)

$(PROTOS_H): $(PROTOS)
	protoc -I=$(SRC_DIR) --cpp_out=$(SRC_DIR) $(PROTOS)

.PHONY: install-deps
install-deps:
	brew install pkg-config protobuf

.PHONY: clean
clean:
	$(RM) $(RAFT_TARGET) $(RAFT_OBJS) $(RAFT_DEPS) $(CLIENT_TARGET) $(CLIENT_OBJS) $(CLIENT_DEPS) $(PROTOS_H) $(PROTOS_CC)

-include $(RAFT_DEPS)
