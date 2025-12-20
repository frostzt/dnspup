PROJECT = dnspup
CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -g -O0 -I .
TARGET = bin/dnspup
TEST_TARGET = bin/test_runner

HEADERS = $(shell find ./lib/ -name '*.hpp')
TEST_SOURCES = $(shell find ./tests/ -name '*.cpp')

# Main target
all: $(TARGET)

$(TARGET): ./lib/main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) ./lib/main.cpp -o $(TARGET)

# Test Target
test: $(TEST_TARGET)
	./$(TEST_TARGET) -d yes

$(TEST_TARGET): $(TEST_SOURCES) $(HEADERS)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(TEST_SOURCES) -o $(TEST_TARGET)

clean:
	rm -f $(TARGET)
	rm -f $(TEST_TARGET)

run: $(TARGET)
	./$(TARGET)

# Integration tests (Python/pytest)
integration-test: $(TARGET)
	cd tests/integration && python3 -m pytest -v

# Run all tests (unit + integration)
test-all: test integration-test

.PHONY: all clean run test integration-test test-all
