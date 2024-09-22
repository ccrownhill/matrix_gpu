CXX := clang++
CXXFLAGS := --std=c++20 -Iinclude -Wall -Wextra -O3
SRC := $(wildcard src/*.cpp)
INC := $(wildcard include/*.hpp)
DEPS := $(patsubst src/%.cpp, build/%.d, $(SRC))
OBJ := $(patsubst src/%.cpp, build/%.o, $(SRC))

bin/conv: $(OBJ)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $^ -o $@

-include $(DEPS)

build/%.o: src/%.cpp Makefile
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MD -MP -c $< -o $@

clean:
	@rm -rf bin/
	@rm -rf build/

.PHONY: all clean
