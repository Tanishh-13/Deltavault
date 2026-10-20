
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude
BUILD_DIR = build

.PHONY: all clean

all: $(BUILD_DIR)/chunker.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/chunker.o: src/chunker.cpp include/deltavault/chunker.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c src/chunker.cpp -o $@

clean:
	$(CXX) --version
