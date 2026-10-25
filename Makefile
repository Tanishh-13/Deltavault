
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude
BUILD_DIR = build

.PHONY: all clean test test-chunker test-hasher test-manifest

all: $(BUILD_DIR)/chunker.o $(BUILD_DIR)/hasher.o

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/chunker.o: src/chunker.cpp include/deltavault/chunker.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c src/chunker.cpp -o $@

$(BUILD_DIR)/hasher.o: src/hasher.cpp include/deltavault/hasher.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c src/hasher.cpp -o $@

test-chunker: tests/test_chunker.cpp src/chunker.cpp include/deltavault/chunker.hpp
	$(CXX) $(CXXFLAGS) tests/test_chunker.cpp src/chunker.cpp -o test_chunker.exe
	./test_chunker.exe

test-hasher: tests/test_hasher.cpp src/hasher.cpp include/deltavault/hasher.hpp
	$(CXX) $(CXXFLAGS) tests/test_hasher.cpp src/hasher.cpp -o test_hasher.exe
	./test_hasher.exe

test-manifest: tests/test_manifest.cpp src/manifest.cpp include/deltavault/manifest.hpp
	$(CXX) $(CXXFLAGS) tests/test_manifest.cpp src/manifest.cpp -o test_manifest.exe
	./test_manifest.exe
test: test-chunker test-hasher test-manifest

clean:
	rm -rf $(BUILD_DIR) test_chunker.exe test_hasher.exe test_manifest.exe
