PROJECT = dnspup
CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -g -O0 -I .
TARGET = bin/dnspup
TEST_TARGET = bin/test_runner

HEADERS = $(shell find ./lib/ -name '*.hpp')
TEST_SOURCES = tests/test_main.cpp

# Main target
all: $(TARGET)

$(TARGET): ./lib/main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) ./lib/main.cpp -o $(TARGET)

# Test Target
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): tests/test_main.cpp $(HEADERS)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(TEST_SOURCES) -o $(TEST_TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
