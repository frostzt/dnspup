PROJECT = dnspup
CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -g -O0 -I .
TARGET = bin/dnspup
TEST_TARGET = bin/test_runner

HEADERS = $(shell find ./lib/ -name '*.hpp')
TEST_SOURCES = tests/TestMain.cpp tests/TestStringUtils.cpp

# Main target
all: $(TARGET)

$(TARGET): ./lib/main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) ./lib/main.cpp -o $(TARGET)

# Test Target
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SOURCES) $(HEADERS)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(TEST_SOURCES) -o $(TEST_TARGET)

clean:
	rm -f $(TARGET)
	rm -f $(TEST_TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
