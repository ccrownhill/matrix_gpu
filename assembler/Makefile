# Based on IAC 2024 compiler coursework makefile (https://github.com/LangProc/langproc-cw),
# which in turn is based on https://stackoverflow.com/a/52036564

CXX := g++
CXXFLAGS := -std=c++20 -I include -g -Wall -Wextra -Werror -fsanitize=address -O3

SOURCES := $(wildcard src/*.cpp)
DEPENDENCIES := $(patsubst src/%.cpp, build/%.d, $(SOURCES))
OBJECTS := $(patsubst src/%.cpp, build/%.o, $(SOURCES))

.PHONY: all clean

all: bin/assembler

bin/assembler: $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^

-include $(DEPENDENCIES)

build/%.o: src/%.cpp Makefile
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MD -MP -c -o $@ $<

clean:
	@rm -rf bin/
	@rm -rf build/
