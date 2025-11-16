CXX ?= clang++
AR ?= ar

CXXFLAGS   := -std=c++20 -O2 -Wall -Wextra -Wpedantic
LDFLAGS    :=

LLVM_COMPONENTS := core support irreader

LLVM_CXXFLAGS := $(shell $(LLVM_CONFIG) --cxxflags)
LLVM_LDFLAGS  := $(shell $(LLVM_CONFIG) --ldflags)
LLVM_LIBS     := $(shell $(LLVM_CONFIG) --libs $(LLVM_COMPONENTS))
SYSTEM_LIBS   := $(shell $(LLVM_CONFIG) --system-libs)

BIN := bin/backend

src_files := $(wildcard src/*.cpp)
obj_files := $(patsubst src/%.cpp,build/%.o,$(src_files))

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) -c $< -o $@

$(BIN): $(obj_files) | bin
	$(CXX) -o $@ $^ $(LDFLAGS) $(LLVM_LDFLAGS) $(LLVM_LIBS) $(SYSTEM_LIBS)

build:
	mkdir -p build
	
bin:
	mkdir -p bin

.PHONY: all
all: $(BIN)
